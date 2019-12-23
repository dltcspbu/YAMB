//
// Created by panda on 11/17/19.
//

#ifndef WEBRTCMESSAGEBROKER_SIGNALINGCONN_HPP
#define WEBRTCMESSAGEBROKER_SIGNALINGCONN_HPP

#include "../base/application.h"
#include "../base/ipc.h"
#include "../net/sslmanager.h"
#include "../net/sslsocket.h"
#include "../symple/client.h"
#include "../base/util.h"
#include "../base/filesystem.h"
#include "../base/idler.h"
#include <mutex>
#include <condition_variable>
#include <future>

using namespace scy;

class connSync {
public:
  bool _connected = false;
  std::condition_variable _cv;
  std::mutex _mut;

  void wait() {
    if (_connected)
      return;
    std::unique_lock<std::mutex> lk(_mut);
    while (!_connected) {
      _cv.wait(lk);
    }
  }

  void wakeup() {
    _connected = true;
    _cv.notify_all();
  }
};

struct SigMessage {
  std::string from;
  std::string roomId;
  scy::json::value data;
  scy::smpl::Peer peer;
};

class SignalingConn : public scy::Application {
public:
  typedef std::shared_ptr<SignalingConn> Ptr;

  explicit SignalingConn(const scy::smpl::Client::Options &);

  Signal<void(const std::vector<std::string> &)> IceServersIncome;
  Signal<void(const SigMessage &)> IceCandidateIncome;
  Signal<void(const SigMessage &)> SDPOfferIncome;
  Signal<void(const SigMessage &)> SDPAnswerIncome;
  Signal<void(SigMessage &)> PeerConnected;
  Signal<void(SigMessage &)> PeerDisconnected;

  void start();

  void close();

  void stopapp();

  void joinRoom(const std::string &roomId);

  void leaveRoom(const std::string &roomId);

  bool waitForOnline();

  void postMessage(const scy::smpl::Message &m);

  void sendPresence(scy::smpl::Address, const std::string &);

  uv::Loop *loop() { return Application::loop; }

private:
  void initIceServers(const scy::json::value &msg);

  void syncMessage(const scy::ipc::Action &action);

  void joinRoomSync(const scy::ipc::Action &action);

  void onClientStateChange(void *sender, scy::sockio::ClientState &state,
                           const scy::sockio::ClientState &oldState);

  void leaveRoomSync(const scy::ipc::Action &action);

  void onMessage(scy::smpl::Message &m);

  void onPeerConnected(scy::smpl::Peer &peer, const scy::json::value &msg);

  void onPeerDisconnected(scy::smpl::Peer &peer, const scy::json::value &msg);

  connSync _connSync;
  connSync _iceSync;
  std::string _socketId;
  std::atomic<bool> stopping;
  scy::smpl::Client::Options _options;
  scy::ipc::SyncQueue<> _ipc;
#if USE_SSL
  smpl::SSLClient _client;
#else
  scy::smpl::TCPClient _client;
#endif
};


#endif //WEBRTCMESSAGEBROKER_SIGNALINGCONN_HPP
