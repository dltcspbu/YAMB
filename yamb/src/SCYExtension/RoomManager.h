//
// Created by panda on 01.07.19.
//

#ifndef WEBRTCSTREAMER_ROOMMANAGER_H
#define WEBRTCSTREAMER_ROOMMANAGER_H

#include "../../base/logger.h"
#include "../../base/collection.h"
#include "../../json/json.h"
#include "../../webrtc/webrtc.h"
#include "../../webrtc/peer.h"
#include <CircularBuffer.h>
#include "NodesMap_t.h"
#include "NodeMapDiff.h"
#include "NodeGraph.h"

//starting from 0
const unsigned int MAX_LVL = 2;


typedef std::vector<NodeUnit *> PeersByLvlContainer;

// substracts b<T> to a<T>
template<typename T>
void
substract_vector(std::vector<T> &a, const std::vector<T> &b);

struct RoomUnit {
  std::map<std::string, scy::wrtc::Peer *> PeersInRoom;
  PeersByLvlContainer NodeUnitsGraphRoots; //this is allias to IamRoot->Childs
  size_t nPeersInRoom = 1;
  NodeUnit *IamRoot;
  CircularBuffer<size_t, 500> LogicMsgHashesBuffer;
  CircularBuffer<size_t, 500> SystemMsgHashesBuffer;
  std::shared_ptr<NodesMap_t> NodesMap;
};

typedef scy::PointerCollection<std::string, RoomUnit> RoomManager;

void createDiffConnectedBylvls(std::shared_ptr<NodesMapDiff> &nm_diff, RoomUnit *RU,
                               const rapidjson::Value &incomingConnectedByLvls);

void createDiffDisconnectedBylvls(std::shared_ptr<NodesMapDiff> &nm_diff, RoomUnit *RU,
                                  const rapidjson::Value &incomingDisconnectedByLvls);

void OnNodesMapDiff(RoomUnit *RU, const std::string &myId, std::shared_ptr<NodesMapDiff> &diff);

void addNode(RoomUnit *RU, const std::string &peerId, scy::wrtc::Peer *peer);

void OnNodesMapDiffJson(RoomUnit *RU, const std::string &myId, std::shared_ptr<NodesMapDiff> &diff);

RoomUnit *createRoomUnit(const std::string &);

void deleteRoomUnit();

#endif //WEBRTCSTREAMER_ROOMMANAGER_H
