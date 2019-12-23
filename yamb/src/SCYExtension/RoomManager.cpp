//
// Created by panda on 01.07.19.
//

#include "RoomManager.h"

// substracts b<T> to a<T>
template<typename T>
void
substract_vector(std::vector<T> &a, const std::vector<T> &b) {
  typename std::vector<T>::iterator it = a.begin();
  typename std::vector<T>::const_iterator it2 = b.begin();

  while (it != a.end()) {
    while (it2 != b.end() && it != a.end()) {
      if (*it == *it2) {
        it = a.erase(it);
        it2 = b.begin();
      } else
        ++it2;
    }
    if (it != a.end())
      ++it;
    it2 = b.begin();
  }
}

void OnNodesMapDiff(RoomUnit *RU, const std::string &myId, std::shared_ptr<NodesMapDiff> &diff) {
//  std::cout << "begin " << myId << std::endl;
  std::for_each(diff->connectedBylvls.begin(), diff->connectedBylvls.end(),
                [&RU, &myId](const auto &pair) {
                  const std::vector<std::shared_ptr<NodesMapDiffUnit>> &mapDiffUnits = pair.second;
                  const unsigned int mylvl = pair.first > MAX_LVL ? MAX_LVL : pair.first;

                  RU->nPeersInRoom += mapDiffUnits.size();
                  for (const auto &mapDiffUnit: mapDiffUnits) {
                    auto &pids = mapDiffUnit->ParentIds;
                    //there is second lvl root here
//                    if (pids.size() == 1 && pids[0] == myId) {
//                      continue;
//                    }
                    if (pids.empty()) {
                      assert(mylvl - RU->IamRoot->Lvl == 1);
//                      std::cout << "adding lvl " << mylvl << " " << mapDiffUnit->Id << " to " << " lvl "
//                                << RU->IamRoot->Lvl << " " << RU->IamRoot->Id << std::endl;
                      auto newNodeUnit = new NodeUnit;
                      newNodeUnit->Id = mapDiffUnit->Id;
                      assert(mylvl == 0 && "mylvl have to be zero in case of root!");
                      newNodeUnit->Lvl = mylvl;
                      RU->IamRoot->Childs.push_back(newNodeUnit);
                      newNodeUnit->Parents.push_back(RU->IamRoot);
                      RU->NodeUnitsGraphRoots.push_back(newNodeUnit);
                    } else {
                      //there is not root with lvl = 0
                      NodeUnit *foundNode = findInGraphByChain(RU->IamRoot, pids);
                      if (foundNode) {
//                        std::cout << "adding lvl " << mylvl << " " << mapDiffUnit->Id << " to " << " lvl "
//                                  << foundNode->Lvl << " " << foundNode->Id << std::endl;
                        assert(mylvl - foundNode->Lvl == 1);
                        auto newNodeUnit = new NodeUnit;
                        newNodeUnit->Id = mapDiffUnit->Id;
                        newNodeUnit->Lvl = mylvl;
                        newNodeUnit->Parents.push_back(foundNode);
                        foundNode->Childs.push_back(newNodeUnit);
                      } else {
                        //NodeUnit *foundNode = findInGraphByChain(RU->IamRoot, pids);
                        assert(false && "This is very bad...");
                      }
                    }
                  }
                  //addToLvl(RU->NodesMap, mylvl, mapDiffUnits);
                });
  std::for_each(diff->disconnectedBylvls.begin(), diff->disconnectedBylvls.end(),
                [&RU, &myId](const auto &pair) {
                  const std::vector<std::shared_ptr<NodesMapDiffUnit>> &mapDiffUnits = pair.second;
                  const unsigned int mylvl = pair.first > MAX_LVL ? MAX_LVL : pair.first;
                  RU->nPeersInRoom -= mapDiffUnits.size();

                  for (const auto &mapDiffUnit: mapDiffUnits) {
                    auto &pids = mapDiffUnit->ParentIds;
                    auto root = findInGraphWithParents(RU->IamRoot, pids, mapDiffUnit->Id);
                    //there is second lvl root here
                    if (pids.empty()) {
                      auto &vr = RU->NodeUnitsGraphRoots;
                      vr.erase(std::remove(vr.begin(), vr.end(), root), vr.end());
                    }
                    deleteNodeFromGraph(root);
                  }
                  //eraseFromLvl(RU->NodesMap, mylvl, mapDiffUnits);
                });
//  std::cout << "end " << myId << std::endl;
}


void OnNodesMapDiffJson(RoomUnit *RU, const std::string &myId, std::shared_ptr<NodesMapDiff> &diff) {
  std::for_each(diff->connectedBylvls.begin(), diff->connectedBylvls.end(),
                [&RU, &myId](const auto &pair) {
                  const std::vector<std::shared_ptr<NodesMapDiffUnit>> &mapDiffUnits = pair.second;
                  const unsigned int mylvl = pair.first > MAX_LVL ? MAX_LVL : pair.first;
                  addToLvl(RU->NodesMap, mylvl, mapDiffUnits);
                });
  std::for_each(diff->disconnectedBylvls.begin(), diff->disconnectedBylvls.end(),
                [&RU, &myId](const auto &pair) {
                  const std::vector<std::shared_ptr<NodesMapDiffUnit>> &mapDiffUnits = pair.second;
                  const unsigned int mylvl = pair.first > MAX_LVL ? MAX_LVL : pair.first;
                  eraseFromLvl(RU->NodesMap, mylvl, mapDiffUnits);
                });
}

void addNode(RoomUnit *RU, const std::string &peerId, scy::wrtc::Peer *peer) {
  if (RU->PeersInRoom.find(peerId) == RU->PeersInRoom.end()) {
    RU->PeersInRoom[peerId] = peer;
  }
}

RoomUnit *createRoomUnit(const std::string &myId) {
  auto RU = new RoomUnit;
  RU->NodesMap = std::make_shared<NodesMap_t>();
  RU->IamRoot = new NodeUnit;
  RU->IamRoot->Id = myId;
  RU->IamRoot->Lvl = -1;
  RU->NodesMap->Data["0"] = scy::json::value::array();
  RU->NodesMap->Data["1"] = scy::json::value::array();
  return RU;
}


void deleteRoomUnit() {

}


void createDiffConnectedBylvls(std::shared_ptr<NodesMapDiff> &nm_diff, RoomUnit *RU,
                               const rapidjson::Value &incomingConnectedByLvls) {
  const auto &iterable = incomingConnectedByLvls.GetObject();
  for (const auto &itr: iterable) {
    const auto lvl = std::atoi(itr.name.GetString()) + 1;
    const auto mylvl = lvl > MAX_LVL ? MAX_LVL : lvl;
    const auto &arr = itr.value.GetArray();
    std::for_each(arr.begin(), arr.end(), [&nm_diff, &RU, mylvl](const rapidjson::Value &val) {
      const auto &MapDiffUnit = val.GetObject();
      const auto Id = MapDiffUnit["Id"].GetString();
      if (Id == RU->IamRoot->Id)
        return;
      std::vector<std::string> inc_pids;
      const auto &arr = MapDiffUnit["ParentIds"].GetArray();
      inc_pids.reserve(arr.Size());
      for (const auto &inc_pid : arr) {
        inc_pids.emplace_back(inc_pid.GetString());
      }
      NodeUnit *foundNode = findInGraph(RU->IamRoot, Id);
      if (foundNode) {
        if (foundNode->Lvl <= mylvl) {
          //its ok
          return;
        } else {
          auto nm_diff_unit = std::make_shared<NodesMapDiffUnit>();
          nm_diff_unit->Id = Id;
          auto f = findInGraph(RU->IamRoot, inc_pids[inc_pids.size() - 1]);
          assert(f);
          nm_diff_unit->ParentIds = inc_pids;
          assert(!nm_diff_unit->ParentIds.empty());
          if (nm_diff_unit->ParentIds.size() == MAX_LVL + 1) {
            nm_diff_unit->ParentIds.erase(nm_diff_unit->ParentIds.begin());
          }
          nm_diff_unit->ParentIds.emplace_back(RU->IamRoot->Id);
          nm_diff->connectedBylvls[mylvl].push_back(nm_diff_unit);
          assert(nm_diff_unit->ParentIds.size() == mylvl + 1);
          assert(nm_diff_unit->ParentIds.back() == RU->IamRoot->Id);
          std::vector<std::vector<std::string>> v;
          fillChain(foundNode, v);
          assert(!v.empty());
          for (const auto &vv: v) {
            auto nm_diff_unit_del = std::make_shared<NodesMapDiffUnit>();
            nm_diff_unit_del->Id = foundNode->Id;
            nm_diff_unit_del->ParentIds = vv;
            assert(!nm_diff_unit_del->ParentIds.empty());
            std::reverse(nm_diff_unit_del->ParentIds.begin(), nm_diff_unit_del->ParentIds.end());
            if (nm_diff_unit_del->ParentIds.size() == MAX_LVL + 1) {
              nm_diff_unit_del->ParentIds.pop_back();
            }
            nm_diff_unit_del->ParentIds.insert(nm_diff_unit_del->ParentIds.begin(), foundNode->Id);
            nm_diff->disconnectedBylvls[foundNode->Lvl].push_back(nm_diff_unit_del);
          }
        }
      } else {
        auto f = findInGraph(RU->IamRoot, inc_pids[inc_pids.size() - 1]);
        //assert(f);
        if (!f)
          return;
        std::vector<std::vector<std::string>> v;
        fillChain(f, v);
        assert(!v.empty());
        std::sort(v.begin(), v.end(), [](std::vector<std::string> &a, std::vector<std::string> &b) {
          return a.size() < b.size();
        });
        const auto &x = v[0];
        assert(!x.empty());
        for (const auto &vv: v) {
          if (vv.size() != x.size())
            continue;
          auto nm_diff_unit = std::make_shared<NodesMapDiffUnit>();
          nm_diff_unit->Id = Id;
          nm_diff_unit->ParentIds = vv;
//            std::for_each(nm_diff_unit->ParentIds.begin(), nm_diff_unit->ParentIds.end(), [](const auto &x) {
//              std::cout << x << std::endl;
//            });
          //nm_diff_unit->ParentIds.emplace_back(f->Id);
          //std::reverse(nm_diff_unit->ParentIds.begin(), nm_diff_unit->ParentIds.end());
          assert(nm_diff_unit->ParentIds.back() == RU->IamRoot->Id);
          if (nm_diff_unit->ParentIds.size() == MAX_LVL + 1) {
            nm_diff_unit->ParentIds.pop_back();
          }
          nm_diff_unit->ParentIds.insert(nm_diff_unit->ParentIds.begin(), f->Id);
          //assert(nm_diff_unit->ParentIds.size() == mylvl + 1);
          //assert(nm_diff_unit->ParentIds.size() - 1 == mylvl);
          assert(nm_diff_unit->ParentIds.size() - 1 == f->Lvl + 1 > MAX_LVL ? MAX_LVL : f->Lvl + 1);
          nm_diff->connectedBylvls[nm_diff_unit->ParentIds.size() - 1].push_back(nm_diff_unit);
        }
      }
//      NodeUnit *foundNode = findInGraphWithParents(RU->IamRoot, inc_pids, Id);
//      if (foundNode) {
//        assert(foundNode->Lvl <= mylvl);
//      } else {
//        foundNode = findInGraphWithoutParents(RU->IamRoot, inc_pids, Id);
//        if (foundNode && foundNode->Lvl > mylvl) {
//          auto nm_diff_unit = std::make_shared<NodesMapDiffUnit>();
//          nm_diff_unit->Id = Id;
//          nm_diff_unit->ParentIds = inc_pids;
//          assert(!nm_diff_unit->ParentIds.empty());
////          if (nm_diff_unit->ParentIds.size() == MAX_LVL + 1) {
////            nm_diff_unit->ParentIds.erase(nm_diff_unit->ParentIds.begin());
////          }
//          nm_diff_unit->ParentIds.emplace_back(RU->IamRoot->Id);
//          nm_diff->connectedBylvls[mylvl].push_back(nm_diff_unit);
//          assert(nm_diff_unit->ParentIds.size() == mylvl + 1);
//          assert(nm_diff_unit->ParentIds.back() == RU->IamRoot->Id);
//          std::vector<std::vector<std::string>> v;
//          fillChain(foundNode, v);
//          for (const auto &vv: v) {
//            auto nm_diff_unit_del = std::make_shared<NodesMapDiffUnit>();
//            nm_diff_unit_del->Id = foundNode->Id;
//            nm_diff_unit_del->ParentIds = vv;
//            assert(!nm_diff_unit_del->ParentIds.empty());
//            std::reverse(nm_diff_unit_del->ParentIds.begin(), nm_diff_unit_del->ParentIds.end());
////            if (nm_diff_unit_del->ParentIds.size() == MAX_LVL + 1) {
////              nm_diff_unit_del->ParentIds.pop_back();
////            }
//            nm_diff_unit_del->ParentIds.insert(nm_diff_unit_del->ParentIds.begin(), foundNode->Id);
//            nm_diff->disconnectedBylvls[foundNode->Lvl].push_back(nm_diff_unit_del);
//          }
//        } else if (!foundNode) {
////          auto nm_diff_unit = std::make_shared<NodesMapDiffUnit>();
////          nm_diff_unit->Id = Id;
//          //nm_diff_unit->ParentIds = inc_pids;
//          assert(!inc_pids.empty());
//          auto f = findInGraph(RU->IamRoot, inc_pids[inc_pids.size() - 1]);
//          assert(f);
//          std::vector<std::vector<std::string>> v;
//          fillChain(f, v);
//          assert(!v.empty());
//          std::sort(v.begin(), v.end(), [](std::vector<std::string> &a, std::vector<std::string>&b){
//            return a.size() < b.size();
//          });
//          const auto& vv = v[0];
//          //for (const auto &vv: v) {
//            auto nm_diff_unit = std::make_shared<NodesMapDiffUnit>();
//            nm_diff_unit->Id = Id;
//            nm_diff_unit->ParentIds = vv;
//            assert(!nm_diff_unit->ParentIds.empty());
////            std::for_each(nm_diff_unit->ParentIds.begin(), nm_diff_unit->ParentIds.end(), [](const auto &x) {
////              std::cout << x << std::endl;
////            });
//            //nm_diff_unit->ParentIds.emplace_back(f->Id);
//            //std::reverse(nm_diff_unit->ParentIds.begin(), nm_diff_unit->ParentIds.end());
//            if (nm_diff_unit->ParentIds.size() == MAX_LVL + 1) {
//              nm_diff_unit->ParentIds.pop_back();
//            }
//            nm_diff_unit->ParentIds.insert(nm_diff_unit->ParentIds.begin(), f->Id);
//            //assert(nm_diff_unit->ParentIds.size() == mylvl + 1);
//            assert(nm_diff_unit->ParentIds.size() - 1 >= 0);
//            //assert(nm_diff_unit->ParentIds.size() - 1 == mylvl);
//            assert(nm_diff_unit->ParentIds.size() - 1 == f->Lvl + 1 > MAX_LVL ? MAX_LVL : f->Lvl + 1);
//            assert(nm_diff_unit->ParentIds.back() == RU->IamRoot->Id);
//            nm_diff->connectedBylvls[nm_diff_unit->ParentIds.size() - 1].push_back(nm_diff_unit);
//          //}
//        }
//      }

    });
  }
}

void createDiffDisconnectedBylvls(std::shared_ptr<NodesMapDiff> &nm_diff, RoomUnit *RU,
                                  const rapidjson::Value &incomingDisconnectedByLvls) {
  const auto &iterable = incomingDisconnectedByLvls.GetObject();
  for (const auto &itr: iterable) {
    const auto lvl = std::atoi(itr.name.GetString()) + 1;
    const auto mylvl = lvl > MAX_LVL ? MAX_LVL : lvl;
    const auto &arr = itr.value.GetArray();
    std::for_each(arr.begin(), arr.end(), [&nm_diff, &RU, mylvl](const rapidjson::Value &val) {
      const auto &MapDiffUnit = val.GetObject();
      const auto Id = MapDiffUnit["Id"].GetString();
      if (Id == RU->IamRoot->Id)
        return;
      std::vector<std::string> inc_pids;
      const auto &arr = MapDiffUnit["ParentIds"].GetArray();
      inc_pids.reserve(arr.Size());

      for (const auto &inc_pid : arr) {
        inc_pids.emplace_back(inc_pid.GetString());
      }

      NodeUnit *foundNode = findInGraphWithParents(RU->IamRoot, inc_pids, Id);

      if (foundNode) {
        auto nm_diff_unit = std::make_shared<NodesMapDiffUnit>();
        nm_diff_unit->Id = Id;
        nm_diff_unit->ParentIds = inc_pids;
        nm_diff_unit->ParentIds.emplace_back(RU->IamRoot->Id);
        const auto resultlvl = mylvl - foundNode->Lvl == 0 ? mylvl : foundNode->Lvl;
        nm_diff->disconnectedBylvls[resultlvl].push_back(nm_diff_unit);
      } else {
        foundNode = findInGraphWithoutParents(RU->IamRoot, inc_pids, Id);
        if (foundNode) {
          inc_pids.clear();
          std::vector<std::vector<std::string>> v;
          fillChain(foundNode, v);
          for (const auto &vv: v) {
            auto nm_diff_unit = std::make_shared<NodesMapDiffUnit>();
            nm_diff_unit->Id = Id;
            nm_diff_unit->ParentIds = vv;
            const auto resultlvl = mylvl - foundNode->Lvl == 0 ? mylvl : foundNode->Lvl;
            nm_diff->disconnectedBylvls[resultlvl].push_back(nm_diff_unit);
          }
        }
      }
    });
  }
}