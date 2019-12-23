//
// Created by panda on 10/9/19.
//

#ifndef WEBRTCMESSAGEBROKER_NODEMAPDIFF_H
#define WEBRTCMESSAGEBROKER_NODEMAPDIFF_H


#include "../../base/collection.h"
#include "../../json/json.h"
#include "../../webrtc/webrtc.h"
#include "../../webrtc/peer.h"


struct NodesMapDiffUnit {
  std::string Id;
  std::vector<std::string> ParentIds;
};

struct NodesMapDiff {
  std::map<int, std::vector<std::shared_ptr<NodesMapDiffUnit>>> connectedBylvls;
  std::map<int, std::vector<std::shared_ptr<NodesMapDiffUnit>>> disconnectedBylvls;
};

std::shared_ptr<NodesMapDiff> createNodesMapDiff();

void to_json(scy::json::value &result, const NodesMapDiff &object);

void to_json(scy::json::value &result, const NodesMapDiffUnit &object);

void to_json(scy::json::value &result, const std::vector<std::shared_ptr<NodesMapDiffUnit>> &nodesMapDiffUnits);

#endif //WEBRTCMESSAGEBROKER_NODEMAPDIFF_H
