///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier:	LGPL-2.1+
//
///


#ifndef SCY_WebRTC_WebRTCStreamer_WebrtcNode_H
#define SCY_WebRTC_WebRTCStreamer_WebrtcNode_H


#include "../base/application.h"
#include "../base/ipc.h"
#include "../net/sslmanager.h"
#include "../net/sslsocket.h"
#include "../symple/client.h"
#include "../base/util.h"
#include "../base/filesystem.h"
#include "../base/idler.h"
#include "config.h"
#include <future>
#include "SCYExtension/Source.h"
#include "SCYExtension/RoomManager.h"
#include "messageBroker.hpp"
#include <shared_mutex>
#include <Hasher.h>
#include "SignalingConn.hpp"

using namespace scy;
using namespace wrtc;

struct config {
  scy::smpl::Client::Options simple_options;
  std::string sqlite3Path;
  std::string outputStream;
  std::vector<std::string> ice_servers;
  int openPortsStart;
  int openPortsEnd;
  std::string hashSeed;
};

struct sendMsgT {
  enum {
    direct, broadcast
  } type;
  std::string except;
  scy::json::value msg;
};

class WebrtcNode : public scy::wrtc::PeerManager {
public:

  void stopapp();

  WebrtcNode(const config &conf);

  ~WebrtcNode();

  config conf() const { return _conf; }

  config conf() { return _conf; }

  void addCallback(const std::string &roomId, messageHandler *callback);

  bool removeCallback(const std::string &roomId, messageHandler *callback);

  void addCallback(const std::string &roomId, std::shared_ptr<messageHandler> callback);

  bool removeCallback(const std::string &roomId, std::shared_ptr<messageHandler> callback);

  bool broadcast2Room(const std::string &roomId, const std::string &j, MessageType t = MessageType::Logic,
                      const std::string &except = "");

  bool send2client(const std::string &peerId, const std::string &roomId, std::string &&j,
                   MessageType t = MessageType::Logic);

  bool send2client(const std::string &peerId, const std::string &roomId, const std::string &j,
                   MessageType t = MessageType::Logic);

  std::vector<std::shared_ptr<messageHandler>> *getCallbacks(const std::string &roomId) {
    return _callbacks.get(roomId);
  }

  void joinRoom(const std::string &roomId);

  void leaveRoom(const std::string &roomId);

  void processWebRTCMessage(const Message::Ptr) override;

  std::string myIdInRoom(const std::string &roomId) const {
    assert(!_conf.simple_options.user.empty());
    return _conf.simple_options.user + roomId;
  }

  void start() {
    auto webrtcThread = rtc::Thread::Current();
    _idler = new scy::Idler(_loop, [=]() {
      if (webrtcThread)
        webrtcThread->ProcessMessages(1);
    });
    _sigConn->start();
  }

  std::vector<std::string> getRoomIds();

  std::vector<std::string> getNodeIds(const std::string &roomId);

protected:

  bool isEventLoopThread() {
    return _tid == std::this_thread::get_id();
  }

  std::size_t
  formMessage(scy::json::value &result, std::string &data, MessageType t, const std::string &roomId,
              const std::string &peerId);

  std::size_t
  formMessage(scy::json::value &result, std::string &data, MessageType t, const std::string &roomId);

  std::size_t
  formMessage(scy::json::value &result, const std::string &data, MessageType t, const std::string &roomId,
              const std::string &peerId);

  std::size_t
  formMessage(scy::json::value &result, const std::string &data, MessageType t, const std::string &roomId);

  bool broadcast2Room(const std::string &roomId, const scy::json::value &j, MessageType t = MessageType::Logic,
                      const std::string &except = "");

  bool send2client(const std::string &peerId, const std::string &roomId, const scy::json::value &j,
                   MessageType t = MessageType::Logic);

  void OnIncomingNodeMapDifference(const Message::Ptr);

  void handleWebrtcNodeMessageSystem(const Message::Ptr);

  void handleWebrtcNodeMessageLogic(const Message::Ptr);

  void joinRoomWS(const std::string &roomId);

  void leaveRoomWS(const std::string &roomId);

  void onAnnounceWebRTC(const Message::Ptr) override;

  void onCreatedConnection(const std::string &, const std::string &) override;

  bool resend2Room(const std::string &roomId, const std::string &msg, std::string_view except);

  bool createPC(const std::string &peerid, const scy::wrtc::Peer::Mode &sdptype, const std::string &roomId);

  bool createPC(scy::smpl::Peer &peer, std::string &roomId);

  void onIceServer(const std::vector<std::string> &);

  void onIceCandidate(const SigMessage &m);

  void onSDPOffer(const SigMessage &m);

  void onSDPAnswer(const SigMessage &m);

  /// PeerManager interface
  void sendSDP(scy::wrtc::Peer *conn, const std::string &type, const std::string &sdp) override;

  void sendCandidate(scy::wrtc::Peer *conn, const std::string &mid, int mlineindex, const std::string &sdp) override;

  void onAddRemoteStream(scy::wrtc::Peer *conn, webrtc::MediaStreamInterface *stream) override;

  void onRemoveRemoteStream(scy::wrtc::Peer *conn, webrtc::MediaStreamInterface *stream) override;

  void onStable(scy::wrtc::Peer *conn) override;

  void onClosed(scy::wrtc::Peer *conn) override;

  void onFailure(scy::wrtc::Peer *conn, const std::string &error) override;

  void onPeerConnectedSignaler(SigMessage &);

  void onPeerDisconnectedSignaler(SigMessage &);

  void sendNodeMap(const std::string &roomId, const std::string &fromWhomUpdate, std::shared_ptr<NodesMapDiff> &diff);

  void updateNodeMap(const std::string &roomId, const std::string &fromWhomUpdate, const rapidjson::Value &diff);

protected:
  SignalingConn::Ptr _sigConn;
  scy::ipc::SyncQueue<> _ipc;
  scy::uv::Loop *_loop;
  PointerCollection<std::string, std::vector<std::shared_ptr<messageHandler>>> _callbacks;
  scy::wrtc::PeerFactoryContext *_context;
  std::thread::id _tid;
  std::string _myid;
  NodeRole *_myRole;
  RoomManager *_roomManager;
  scy::Idler *_idler;
  Hasher _hasher;
  config _conf;
  std::vector<std::string> rooms;
  TSDeque<sendMsgT*> _sMq;
  std::condition_variable _cvsMq;
  std::thread message_sender;
  scy::PointerCollection<std::string, std::shared_timed_mutex> _callbacks_mutexes;
  scy::PointerCollection<std::string, std::shared_timed_mutex> _PeersInLvl_mutexes;
  std::atomic<bool> stopped;
  std::atomic<bool> stopping;
};


#endif
