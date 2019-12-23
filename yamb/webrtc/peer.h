///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier:	LGPL-2.1+
//
/// @addtogroup webrtc
/// @{


#ifndef SCY_WebRTC_Peer_H
#define SCY_WebRTC_Peer_H


#include "peerfactorycontext.h"

#include "api/jsep.h"
#include "api/peer_connection_interface.h"
//#include "api/test/fakeconstraints.h"
#include "p2p/client/basic_port_allocator.h"
#include "../base/collection.h"
#include "../json/json.h"

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier:	LGPL-2.1+
//
/// @addtogroup webrtc
/// @{
#include <cstdio>
#include "../rapidjson/document.h"     // rapidjson's DOM-style API
#include "../rapidjson/prettywriter.h" // for stringify JSON

template<typename T>
class TSQueue {
public:
  typedef std::shared_ptr<TSQueue> Ptr;

  T pop() {
    std::unique_lock<std::mutex> mlock(_mutex);
    if (!_queue.empty()) {
      auto item = _queue.front();
      _queue.pop();
      return item;
    }
    return nullptr;
  }

  void push(T item) {
    std::unique_lock<std::mutex> mlock(_mutex);
    _queue.push(item);
  }

  bool isEmpty() const {
    return _queue.empty();
  }

  inline static Ptr Create() {
    return std::make_shared<TSQueue>();
  }

protected:
  std::queue<T> _queue;
  std::mutex _mutex;
};

template<typename T>
class TSDeque {
public:
  typedef std::shared_ptr<TSDeque> Ptr;

  T get() {
    std::unique_lock<std::mutex> mlock(_mutex);
    if (!_queue.empty()) {
      auto item = _queue.front();
      _queue.pop_front();
      return item;
    }
    return nullptr;
  }

  void push(T item) {
    std::unique_lock<std::mutex> mlock(_mutex);
    _queue.push_back(item);
  }

  bool isEmpty() const {
    return _queue.empty();
  }

  inline static Ptr Create() {
    return std::make_shared<TSDeque>();
  }

protected:
  std::deque<T> _queue;
  std::mutex _mutex;
};

namespace scy {
  namespace wrtc {

    class Peer;

    class PeerManager;

    enum MessageType {
      AnnounceWebrtc, Logic, UpdateMap, Data, System, Stat, Unknown, Ping, Pong
    };

    struct Message {
      typedef std::shared_ptr<Message> Ptr;
      std::string roomId;
      std::string msg;
      rapidjson::Document json;
      size_t hash;
      MessageType type = Data;
      webrtc::DataBuffer webrtcbuffer;

      Message() : webrtcbuffer("") {}

      inline static Ptr Create() {
        return std::make_shared<Message>();
      }

      ~Message() {
      }
    };

    void getIceServerFromUrl(const std::string &url, std::string &outUrl, std::string &outUser, std::string &outPass);

    class DataChannelObserver : public webrtc::DataChannelObserver {
    public:
      DataChannelObserver(rtc::scoped_refptr<webrtc::DataChannelInterface> dataChannel, Peer *p)
          : m_dataChannel(
          dataChannel), _p(p) {
        isOpened = false;
        m_dataChannel->RegisterObserver(this);
      }

      ~DataChannelObserver() override {
        m_dataChannel->UnregisterObserver();
      }

      // DataChannelObserver interface
      void OnStateChange() override;

      void Send(std::string msg) {
        //std::cout << msg << std::endl;
        webrtc::DataBuffer buffer(msg.data());
        m_dataChannel->Send(buffer);
      }

      bool isOpened;

      void OnMessage(const webrtc::DataBuffer &buffer) override;

    protected:
      rtc::scoped_refptr<webrtc::DataChannelInterface> m_dataChannel;
      Peer *_p;
    };


    class DummySetSessionDescriptionObserver
        : public webrtc::SetSessionDescriptionObserver {
    public:
      static DummySetSessionDescriptionObserver *Create(Peer *p, const std::vector<scy::json::value> &iceCandidates) {
        auto observer = new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
        observer->_iceCandidates = iceCandidates;
        observer->_p = p;
        return observer;
      }

      static DummySetSessionDescriptionObserver *Create(Peer *p) {
        auto observer = new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
        observer->_p = p;
        return observer;
      }

      static DummySetSessionDescriptionObserver *Create() {
        return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
      }

      virtual void OnSuccess() override;

      virtual void OnFailure(const std::string &error) override;

    protected:
      std::vector<scy::json::value> _iceCandidates;
      Peer *_p = nullptr;

      DummySetSessionDescriptionObserver() = default;

      ~DummySetSessionDescriptionObserver() = default;
    };

    class Peer : public webrtc::PeerConnectionObserver,
                 public webrtc::CreateSessionDescriptionObserver {
    public:
      enum Mode {
        Offer, ///< Operating as offerer
        Answer ///< Operating as answerer
      };

      Peer(PeerManager *manager,
           PeerFactoryContext *context,
           const std::string &peerid,
           const std::string &token,
           Mode mode,
           const std::string &lastRoom,
           const std::vector<std::string> &ice_servers,
           int openPortsStart,
           int openPortsEnd);

      Peer(PeerManager *manager,
           PeerFactoryContext *context,
           const std::string &peerid,
           const std::string &token,
           Mode mode);

      virtual ~Peer();

      /// Create the local media stream.
      /// Only necessary when we are creating the offer.
      virtual rtc::scoped_refptr<webrtc::MediaStreamInterface> createMediaStream();

      /// Create the peer connection once configuration, constraints and
      /// streams have been created.
      virtual void createConnection();

      /// Close the peer connection.
      virtual void closeConnection();

      /// Create the offer SDP tos end to the peer.
      /// No offer should be received after creating the offer.
      /// A call to `recvSDP` with answer is expected in order to initiate the session.
      virtual void createOffer();

      /// Receive a remote offer or answer.
      virtual void recvSDP(const std::string &type, const std::string &sdp);

      virtual void
      recvSDP(const std::string &type, const std::string &sdp, const std::vector<scy::json::value> &iceCandidates);

      /// Receive a remote candidate.
      virtual void recvCandidate(const std::string &mid, int mlineindex, const std::string &sdp);

      /// Set a custom PeerFactory object.
      /// Must be set before any streams are initiated.
      void setPeerFactory(rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory);

      /// Set the port range for WebRTC connections.
      /// Must be done before the connection is initiated.
      void setPortRange(int minPort, int maxPort);

      std::string peerid() const;

      Mode mode() const { return _mode; }

      std::string token() const;

      std::string lastRoom() const;

      //webrtc::FakeConstraints &constraints();

      webrtc::PeerConnectionInterface::RTCOfferAnswerOptions &offerAnswerOptions();

      webrtc::PeerConnectionFactoryInterface *factory() const;

      rtc::scoped_refptr<webrtc::PeerConnectionInterface> peerConnection() const;

      rtc::scoped_refptr<webrtc::MediaStreamInterface> stream() const;

      DataChannelObserver *getDataChannel() const;

      PeerManager *manager() { return _manager; }

      std::vector<scy::json::value> remoteCandidates;

      std::atomic<bool> sentSDP = false;

      std::vector<webrtc::IceCandidateInterface *> &pendingCandidates() { return _pendingCandidates; }

    protected:

      void addGoogleStun();

      /// inherited from PeerConnectionObserver
      virtual void OnAddStream(webrtc::MediaStreamInterface *stream); ///< @deprecated
      virtual void OnRemoveStream(webrtc::MediaStreamInterface *stream); ///< @deprecated
      virtual void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
      override; ///< since 7f0676
      virtual void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
      override; ///< since 7f0676
      virtual void
      OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface>
                    channel) override; ///< since 7f0676
      virtual void OnIceCandidate(const webrtc::IceCandidateInterface *candidate)
      override;

      virtual void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state)
      override;

      virtual void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state)
      override;

      virtual void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state)
      override;

      virtual void OnRenegotiationNeeded();

      /// inherited from CreateSessionDescriptionObserver
      void OnSuccess(webrtc::SessionDescriptionInterface *desc)
      override;

      void OnFailure(const std::string &error)
      override;

      void AddRef() const
      override { return; }

      virtual rtc::RefCountReleaseStatus

      Release()
      const override { return rtc::RefCountReleaseStatus::kDroppedLastRef; }

    protected:
      std::vector<webrtc::IceCandidateInterface *> _pendingCandidates;
      PeerManager *_manager;
      PeerFactoryContext *_context;
      std::string _peerid;
      std::string _token;
      Mode _mode;
      webrtc::PeerConnectionInterface::RTCConfiguration _config;
      //webrtc::FakeConstraints _constraints;
      webrtc::PeerConnectionInterface::RTCOfferAnswerOptions _offerAnswerOptions;
      rtc::scoped_refptr<webrtc::PeerConnectionInterface> _peerConnection;
      rtc::scoped_refptr<webrtc::MediaStreamInterface> _stream;
      std::unique_ptr<cricket::BasicPortAllocator> _portAllocator;
      DataChannelObserver *m_localChannel;
      DataChannelObserver *m_remoteChannel;
      std::string _lastRoom;
      std::atomic<bool> destroying;
      std::atomic<bool> closing;
    };

  }
} // namespace scy::wrtc


#endif


/// @\}
