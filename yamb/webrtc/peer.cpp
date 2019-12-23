///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier:	LGPL-2.1+
//
/// @addtogroup webrtc
/// @{


#include "peer.h"
#include "peermanager.h"
#include "peerfactorycontext.h"
#include "../base/logger.h"


using std::endl;


namespace scy {
  namespace wrtc {

    void DataChannelObserver::OnStateChange() {
      LDebug("channel: ", m_dataChannel->label(), " state: ",
             webrtc::DataChannelInterface::DataStateString(m_dataChannel->state()))
      isOpened = std::string(webrtc::DataChannelInterface::DataStateString(m_dataChannel->state())) ==
                 std::string("open");
      if (isOpened) {
        _p->manager()->onCreatedConnection(_p->peerid(), _p->lastRoom());
//          std::string msg(
//              m_dataChannel->label() + " " + webrtc::DataChannelInterface::DataStateString(m_dataChannel->state()));
//          webrtc::DataBuffer buffer(msg);
//          m_dataChannel->Send(buffer);
      }
    }

    void DataChannelObserver::OnMessage(const webrtc::DataBuffer &buffer) {
      //std::string msg((const char *) buffer.data.data(), buffer.data.size());

      Message::Ptr m = Message::Create();
      m->webrtcbuffer = std::move(const_cast<webrtc::DataBuffer &>(buffer));
      //LDebug("Incoming webrtc message: ", (const char *) m->webrtcbuffer.data.data())
      _p->manager()->processWebRTCMessage(m);
    }

    void getIceServerFromUrl(const std::string &url, std::string &outUrl, std::string &outUser, std::string &outPass) {
      outUrl = url;
      std::size_t pos = url.find_first_of(':');
      if (pos != std::string::npos) {
        std::string protocol = url.substr(0, pos);
        std::string uri = url.substr(pos + 1);
        std::string credentials;

        std::size_t pos = uri.find('@');
        if (pos != std::string::npos) {
          credentials = uri.substr(0, pos);
          uri = uri.substr(pos + 1);
        }

        outUrl = protocol + ":" + uri;

        if (!credentials.empty()) {
          pos = credentials.find(':');
          if (pos == std::string::npos) {
            outUser = credentials;
          } else {
            outUser = credentials.substr(0, pos);
            outPass = credentials.substr(pos + 1);
          }
        }
      }
      return;
    }

    void Peer::addGoogleStun() {
      webrtc::PeerConnectionInterface::IceServer stun;
      stun.uri = "stun:stun.l.google.com:19302";
      _config.servers.push_back(stun);
    }


    Peer::Peer(PeerManager *manager,
               PeerFactoryContext *context,
               const std::string &peerid,
               const std::string &token,
               Mode mode,
               const std::string &lastRoom,
               const std::vector<std::string> &ice_servers,
               int openPortsStart,
               int openPortsEnd) : _manager(manager), _context(context), _peerid(peerid), _token(token),
                                   _mode(mode), _lastRoom(lastRoom)
        //, _context->factory(manager->factory())
        , _peerConnection(nullptr), _stream(nullptr), destroying(false), closing(false), m_remoteChannel(nullptr),
                                   m_localChannel(
                                       nullptr) {
      for (auto it = ice_servers.begin(); it != ice_servers.end(); ++it) {
        if (!(*it).empty()) {
          std::string url;
          std::string user;
          std::string pass;
          getIceServerFromUrl(*it, url, user, pass);
          webrtc::PeerConnectionInterface::IceServer ice;
          ice.urls.push_back(url);
          if (!user.empty() && !pass.empty()) {
            ice.username = user;
            ice.password = pass;
          }
          _config.servers.push_back(ice);
        }
      }
      _config.enable_dtls_srtp = true;
      //addGoogleStun();

      if (openPortsStart && openPortsEnd) {
        setPortRange(openPortsStart, openPortsEnd);
      }
    }

    Peer::~Peer() {
      LDebug(_peerid, ": Destroying")
      // closeConnection();
      delete m_remoteChannel;
      delete m_localChannel;
      if (_peerConnection && !closing) {
        closing = true;
        _peerConnection->Close();
      }
      LDebug(_peerid, ": Destroyed")
    }


    rtc::scoped_refptr<webrtc::MediaStreamInterface> Peer::createMediaStream() {
      assert(_mode == Offer);
      //assert(_context->factory);
      assert(!_stream);
      _stream = _context->factory->CreateLocalMediaStream(kStreamLabel);
      return _stream;
    }


    void Peer::setPortRange(int minPort, int maxPort) {
      assert(!_peerConnection);

      if (!_context->networkManager) {
        throw std::runtime_error("Must initialize custom network manager to set port range");
      }

      if (!_context->socketFactory) {
        throw std::runtime_error("Must initialize custom socket factory to set port range");
      }

      if (!_portAllocator)
        _portAllocator.reset(new cricket::BasicPortAllocator(
            _context->networkManager.get(),
            _context->socketFactory.get()));
      _portAllocator->SetPortRange(minPort, maxPort);
    }


    void Peer::createConnection() {
      assert(_context->factory);
      _peerConnection = _context->factory->CreatePeerConnection(_config,
                                                                std::move(_portAllocator), nullptr, this);
      if (_stream) {
        if (!_peerConnection->AddStream(_stream)) {
          throw std::runtime_error("Adding stream to Peer failed");
        }
      }
    }


    void Peer::closeConnection() {
      LDebug(_peerid, ": Closing")

      if (_peerConnection && !closing) {
        closing = true;
        //std::this_thread::__sleep_for(std::chrono::seconds{2}, std::chrono::nanoseconds{2} );
        LDebug(_peerid, ": Closing enter")
        _peerConnection->Close();
        LDebug(_peerid, ": Closed end")
      } else {
        // Call onClosed if no connection has been
        // made so callbacks are always run.
        _manager->onClosed(this);
      }
    }


    void Peer::createOffer() {
      assert(_mode == Offer);
      assert(_peerConnection);
      if (_peerConnection.get() && _mode == Offer) {
        LDebug("CreateDataChannel peerid: ", _peerid)
        rtc::scoped_refptr<webrtc::DataChannelInterface> channel = _peerConnection->CreateDataChannel(
            std::string("Channel_owner_is_") + _peerid,
            nullptr);
        m_localChannel = new DataChannelObserver(channel, this);
        //std::make_shared<DataChannelObserver>(DataChannelObserver(channel, _manager->onMessageSignals()));
      }
      _peerConnection->CreateOffer(this, offerAnswerOptions());
    }


    void Peer::recvSDP(const std::string &type, const std::string &sdp) {
      //LDebug(_peerid, ": Receive ", type, ": ", sdp)

      webrtc::SdpParseError error;
      webrtc::SessionDescriptionInterface *desc(
          webrtc::CreateSessionDescription(type, sdp, &error));
      if (!desc) {
        throw std::runtime_error("Can't parse remote SDP: " + error.description);
      }
      _peerConnection->SetRemoteDescription(
          DummySetSessionDescriptionObserver::Create(this), desc);

      if (type == "offer") {
        assert(_mode == Answer);
        _peerConnection->CreateAnswer(this, offerAnswerOptions());
      } else {
        assert(_mode == Offer);
      }
    }

    void
    Peer::recvSDP(const std::string &type, const std::string &sdp, const std::vector<scy::json::value> &iceCandidates) {
      //LDebug(_peerid, ": Receive ", type, ": ", sdp)

      webrtc::SdpParseError error;
      webrtc::SessionDescriptionInterface *desc(
          webrtc::CreateSessionDescription(type, sdp, &error));
      if (!desc) {
        throw std::runtime_error("Can't parse remote SDP: " + error.description);
      }
      _peerConnection->SetRemoteDescription(
          DummySetSessionDescriptionObserver::Create(this, iceCandidates), desc);

      if (type == "offer") {
        assert(_mode == Answer);
        _peerConnection->CreateAnswer(this, offerAnswerOptions());
      } else {
        assert(_mode == Offer);
      }
    }


    void Peer::recvCandidate(const std::string &mid, int mlineindex,
                             const std::string &sdp) {
      webrtc::SdpParseError error;
//      _pendingCandidates.emplace_back(webrtc::CreateIceCandidate(mid, mlineindex, sdp, &error));
      std::unique_ptr<webrtc::IceCandidateInterface> candidate(
          webrtc::CreateIceCandidate(mid, mlineindex, sdp, &error));
      if (!candidate) {
        throw std::runtime_error("Can't parse remote candidate: " + error.description);
      }
      if (_peerConnection->current_remote_description()) {
        _peerConnection->AddIceCandidate(candidate.get());
      } else {
        _pendingCandidates.push_back(candidate.release());
      }
    }

    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions &Peer::offerAnswerOptions() {
      _offerAnswerOptions.offer_to_receive_audio = false;
      _offerAnswerOptions.offer_to_receive_video = false;
      if (_mode == Offer)
        _offerAnswerOptions.ice_restart = true;
      return _offerAnswerOptions;
    }


    void Peer::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {
      LDebug(_peerid, ": On signaling state change: ", new_state)

      switch (new_state) {
        case webrtc::PeerConnectionInterface::kStable:
          _manager->onStable(this);
          break;
        case webrtc::PeerConnectionInterface::kClosed:
          //_manager->onClosed(this);
          break;
        case webrtc::PeerConnectionInterface::kHaveLocalOffer:
        case webrtc::PeerConnectionInterface::kHaveRemoteOffer:
        case webrtc::PeerConnectionInterface::kHaveLocalPrAnswer:
        case webrtc::PeerConnectionInterface::kHaveRemotePrAnswer:
          break;
      }
    }

    DataChannelObserver *Peer::getDataChannel() const {
      if (destroying || closing)
        return nullptr;
      if (m_remoteChannel != nullptr && m_remoteChannel->isOpened)
        return m_remoteChannel;
      else if (m_localChannel != nullptr && m_localChannel->isOpened)
        return m_localChannel;
        //else throw std::runtime_error("All channels(remote and local) are nullptrs.");
      else return nullptr;
    }

    void Peer::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {
      LDebug(_peerid, ": On ICE connection change: ", new_state)
      if ((new_state == webrtc::PeerConnectionInterface::kIceConnectionFailed) && !destroying) {
        destroying = true;
        _manager->onClosed(this);
        //this->closeConnection();
      }
    }


    void Peer::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) {
      LDebug(_peerid, ": On ICE gathering change: ", new_state)
    }


    void Peer::OnRenegotiationNeeded() {
      LDebug(_peerid, ": On renegotiation needed")
    }


    void Peer::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
      // proxy to deprecated OnAddStream method
      OnAddStream(stream.get());
    }


    void Peer::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
      // proxy to deprecated OnRemoveStream method
      OnRemoveStream(stream.get());
    }

    void Peer::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
      LDebug("On DataChannel:", channel->label())
      m_remoteChannel = new DataChannelObserver(channel, this);
      //channel->RegisterObserver(m_remoteChannel);
      //DataChannelObserver::Create(std::move(channel), _manager->onMessageSignals());
    }


    void Peer::OnAddStream(webrtc::MediaStreamInterface *stream) {
      assert(_mode == Answer);

      LDebug(_peerid, ": On add stream")
      _manager->onAddRemoteStream(this, stream);
    }


    void Peer::OnRemoveStream(webrtc::MediaStreamInterface *stream) {
      assert(_mode == Answer);

      LDebug(_peerid, ": On remove stream")
      _manager->onRemoveRemoteStream(this, stream);
    }


    void Peer::OnIceCandidate(const webrtc::IceCandidateInterface *candidate) {
      std::string sdp;
      if (!candidate->ToString(&sdp)) {
        LError(_peerid, ": Failed to serialize candidate")
        assert(0);
        return;
      }

      _manager->sendCandidate(this, candidate->sdp_mid(),
                              candidate->sdp_mline_index(), sdp);
    }


    void Peer::OnSuccess(webrtc::SessionDescriptionInterface *desc) {
      LDebug(_peerid, ": Set local description")
      _peerConnection->SetLocalDescription(
          DummySetSessionDescriptionObserver::Create(), desc);

      // Send an SDP offer to the peer
      std::string sdp;
      if (!desc->ToString(&sdp)) {
        LError(_peerid, ": Failed to serialize local sdp")
        assert(0);
        return;
      }

      _manager->sendSDP(this, desc->type(), sdp);
    }


    void Peer::OnFailure(const std::string &error) {
      LError(_peerid, ": On failure: ", error)

      _manager->onFailure(this, error);
    }


    void Peer::setPeerFactory(rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory) {
      assert(!_context->factory); // should not be already set via PeerManager
      _context->factory = factory;
    }


    std::string Peer::peerid() const {
      return _peerid;
    }

    std::string Peer::lastRoom() const {
      return _lastRoom;
    }

    std::string Peer::token() const {
      return _token;
    }

    webrtc::PeerConnectionFactoryInterface *Peer::factory() const {
      return _context->factory.get();
    }


    rtc::scoped_refptr<webrtc::PeerConnectionInterface> Peer::peerConnection() const {
      return _peerConnection;
    }


    rtc::scoped_refptr<webrtc::MediaStreamInterface> Peer::stream() const {
      return _stream;
    }


//
// Dummy Set Peer Description Observer
//


    void DummySetSessionDescriptionObserver::OnSuccess() {
      if (_p) {
        std::for_each(_iceCandidates.begin(), _iceCandidates.end(), [this](const scy::json::value &iceCandidate) {
          std::string mid = iceCandidate.value(kCandidateSdpMidName, "");
          int mlineindex = iceCandidate.value(kCandidateSdpMlineIndexName, -1);
          std::string sdp = iceCandidate.value(kCandidateSdpName, "");
          if (mlineindex == -1 || mid.empty() || sdp.empty()) {
            LError("Invalid candidate format")
            assert(0 && "bad candiate");
            return;
          }
          LDebug("Received candidate: ", sdp)
          _p->recvCandidate(mid, mlineindex, sdp);
        });
        for (auto i = 0; i < _p->pendingCandidates().size(); ++i) {
          _p->peerConnection()->AddIceCandidate(_p->pendingCandidates()[i]);
          delete _p->pendingCandidates()[i];
        }
      }
      LDebug("On SDP parse success")
    }


    void DummySetSessionDescriptionObserver::OnFailure(const std::string &error) {
      LError("On SDP parse error: ", error)
      assert(0);
    }


  }
} // namespace scy::wrtc


/// @\}
