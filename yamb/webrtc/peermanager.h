///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier:	LGPL-2.1+
//
/// @addtogroup webrtc
/// @{


#ifndef SCY_WebRTC_PeerManager_H
#define SCY_WebRTC_PeerManager_H


#include "../base/collection.h"
#include "../json/json.h"
#include "webrtc.h"
#include "peer.h"

//#include "api/peerconnectioninterface.h"
#include "api/peer_connection_interface.h"


#include <iostream>
#include <string>


namespace scy {
  namespace wrtc {


    class Peer;


    class PeerManager : public PointerCollection<std::string, Peer> {
    public:
      PeerManager();

      virtual ~PeerManager();

      virtual void sendSDP(Peer *conn, const std::string &type, const std::string &sdp);

      virtual void sendCandidate(Peer *conn, const std::string &mid, int mlineindex, const std::string &sdp);

      virtual void recvSDP(const std::string &token, const json::value &data);

      virtual void
      recvSDP(const std::string &token, const json::value &message, const std::vector<scy::json::value> &iceCandidates);

      virtual void recvCandidate(const std::string &token, const json::value &data);

      virtual void onAddRemoteStream(Peer *conn, webrtc::MediaStreamInterface *stream);

      virtual void onRemoveRemoteStream(Peer *conn, webrtc::MediaStreamInterface *stream);

      virtual void onStable(Peer *conn);

      virtual void onClosed(Peer *conn);

      virtual void onFailure(Peer *conn, const std::string &error);

      virtual void onAnnounceWebRTC(const Message::Ptr) = 0;

      virtual void onCreatedConnection(const std::string &, const std::string &) = 0;

      virtual void processWebRTCMessage(const Message::Ptr) = 0;
    };


  }
} // namespace scy::wrtc


#endif


/// @\}
