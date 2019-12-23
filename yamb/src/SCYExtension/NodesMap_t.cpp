//
// Created by panda on 10/3/19.
//

#include "NodesMap_t.h"


void
substract_json_arrays(scy::json::value &a, const scy::json::value &b) {
  auto it = a.begin();
  auto it2 = b.begin();

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

//bool addToLvl(std::shared_ptr<NodesMap_t> &nm, int lvl, const std::string &nodeId) {
//  auto stringlvl = std::to_string(lvl);
//  auto &nm_data = nm->Data;
//  if (nm_data.find(stringlvl) == nm_data.end())
//    nm_data[stringlvl] = scy::json::value::array();
//  nm_data[stringlvl].push_back(nodeId);
//  return true;
//}

bool addToLvl(std::shared_ptr<NodesMap_t> &nm, int lvl,
              const std::vector<std::shared_ptr<NodesMapDiffUnit>> &nodesMapDiffUnits) {
  auto stringlvl = std::to_string(lvl);
  auto &nm_data = nm->Data;
  if (nm_data.find(stringlvl) == nm_data.end())
    nm_data[stringlvl] = scy::json::value::array();
  nlohmann::json tmp = nodesMapDiffUnits;
  std::for_each(nodesMapDiffUnits.begin(), nodesMapDiffUnits.end(),
                [&nm_data, &stringlvl](const std::shared_ptr<NodesMapDiffUnit> &du) {
                  //std::cout << nm_data[stringlvl].dump() << std::endl;
                  auto it = std::find_if(nm_data[stringlvl].begin(), nm_data[stringlvl].end(),
                                         [&du](scy::json::value &x) {
                                           return (x["Id"].get<std::string>() == du->Id);
                                         });
                  if (it != nm_data[stringlvl].end()) {
                    auto y = it->get<scy::json::value::object_t>();
                    auto q = y["ParentIds"].get<scy::json::value::array_t>();
                    std::for_each(du->ParentIds.begin(), du->ParentIds.end(), [&q](auto &x) {
                      q.push_back(x);
                    });
                  } else {
                    nm_data[stringlvl].insert(nm_data[stringlvl].end(), *du);
                  }
                });
  //nm_data[stringlvl].insert(nm_data[stringlvl].begin(), tmp.begin(), tmp.end());
  return true;
}

//bool eraseFromLvl(std::shared_ptr<NodesMap_t> &nm, int lvl, const std::string &nodeId) {
//  auto stringlvl = std::to_string(lvl);
//  auto &nm_data = nm->Data;
//  if (nm_data.find(stringlvl) == nm_data.end())
//    return false;
//  auto pos = std::find_if(nm_data[stringlvl].begin(), nm_data[stringlvl].end(), [nodeId](const nlohmann::json &x) {
//    auto iterId = x.get<std::string>();
//    return iterId == nodeId;
//  });
//  if (pos == nm_data[stringlvl].end())
//    return false;
//  nm_data[stringlvl].erase(pos);
//  return true;
//}

bool eraseFromLvl(std::shared_ptr<NodesMap_t> &nm, int lvl,
                  const std::vector<std::shared_ptr<NodesMapDiffUnit>> &nodesMapDiffUnits) {
  auto stringlvl = std::to_string(lvl);
  auto &nm_data = nm->Data;
  if (nm_data.find(stringlvl) == nm_data.end())
    return false;
  nlohmann::json tmp = nodesMapDiffUnits;
  substract_json_arrays(nm_data[stringlvl], nodesMapDiffUnits);
  return true;
}