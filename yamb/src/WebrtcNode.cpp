///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier:	LGPL-2.1+
//
///

#include <iostream>
#include <string>

#include "../webrtc/videopacketsource.h"
#include "WebrtcNode.h"

#include "api/media_stream_track_proxy.h"


using std::endl;

WebrtcNode::WebrtcNode(const config &conf)
    : _sigConn(std::make_shared<SignalingConn>(conf.simple_options)),
      _loop(_sigConn->loop()),
      _ipc(_sigConn->loop()),
      stopped(false),
      stopping(false),
      _hasher(conf.hashSeed),
      _conf(conf) {
  // Setup the signalling client

  //Logger::instance().add(new ConsoleChannel("debug", Level::Debug));
  rtc::LogMessage::LogToDebug(rtc::LERROR); // LS_VERBOSE, LS_INFO, LERROR
  rtc::LogMessage::LogTimestamps();
  rtc::LogMessage::LogThreads();

  _roomManager = new RoomManager();
  _myRole = new Source(_conf.simple_options.user, _conf.simple_options.name);

  _context = new wrtc::PeerFactoryContext();
  if (_conf.openPortsStart && _conf.openPortsEnd) {
    _context->initCustomNetworkManager();
  }

  //symple init and callbacks
  _sigConn->IceServersIncome += slot(this, &WebrtcNode::onIceServer);
  _sigConn->IceCandidateIncome += slot(this, &WebrtcNode::onIceCandidate);
  _sigConn->SDPOfferIncome += slot(this, &WebrtcNode::onSDPOffer);
  _sigConn->SDPAnswerIncome += slot(this, &WebrtcNode::onSDPAnswer);
  _sigConn->PeerConnected += slot(this, &WebrtcNode::onPeerConnectedSignaler);
  _sigConn->PeerDisconnected += slot(this, &WebrtcNode::onPeerDisconnectedSignaler);

  message_sender = std::thread([this]() {
    std::mutex m;
    while (true) {
      std::unique_lock<std::mutex> lk(m);
      while (_sMq.isEmpty()) {
        if (stopping.load())
          return;
        _cvsMq.wait(lk);
      }
      if (stopping.load())
        return;
      auto smT = _sMq.get();
      auto ms = smT->msg.dump();
      const auto RU = _roomManager->get(smT->msg["roomId"].get<std::string>());
      if (smT->type == sendMsgT::broadcast) {
        const auto &peers_in_room = RU->PeersInRoom;
        for (auto &peer_it: peers_in_room) {
          if (!smT->except.empty() && peer_it.first == smT->except)
            continue;
          auto p = peer_it.second;
          if (!p) {
            continue;
          }
          auto d = p->getDataChannel();
          if (!d) {
            auto newsmT = new sendMsgT;
            newsmT->type = sendMsgT::direct;
            newsmT->msg = smT->msg;
            newsmT->msg["to"] = p->peerid();
            _sMq.push(newsmT);
            continue;
          }
          d->Send(ms);
        }
        delete smT;
      } else {
        if (RU->PeersInRoom.find(smT->msg["to"].get<std::string>()) != RU->PeersInRoom.end()) {
          const auto p = RU->PeersInRoom[smT->msg["to"].get<std::string>()];
          if (p) {
            auto d = p->getDataChannel();
            if (!d) {
              _sMq.push(smT);
            } else {
              d->Send(ms);
              delete smT;
            }
          }
        }
      }
    }
  });
}

void WebrtcNode::processWebRTCMessage(const Message::Ptr pMsg) {
  pMsg->msg = {(const char *) pMsg->webrtcbuffer.data.data(), pMsg->webrtcbuffer.data.size()};

  if (pMsg->json.ParseInsitu<rapidjson::kParseStopWhenDoneFlag>(
      (char *) pMsg->webrtcbuffer.data.data()).HasParseError()) {
    std::cout << "error parsing processing: " << pMsg->json.GetParseError() << " "
              << pMsg->webrtcbuffer.data.data()
              << std::endl;
    return;
  }
  if (pMsg->json.HasMember("roomId") &&
      pMsg->json.HasMember("hash") &&
      pMsg->json.HasMember("type")) {
    pMsg->roomId = pMsg->json.FindMember("roomId")->value.GetString();
    pMsg->hash = size_t(pMsg->json.FindMember("hash")->value.GetUint64());
    pMsg->type = MessageType(pMsg->json.FindMember("type")->value.GetInt());
  } else {
    pMsg->roomId = "system";
    pMsg->hash = 0;
    pMsg->type = MessageType::Unknown;
  }

  LDebug("Processing WebrtcNode message: ", pMsg->json["data"].GetString())

  if (pMsg->type != MessageType::Logic) {
    handleWebrtcNodeMessageSystem(pMsg);
    return;
  }
  handleWebrtcNodeMessageLogic(pMsg);
}


void WebrtcNode::handleWebrtcNodeMessageSystem(const Message::Ptr msgObj) {
  auto roomId = msgObj->roomId;
  if (!_roomManager->exists(roomId))
    return;
  std::cout << "OK" << std::endl;
  const auto &RU = _roomManager->get(roomId);
  auto hash = msgObj->hash;

  if (RU->SystemMsgHashesBuffer.isExists(hash))
    return;
  RU->SystemMsgHashesBuffer.put(hash);

  auto type = msgObj->type;

  switch (type) {
    case MessageType::AnnounceWebrtc:
      onAnnounceWebRTC(msgObj);
      break;
    case MessageType::UpdateMap:
      OnIncomingNodeMapDifference(msgObj);
      break;
    default:
      std::cout << "Unknown System Message came from WebrtcConnection in room=" + roomId << ". With message: "
                << msgObj->msg << std::endl;
      break;
  }
}

void WebrtcNode::handleWebrtcNodeMessageLogic(const Message::Ptr pMsg) {
  if (!_roomManager->exists(pMsg->roomId))
    return;

  const auto &RU = _roomManager->get(pMsg->roomId);
  //auto msgstr = pMsg->msg;
  auto hash = pMsg->hash;
  if (RU->LogicMsgHashesBuffer.isExists(hash)) {
    return;
  }

  RU->LogicMsgHashesBuffer.put(hash);

  if (pMsg->json.HasMember("to")) {
    auto to = pMsg->json["to"].GetString();
    if (to != myIdInRoom(pMsg->roomId)) {
      auto sendMsgTask = new sendMsgT;
      sendMsgTask->type = sendMsgT::direct;
      formMessage(sendMsgTask->msg, pMsg->json["data"].GetString(), MessageType(pMsg->json["type"].GetInt()),
                  pMsg->roomId, to);
      sendMsgTask->msg["hash"] = pMsg->json["hash"].GetUint64();
      _sMq.push(sendMsgTask);
      return;
    }
  } else {
    auto sendMsgTask = new sendMsgT;
    sendMsgTask->type = sendMsgT::broadcast;
    formMessage(sendMsgTask->msg, pMsg->json["data"].GetString(), MessageType(pMsg->json["type"].GetInt()),
                pMsg->roomId);
    sendMsgTask->msg["hash"] = pMsg->json["hash"].GetUint64();
    _sMq.push(sendMsgTask);
  }

  std::vector<std::shared_ptr<messageHandler>> call_vec;
  {
    std::shared_timed_mutex *m = _callbacks_mutexes.get(pMsg->roomId);
    std::shared_lock<std::shared_timed_mutex> lock_read_callback_vec(*m);
    const auto &callbacks = getCallbacks(pMsg->roomId);
    call_vec.assign(callbacks->begin(), callbacks->end());
  }
  for (auto &callback : call_vec) {
    callback->handle(pMsg->msg);
  }
}

void WebrtcNode::OnIncomingNodeMapDifference(const Message::Ptr WebRTCMsg) {
  const auto &roomId = WebRTCMsg->roomId;
  const auto &from = WebRTCMsg->json["from"].GetString();
  assert(WebRTCMsg->json.HasMember("roomId") &&
         "roomId field should be presented in AnnounceWebrtc json");

  rapidjson::Document data;
  if (data.Parse(WebRTCMsg->json["data"].GetString()).HasParseError()) {
    std::cout << "error parsing:" << WebRTCMsg->json["data"].GetString() << std::endl;
    return;
  }
  updateNodeMap(roomId, from, data);
}

void
WebrtcNode::updateNodeMap(const std::string &roomId, const std::string &fromWhomUpdate, const rapidjson::Value &diff) {
  auto diffMap = createNodesMapDiff();
  std::lock_guard<std::shared_timed_mutex> lk(*_PeersInLvl_mutexes.get(roomId));
  const auto &RU = _roomManager->get(roomId);
  const auto &myId = myIdInRoom(roomId);
  if (diff.HasMember("connectedBylvls") && !diff["connectedBylvls"].IsNull())
    createDiffConnectedBylvls(diffMap, RU, diff["connectedBylvls"]);

  if (diff.HasMember("disconnectedBylvls") && !diff["disconnectedBylvls"].IsNull())
    createDiffDisconnectedBylvls(diffMap, RU, diff["disconnectedBylvls"]);

  if (diffMap->connectedBylvls.empty() && diffMap->disconnectedBylvls.empty())
    return;

  OnNodesMapDiff(RU, myId, diffMap);
  OnNodesMapDiffJson(RU, myId, diffMap);
  sendNodeMap(roomId, fromWhomUpdate, diffMap);
}

std::size_t
WebrtcNode::formMessage(scy::json::value &result, std::string &data, MessageType t, const std::string &roomId,
                        const std::string &peerId) {
  result["roomId"] = roomId;
  result["from"] = myIdInRoom(roomId);
  result["to"] = peerId;
  result["type"] = t;
  auto msgId = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
  ).count();
  result["msgId"] = msgId;
  std::size_t hash = _hasher(data + std::to_string(msgId) + myIdInRoom(roomId));
  result["data"] = std::move(data);
  result["hash"] = hash;
  return hash;
}

std::size_t
WebrtcNode::formMessage(scy::json::value &result, const std::string &data, MessageType t, const std::string &roomId,
                        const std::string &peerId) {
  result["roomId"] = roomId;
  result["from"] = myIdInRoom(roomId);
  result["to"] = peerId;
  result["type"] = t;
  auto msgId = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
  ).count();
  result["msgId"] = msgId;
  std::size_t hash = _hasher(data + std::to_string(msgId) + myIdInRoom(roomId));
  result["data"] = data;
  result["hash"] = hash;
  return hash;
}

std::size_t
WebrtcNode::formMessage(scy::json::value &result, const std::string &data, MessageType t, const std::string &roomId) {
  result["roomId"] = roomId;
  result["from"] = myIdInRoom(roomId);
  result["type"] = t;
  auto msgId = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
  ).count();
  result["msgId"] = msgId;
  std::size_t hash = _hasher(data + std::to_string(msgId) + myIdInRoom(roomId));
  result["data"] = data;
  result["hash"] = hash;
  return hash;
}

std::size_t
WebrtcNode::formMessage(scy::json::value &result, std::string &data, MessageType t, const std::string &roomId) {
  result["roomId"] = roomId;
  result["from"] = myIdInRoom(roomId);
  result["type"] = t;
  auto msgId = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
  ).count();
  result["msgId"] = msgId;
  std::size_t hash = _hasher(data + std::to_string(msgId) + myIdInRoom(roomId));
  result["data"] = std::move(data);
  result["hash"] = hash;
  return hash;
}

bool WebrtcNode::send2client(const std::string &peerId, const std::string &roomId, std::string &&j,
                             MessageType t) {
//  _ipc.push(new ipc::Action(
//      [roomId, peerId, t, this](const scy::ipc::Action &action) {
  if (!_roomManager->exists(roomId))
    return false;
  const auto &RU = _roomManager->get(roomId);
  scy::json::value msg;
  //auto j = reinterpret_cast<std::string *>(action.arg);
  std::size_t hash = formMessage(msg, j, t, roomId, peerId);
  if (t == MessageType::Logic) {
    RU->LogicMsgHashesBuffer.put(hash);
  } else {
    RU->SystemMsgHashesBuffer.put(hash);
  }

  auto sendMsgTask = new sendMsgT;
  sendMsgTask->type = PeerManager::exists(peerId) ? sendMsgT::direct : sendMsgT::broadcast;
  sendMsgTask->msg = std::move(msg);
  _sMq.push(sendMsgTask);
  _cvsMq.notify_all();
//  if (PeerManager::exists(peerId)) {
//    auto p = PeerManager::get(peerId) ;
//    auto d = p->getDataChannel();
//    if (d)
//      d->Send(msg.dump());
//  } else {
//    std::vector<std::string> chainIds;
//    auto node = findInGraphWithRememberingDirectConnections(RU->IamRoot, peerId, chainIds);
//    if (node && !chainIds.empty()) {
//      for (const auto &directId: chainIds) {
//        send2client(directId, roomId, j, t);
//      }
//    } else {
//      LDebug("There is no node with given ID in connections graph", peerId);
//    }
//  }
//        delete j;
//      }, new std::string(std::move(j))));
  return true;
}


bool WebrtcNode::send2client(const std::string &peerId, const std::string &roomId, const std::string &j,
                             MessageType t) {
//  _ipc.push(new ipc::Action(
//      [roomId, peerId, t, this](const scy::ipc::Action &action) {
  if (!_roomManager->exists(roomId))
    return false;
  const auto &RU = _roomManager->get(roomId);
  scy::json::value msg;
  // auto j = reinterpret_cast<std::string *>(action.arg);
  std::size_t hash = formMessage(msg, j, t, roomId, peerId);
  if (t == MessageType::Logic) {
    RU->LogicMsgHashesBuffer.put(hash);
  } else {
    RU->SystemMsgHashesBuffer.put(hash);
  }

  auto sendMsgTask = new sendMsgT;
  sendMsgTask->type = PeerManager::exists(peerId) ? sendMsgT::direct : sendMsgT::broadcast;
  sendMsgTask->msg = std::move(msg);
  _sMq.push(sendMsgTask);
  _cvsMq.notify_all();
//  if (PeerManager::exists(peerId)) {
//    auto p = PeerManager::get(peerId);
//    auto d = p->getDataChannel();
//    if (d)
//      d->Send(msg.dump());
//  } else {
//    std::vector<std::string> chainIds;
//    auto node = findInGraphWithRememberingDirectConnections(RU->IamRoot, peerId, chainIds);
//    if (node && !chainIds.empty()) {
//      for (const auto &directId: chainIds) {
//        send2client(directId, roomId, j, t);
//      }
//    } else {
//      LDebug("There is no node with given ID in connections graph", peerId);
//    }
//  }
//        delete j;
//      }, new std::string(j)));
  return true;
}

void WebrtcNode::onCreatedConnection(const std::string &peerId, const std::string &roomId) {
  scy::json::value sendingData;
  auto m = _PeersInLvl_mutexes.get(roomId);
  std::shared_lock<std::shared_timed_mutex> lk_write(*m);
  const auto &RU = _roomManager->get(roomId);
  sendingData["connectionsMap"] = {{"connectedBylvls",    RU->NodesMap->Data},
                                   {"disconnectedBylvls", {}}};
  auto pp = RU->NodesMap->Data.dump();
  sendingData["nPeersInRoom"] = RU->nPeersInRoom;
  send2client(peerId, roomId, sendingData.dump(), MessageType::AnnounceWebrtc);
}

void WebrtcNode::onAnnounceWebRTC(const Message::Ptr WebRTCMsg) {
  const auto &roomId = WebRTCMsg->roomId;

  assert(WebRTCMsg->json.HasMember("roomId") &&
         "roomId field should be presented in AnnounceWebrtc json");

  const auto &from = WebRTCMsg->json["from"].GetString();

  rapidjson::Document data;
  if (data.Parse(WebRTCMsg->json["data"].GetString()).HasParseError()) {
    std::cout << "error parsing announce data:" << WebRTCMsg->json["data"].GetString() << std::endl;
    return;
  }

  assert(data.HasMember("connectionsMap") &&
         "connectionsMap field should be presented in AnnounceWebrtc json[\"data\"]");

  const auto &incomingNodesMap = data["connectionsMap"];
  updateNodeMap(roomId, from, incomingNodesMap);
}

void WebrtcNode::sendNodeMap(const std::string &roomId, const std::string &fromWhomUpdate,
                             std::shared_ptr<NodesMapDiff> &diff) {
  scy::json::value diffJson = *diff;
  for (const auto &v: diff->connectedBylvls) {
    for (const auto &vv: v.second) {
      assert(!vv->ParentIds.empty());
    }
  }
  const std::string &msg = diffJson.dump();
  broadcast2Room(roomId, msg, MessageType::UpdateMap, fromWhomUpdate);
}


bool
WebrtcNode::broadcast2Room(const std::string &roomId, const scy::json::value &j, MessageType t,
                           const std::string &except) {
  return true;
}

bool WebrtcNode::send2client(const std::string &peerId, const std::string &roomId, const scy::json::value &j,
                             MessageType t) {
  return true;
}

bool WebrtcNode::resend2Room(const std::string &roomId, const std::string &msg, std::string_view except) {
  if (!_roomManager->exists(roomId))
    return false;

  const auto &RU = _roomManager->get(roomId);
  const auto &peers_in_room = RU->PeersInRoom;
  for (auto &peer_it: peers_in_room) {
    if (peer_it.first == except)
      continue;
    auto p = peer_it.second;
    if (!p)
      continue;
    auto d = p->getDataChannel();
    if (!d)
      continue;
    d->Send(msg);
  }

  return true;
}

WebrtcNode::~WebrtcNode() {
  if (!stopped.load()) {
    stopapp();
  }

  if (message_sender.joinable())
    message_sender.join();

  delete _roomManager;
  delete _context;
  delete _myRole;
  delete _idler;

  std::cout << "destructed" << std::endl;
}

void WebrtcNode::stopapp() {
  stopping.store(true);
  _cvsMq.notify_all();
  std::cout << "here" << std::endl;
//  const auto pm = PeerManager::map();
//  std::vector<std::string> keys;
//  for (auto &pm_v : pm) {
//    keys.push_back(pm_v.first);
//  }
//  for (auto key:keys) {
//    auto p = PeerManager::remove(key);
//    if (p)
//      delete p;
//  }

  PeerManager::clear();
  _sigConn->close();
  std::cout << "here0" << std::endl;
  auto b = new std::atomic<bool>(false);
  std::condition_variable *cv = new std::condition_variable;
  std::mutex *m = new std::mutex;

  std::future<void> promise = std::async(std::launch::async, [this, b, cv, m]() {
    std::unique_lock lk(*m);
    while (!b->load()) {
      cv->wait(lk);
    }
  });


  std::cout << "here2" << std::endl;

  _idler->cancel();


  std::cout << "here3" << std::endl;
  _ipc.push(new ipc::Action(
      [b, cv, this](const scy::ipc::Action &action) {
        _context->cleanup();
        b->store(true);
        cv->notify_all();
      }));
  std::cout << "here4" << std::endl;
  promise.wait();
  std::cout << "here5" << std::endl;
  _sigConn->stopapp();
  std::cout << "here6" << std::endl;
  stopped.store(true);
  std::cout << "here7" << std::endl;
  _cvsMq.notify_all();
  cv->notify_all();
}


void WebrtcNode::addCallback(const std::string &roomId, messageHandler *callback) {
  std::shared_timed_mutex *m = _callbacks_mutexes.get(roomId);
  if (!_callbacks.exists(roomId)) {
    _callbacks.add(roomId, new std::vector<std::shared_ptr<messageHandler>>{
        std::shared_ptr<messageHandler>(callback)});
  } else {
    std::lock_guard<std::shared_timed_mutex> lk(*m);
    _callbacks.get(roomId)->push_back(std::shared_ptr<messageHandler>(callback));
  }
}

bool WebrtcNode::removeCallback(const std::string &roomId, messageHandler *callback) {
  if (!_callbacks_mutexes.exists(roomId))
    return false;
  std::shared_timed_mutex *m = _callbacks_mutexes.get(roomId);
  if (_callbacks.exists(roomId)) {
    std::lock_guard<std::shared_timed_mutex> lk(*m);
    _callbacks.get(roomId)->erase(std::remove_if(_callbacks.get(roomId)->begin(), _callbacks.get(roomId)->end(),
                                                 [&callback](std::shared_ptr<messageHandler> &x) -> bool {
                                                   return x.get() == callback;
                                                 }), _callbacks.get(roomId)->end());
    return true;
  }
  return true;
}


void WebrtcNode::addCallback(const std::string &roomId, std::shared_ptr<messageHandler> callback) {
  std::shared_timed_mutex *m = _callbacks_mutexes.get(roomId);
  if (!_callbacks.exists(roomId)) {
    _callbacks.add(roomId, new std::vector<std::shared_ptr<messageHandler>>{
        callback});
  } else {
    std::lock_guard<std::shared_timed_mutex> lk(*m);
    _callbacks.get(roomId)->push_back(callback);
  }
}

bool WebrtcNode::removeCallback(const std::string &roomId, std::shared_ptr<messageHandler> callback) {
  if (!_callbacks_mutexes.exists(roomId))
    return false;
  std::shared_timed_mutex *m = _callbacks_mutexes.get(roomId);
  if (_callbacks.exists(roomId)) {
    std::lock_guard<std::shared_timed_mutex> lk(*m);
    _callbacks.get(roomId)->erase(std::remove_if(_callbacks.get(roomId)->begin(), _callbacks.get(roomId)->end(),
                                                 [&callback](std::shared_ptr<messageHandler> &x) -> bool {
                                                   return x.get() == callback.get();
                                                 }), _callbacks.get(roomId)->end());
    return true;
  }
  return true;
}

void WebrtcNode::sendSDP(scy::wrtc::Peer *conn, const std::string &type, const std::string &sdp) {
  assert(type == "offer" || type == "answer");
  scy::smpl::Message m;
  scy::json::value desc;
  desc[scy::wrtc::kSessionDescriptionTypeName] = type;
  desc[scy::wrtc::kSessionDescriptionSdpName] = sdp;
  m[type] = desc;
  if (!conn->remoteCandidates.empty())
    m["candidates"] = conn->remoteCandidates;
  m["to"] = conn->peerid();
  m["roomId"] = conn->lastRoom();
  if (stopping.load())
    return;
  _sigConn->postMessage(m);
  conn->sentSDP.store(true);
}

void WebrtcNode::sendCandidate(scy::wrtc::Peer *conn, const std::string &mid, int mlineindex, const std::string &sdp) {
  scy::json::value desc;
  desc[scy::wrtc::kCandidateSdpMidName] = mid;
  desc[scy::wrtc::kCandidateSdpMlineIndexName] = mlineindex;
  desc[scy::wrtc::kCandidateSdpName] = sdp;
  if (conn->sentSDP.load()) {
    scy::smpl::Message m;
    m["candidate"] = desc;
    m["to"] = conn->peerid();
    m["roomId"] = conn->lastRoom();
    m["sdptype"] = conn->mode();
    if (stopping.load())
      return;
    _sigConn->postMessage(m);
  } else {
    std::cout << desc.dump() << std::endl;
    conn->remoteCandidates.emplace_back(desc);
  }
}

bool WebrtcNode::createPC(const std::string &peerid, const scy::wrtc::Peer::Mode &sdptype, const std::string &roomId) {
  scy::wrtc::Peer *conn = nullptr;
  bool result;
  bool exists = scy::wrtc::PeerManager::exists(peerid);
  if (!exists) {
    conn = new wrtc::Peer(this, _context, peerid, "", sdptype, roomId, _conf.ice_servers,
                          _conf.openPortsStart, _conf.openPortsEnd);
    conn->createConnection();
  } else {
    conn = scy::wrtc::PeerManager::get(peerid);
    conn->lastRoom() = roomId;
    if (conn->mode() != sdptype)
      return false;
  }

  if (!exists) {
    result = scy::wrtc::PeerManager::add(peerid, conn);
  } else result = true;

  std::lock_guard<std::shared_timed_mutex> lk(*_PeersInLvl_mutexes.get(roomId));
  const auto &RU = _roomManager->get(roomId);
//
//  auto diff = createNodesMapDiff();
//  auto diffUnit = std::make_shared<NodesMapDiffUnit>();
//  diffUnit->Id = peerid;
//  diff->connectedBylvls = {{0, {diffUnit}}};
//  diff->disconnectedBylvls = {};
//
//  diff->connectedBylvls[0].operator[](0)->ParentIds.emplace_back(myIdInRoom(roomId));
//  OnNodesMapDiff(RU, myIdInRoom(roomId), diff);
  addNode(RU, peerid, conn);
//  OnNodesMapDiffJson(RU, myIdInRoom(roomId), diff);
//  sendNodeMap(roomId, myIdInRoom(roomId), diff);
  return result;
}

//create offer Connection if it is Initiator
//or wait for offer to answer i.e. do nothing
bool WebrtcNode::createPC(scy::smpl::Peer &peer, std::string &roomId) {
  scy::wrtc::Peer::Mode sdptype;
  auto peerid = peer.user() + roomId;

  const auto weoffer =
      peerid > _conf.simple_options.user;//_myRole->isInitiator() && !_myRole->isHeInitiator(peer.user());

  if (weoffer) {
    sdptype = scy::wrtc::Peer::Mode::Offer;
  } else {
    _sigConn->sendPresence(peer.address(), roomId);
    return false;
  }

  scy::wrtc::Peer *conn = nullptr;

  bool result;
  bool exists = scy::wrtc::PeerManager::exists(peerid);

  if (!exists) {
    conn = new wrtc::Peer(this, _context, peerid, "", sdptype, roomId, _conf.ice_servers,
                          _conf.openPortsStart, _conf.openPortsEnd);
  } else {
    conn = scy::wrtc::PeerManager::get(peerid);
    conn->lastRoom() = roomId;
    if (conn->mode() != sdptype)
      return false;
  }

  if (!exists)
    conn->createConnection();

  conn->createOffer();

  if (!exists) {
    result = scy::wrtc::PeerManager::add(peerid, conn);
  } else result = true;

  std::lock_guard<std::shared_timed_mutex> lk(*_PeersInLvl_mutexes.get(roomId));
  const auto &RU = _roomManager->get(roomId);
//  auto diff = createNodesMapDiff();
//  auto diffUnit = std::make_shared<NodesMapDiffUnit>();
//  diffUnit->Id = peerid;
//  diff->connectedBylvls = {{0, {diffUnit}}};
//  diff->disconnectedBylvls = {};
//
//  diff->connectedBylvls[0].operator[](0)->ParentIds.emplace_back(myIdInRoom(roomId));
//  OnNodesMapDiff(RU, myIdInRoom(roomId), diff);
  addNode(RU, peerid, conn);
//  OnNodesMapDiffJson(RU, myIdInRoom(roomId), diff);
//  sendNodeMap(roomId, myIdInRoom(roomId), diff);
  return result;
}

void WebrtcNode::onPeerConnectedSignaler(SigMessage &msg) {
  if (stopping.load())
    return;
  LDebug("Peer connected: ", msg.from);
  if (scy::wrtc::PeerManager::exists(msg.from)) {
    LDebug("Peer already has session: ", msg.from)
  } else {
    createPC(msg.peer, msg.roomId);
  }
}

void WebrtcNode::onPeerDisconnectedSignaler(SigMessage &msg) {
  if (stopping.load())
    return;
  //here we will leave from system room
  LDebug("Peer disconnected")
}

void WebrtcNode::joinRoomWS(const std::string &roomId) {
  _sigConn->waitForOnline();

  if (std::find(rooms.begin(), rooms.end(), roomId) == rooms.end())
    rooms.push_back(roomId);
  else
    return;

  if (!_callbacks_mutexes.exists(roomId)) {
    _callbacks_mutexes.add(roomId, new std::shared_timed_mutex, false);
  }

  if (!_PeersInLvl_mutexes.exists(roomId)) {
    _PeersInLvl_mutexes.add(roomId, new std::shared_timed_mutex, false);
  }

  if (!_roomManager->exists(roomId)) {
    _roomManager->add(roomId, createRoomUnit(myIdInRoom(roomId)));
  }
  _sigConn->joinRoom(roomId);
}

void WebrtcNode::leaveRoom(const std::string &roomId) {
  //TODO make via webrtc
  leaveRoomWS(roomId);
}

void WebrtcNode::joinRoom(const std::string &roomId) {
  //TODO make via webrtc
  joinRoomWS(roomId);
}

void WebrtcNode::leaveRoomWS(const std::string &roomId) {
  _sigConn->waitForOnline();

  auto it = std::find(rooms.begin(), rooms.end(), roomId);
  if (it != rooms.end())
    rooms.erase(it);
  else
    return;

  if (!_roomManager->exists(roomId))
    return;

  const auto &peersMap = _roomManager->get(roomId)->PeersInRoom;
  const auto &RU = _roomManager->get(roomId);
  for (const auto &peer: peersMap) {
    RU->PeersInRoom.erase(peer.second->peerid());
    onClosed(peer.second);
  }

  auto r = _roomManager->remove(roomId);
  if (r)
    delete r;

  std::shared_timed_mutex *m = _callbacks_mutexes.get(roomId);

  if (!_callbacks.exists(roomId)) {
    std::lock_guard<std::shared_timed_mutex> lk(*m);
    auto rc = _callbacks.remove(roomId);
    if (rc) {
      delete rc;
    }
  }

  _sigConn->leaveRoom(roomId);
}

void WebrtcNode::onAddRemoteStream(scy::wrtc::Peer *conn, webrtc::MediaStreamInterface *stream) {
  assert(0 && "not required");
}


void WebrtcNode::onRemoveRemoteStream(scy::wrtc::Peer *conn, webrtc::MediaStreamInterface *stream) {
  assert(0 && "not required");
}


void WebrtcNode::onStable(scy::wrtc::Peer *conn) {
  _myRole->onStable(conn);
}

bool
WebrtcNode::broadcast2Room(const std::string &roomId, const std::string &j, MessageType t, const std::string &except) {
  if (!_roomManager->exists(roomId))
    return false;
  const auto &RU = _roomManager->get(roomId);

  scy::json::value msg;
  std::size_t hash = formMessage(msg, j, t, roomId);

  if (t == MessageType::Logic) {
    RU->LogicMsgHashesBuffer.put(hash);
  } else {
    RU->SystemMsgHashesBuffer.put(hash);
  }

  auto sendMsgTask = new sendMsgT;
  sendMsgTask->type = sendMsgT::broadcast;
  sendMsgTask->msg = std::move(msg);
  sendMsgTask->except = except;
  _sMq.push(sendMsgTask);
  _cvsMq.notify_all();
//  m = msg.dump();
//  const auto &peers_in_room = RU->PeersInRoom;
//  for (auto &peer_it: peers_in_room) {
//    if (!except.empty() && peer_it.first == except)
//      continue;
//    auto p = peer_it.second;
//    if (!p)
//      continue;
//    auto d = p->getDataChannel();
//    if (!d)
//      continue;
//    d->Send(m);
//  }
  return true;
}

void WebrtcNode::onClosed(wrtc::Peer *conn) {
  LDebug("ON Closed")
  const auto &RoomsMap = _roomManager->map();
  std::string roomId;
  for (auto &peersByRoomsMap: RoomsMap) {
    if (peersByRoomsMap.second->PeersInRoom.find(conn->peerid()) != peersByRoomsMap.second->PeersInRoom.end()) {
      roomId = peersByRoomsMap.first;
      break;
    }
  }

  if (roomId.empty())
    return;

  const auto m = _PeersInLvl_mutexes.get(roomId);
  std::lock_guard<std::shared_timed_mutex> lk_write(*m);
  auto diff = createNodesMapDiff();
  diff->connectedBylvls = {};
  auto diffUnit = std::make_shared<NodesMapDiffUnit>();
  diffUnit->Id = conn->peerid();
  diffUnit->ParentIds.emplace_back(myIdInRoom(roomId));
  diff->disconnectedBylvls = {{0, {diffUnit}}};
  OnNodesMapDiffJson(_roomManager->get(roomId), myIdInRoom(roomId), diff);
  sendNodeMap(roomId, conn->peerid(), diff);

  if (PeerManager::remove(conn))
    delete conn; // async delete
}


void WebrtcNode::onFailure(wrtc::Peer *conn, const std::string &error) {
  LDebug("ON Failure: ", error)
  const auto &RoomsMap = _roomManager->map();
  std::string roomId;
  for (auto &peersByRoomsMap: RoomsMap) {
    if (peersByRoomsMap.second->PeersInRoom.find(conn->peerid()) != peersByRoomsMap.second->PeersInRoom.end()) {
      roomId = peersByRoomsMap.first;
      break;
    }
  }

  if (roomId.empty())
    return;

  const auto m = _PeersInLvl_mutexes.get(roomId);
  std::lock_guard<std::shared_timed_mutex> lk_write(*m);
  auto diff = createNodesMapDiff();
  diff->connectedBylvls = {};
  auto diffUnit = std::make_shared<NodesMapDiffUnit>();
  diffUnit->Id = conn->peerid();
  diffUnit->ParentIds.emplace_back(myIdInRoom(roomId));
  diff->disconnectedBylvls = {{0, {diffUnit}}};
  OnNodesMapDiff(_roomManager->get(roomId), myIdInRoom(roomId), diff);
  OnNodesMapDiffJson(_roomManager->get(roomId), myIdInRoom(roomId), diff);
  sendNodeMap(roomId, conn->peerid(), diff);

  if (PeerManager::remove(conn))
    delete conn; // async delete
}

std::vector<std::string> WebrtcNode::getRoomIds() {
  return rooms;
}

std::vector<std::string> WebrtcNode::getNodeIds(const std::string &roomId) {
  std::vector<std::string> result;
  {
    std::shared_lock<std::shared_timed_mutex> lk(*_PeersInLvl_mutexes.get(roomId));
    collectAllNodeIds(result, _roomManager->get(roomId)->IamRoot);
  }
  return result;
}

void WebrtcNode::onIceServer(const std::vector<std::string> &v) {
  if (stopping.load())
    return;
  _conf.ice_servers.insert(_conf.ice_servers.end(), v.begin(), v.end());
}

void WebrtcNode::onIceCandidate(const SigMessage &msg) {
  if (stopping.load())
    return;
  recvCandidate(msg.from, msg.data);
}

void WebrtcNode::onSDPOffer(const SigMessage &msg) {
  if (stopping.load())
    return;
  if (createPC(msg.from, wrtc::Peer::Mode::Answer, msg.roomId)) {
    _myRole->onReceiveAnswer(wrtc::PeerManager::get(msg.from), _context);
    recvSDP(msg.from, msg.data);
  } else {
    LDebug("Can not create PC in mode Answer")
  }
}

void WebrtcNode::onSDPAnswer(const SigMessage &msg) {
  if (stopping.load())
    return;
  recvSDP(msg.from, msg.data);
}
