//
// Created by panda on 10/9/19.
//

#include "NodeMapDiff.h"

std::shared_ptr<NodesMapDiff> createNodesMapDiff() {
  return std::make_shared<NodesMapDiff>();
}


void to_json(scy::json::value &result, const NodesMapDiff &object) {
  result["connectedBylvls"] = scy::json::value::object();
  result["disconnectedBylvls"] = scy::json::value::object();
  std::for_each(object.disconnectedBylvls.begin(), object.disconnectedBylvls.end(),
                [&result](const auto &pair) {
                  result["disconnectedBylvls"][std::to_string(pair.first)] = scy::json::value::array();
                  for (const auto &nm_unit: pair.second) {
                    result["disconnectedBylvls"][std::to_string(pair.first)].push_back({{"Id",        nm_unit->Id},
                                                                                        {"ParentIds", nm_unit->ParentIds}});
                  }
                });
  std::for_each(object.connectedBylvls.begin(), object.connectedBylvls.end(),
                [&result](const auto &pair) {
                  result["connectedBylvls"][std::to_string(pair.first)] = scy::json::value::array();
                  for (const auto &nm_unit: pair.second) {
                    result["connectedBylvls"][std::to_string(pair.first)].push_back({{"Id",        nm_unit->Id},
                                                                                     {"ParentIds", nm_unit->ParentIds}});
                  }
                });
}

void to_json(scy::json::value &result, const NodesMapDiffUnit &object) {
  result = {{"Id",        object.Id},
            {"ParentIds", object.ParentIds}};
}

void to_json(scy::json::value &result, const std::vector<std::shared_ptr<NodesMapDiffUnit>> &nodesMapDiffUnits) {
  result = scy::json::value::array();
  for (const auto &nm_diffUnit: nodesMapDiffUnits) {
    result.push_back({{"Id",        nm_diffUnit->Id},
                      {"ParentIds", nm_diffUnit->ParentIds}});
  }
}