//
// Created by panda on 11/17/19.
//

#include "SignalingConn.hpp"

void SignalingConn::joinRoom(const std::string &roomId) {
  if (stopping.load())
    return;
  _ipc.push(new scy::ipc::Action(
      std::bind(&SignalingConn::joinRoomSync, this, std::placeholders::_1),
      new std::string(roomId)));
}

void SignalingConn::leaveRoom(const std::string &roomId) {
  if (stopping.load())
    return;
  _ipc.push(new ipc::Action(
      std::bind(&SignalingConn::leaveRoomSync, this, std::placeholders::_1),
      new std::string(roomId)));
}

void SignalingConn::leaveRoomSync(const scy::ipc::Action &action) {
  if (stopping.load())
    return;
  auto roomId = reinterpret_cast<std::string *>(action.arg);
  _client.leaveRoom(*roomId);
  delete roomId;
}

void SignalingConn::joinRoomSync(const scy::ipc::Action &action) {
  if (stopping.load())
    return;
  auto roomId = reinterpret_cast<std::string *>(action.arg);
  _client.joinRoom(*roomId);
  delete roomId;
}

bool SignalingConn::waitForOnline() {
  _connSync.wait();
  return true;
}

SignalingConn::SignalingConn(const scy::smpl::Client::Options &symple_options) : Application(scy::uv::createLoop()),
                                                                                 _ipc(Application::loop),
                                                                                 _client(symple_options,
                                                                                         Application::loop),
                                                                                 _options(symple_options),
                                                                                 stopping(false) {
  _client.StateChange += slot(this, &SignalingConn::onClientStateChange);
  _client.PeerConnected += slot(this, &SignalingConn::onPeerConnected);
  _client.PeerDisconnected += slot(this, &SignalingConn::onPeerDisconnected);
  _client += packetSlot(this, &SignalingConn::onMessage);
  _client.connect();
}

void SignalingConn::onClientStateChange(void *sender, scy::sockio::ClientState &state,
                                        const scy::sockio::ClientState &oldState) {
  LDebug("Client state changed from ", oldState, " to ", state)

  switch (state.id()) {
    case scy::sockio::ClientState::Connecting:
      break;
    case scy::sockio::ClientState::Connected:
      break;
    case scy::sockio::ClientState::Online:
      _socketId = _client.ourID();
      _connSync.wakeup();
      std::cout << "Going online rooms number: " << _client.rooms().size() << std::endl;
      for (const auto &r: _client.rooms()) {
        this->joinRoom(r);
      }
      break;
    case scy::sockio::ClientState::Error:
      _connSync._connected = false;
      break;
  }
}

void SignalingConn::syncMessage(const scy::ipc::Action &action) {
  if (stopping.load())
    return;
  auto m = reinterpret_cast<smpl::Message *>(action.arg);
  _client.send(*m);
  delete m;
}

void SignalingConn::postMessage(const scy::smpl::Message &m) {
  if (stopping.load())
    return;
  _ipc.push(new ipc::Action(
      std::bind(&SignalingConn::syncMessage, this, std::placeholders::_1),
      m.clone()));
}

void SignalingConn::onMessage(scy::smpl::Message &m) {
  if (stopping.load())
    return;
  auto peerId = m.from().user;
  std::string room;

  if (m.find("roomId") != m.end()) {
    room = m["roomId"].get<std::string>();
  } else
    room = "system";

  peerId += room;
  SigMessage sm;
  sm.from = std::move(peerId);
  sm.roomId = std::move(room);
  if (m.find("candidate") != m.end()) {
    sm.data = m["candidate"].get<scy::json::value>();
    IceCandidateIncome.emit(sm);
  } else if (m.find("offer") != m.end()) {
    _iceSync.wait();
    sm.data = m["offer"].get<scy::json::value>();
    SDPOfferIncome.emit(sm);
  } else if (m.find("answer") != m.end()) {
    _iceSync.wait();
    sm.data = m["answer"].get<scy::json::value>();
    SDPAnswerIncome.emit(sm);
  }
}

void SignalingConn::initIceServers(const scy::json::value &msg) {
  if (msg.find("type") != msg.end()) {
    auto t = msg["type"].get<std::string>();
    if (t == "response") {

      assert(msg.find("data") != msg.end() && "data should be in JSON from signaller");
      assert(msg["data"].find("iceServers") != msg["data"].end() && "iceServers should be in JSON from signaller");

      auto iceServers = msg["data"]["iceServers"];
      std::vector<std::string> ice_servers;
      for (auto iceServer : iceServers) {
        auto type = iceServer["type"].get<std::string>();
        auto host = iceServer["host"].get<std::string>();
        auto port = iceServer["port"].get<std::string>();
        std::string iceurl = type + ":";
        if (iceServer.find("credentials") != iceServer.end()) {
          auto username = iceServer["credentials"]["username"].get<std::string>();
          auto password = iceServer["credentials"]["password"].get<std::string>();
          iceurl.append(username).append(":").append(password).append("@");
        }

        iceurl.append(host).append(":").append(port);

        ice_servers.push_back(iceurl);
      }
      IceServersIncome.emit(ice_servers);
      _iceSync.wakeup();
    }
  }
}

void SignalingConn::stopapp() {
  _ipc.close();
  Application::stop();
  Application::finalize();
}

void SignalingConn::start() {
  waitForShutdown([](void *opaque) {
    reinterpret_cast<SignalingConn *>(opaque)->stopapp();
  }, this);
}

void SignalingConn::onPeerDisconnected(scy::smpl::Peer &peer, const scy::json::value &msg) {
  if (stopping.load())
    return;
  auto peerId = peer.user();

  if (msg.find("roomId") == msg.end())
    return;

  SigMessage sm;
  sm.roomId = msg["roomId"].get<std::string>();
  sm.from = peerId + sm.roomId;
  sm.peer = peer;

  PeerDisconnected.emit(sm);
}

void SignalingConn::onPeerConnected(scy::smpl::Peer &peer, const scy::json::value &msg) {
  if (stopping.load())
    return;
  auto peerId = peer.user();
  if (peerId == _options.user)
    initIceServers(msg);
  _iceSync.wait();
  auto p = msg.dump();
  if (msg.find("roomId") == msg.end())
    return;

  SigMessage sm;
  sm.roomId = msg["roomId"].get<std::string>();
  sm.from = peerId + sm.roomId;
  sm.peer = peer;
  PeerConnected.emit(sm);
}

void SignalingConn::close() {
  stopping.store(true);
  _client.close();
}

void SignalingConn::sendPresence(smpl::Address a, const std::string &roomId) {
  if (stopping.load())
    return;
  _client.sendPresence(a, roomId, false);
}
