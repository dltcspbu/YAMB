//
// Created by panda on 10/3/19.
//

#ifndef WEBRTCMESSAGEBROKER_NODESMAP_T_H
#define WEBRTCMESSAGEBROKER_NODESMAP_T_H

#include "../../base/collection.h"
#include "../../json/json.h"
#include "../../webrtc/webrtc.h"
#include "../../webrtc/peer.h"
#include "NodeMapDiff.h"

// substracts b<T> to a<T>
void
substract_json_arrays(scy::json::value &a, const scy::json::value &b);

struct NodesMap_t {
  scy::json::value Data;
};

//assuming that nodeId is new node and we have no this Id in our Map
//bool addToLvl(std::shared_ptr<NodesMap_t> &, int lvl, const std::string &nodeId);

bool addToLvl(std::shared_ptr<NodesMap_t> &, int lvl,
              const std::vector<std::shared_ptr<NodesMapDiffUnit>> &nodesMapDiffUnits);

//bool eraseFromLvl(std::shared_ptr<NodesMap_t> &, int lvl, const std::string &nodeId);

bool eraseFromLvl(std::shared_ptr<NodesMap_t> &, int lvl,
                  const std::vector<std::shared_ptr<NodesMapDiffUnit>> &nodesMapDiffUnits);

#endif //WEBRTCMESSAGEBROKER_NODESMAP_T_H
