//
// Created by panda on 10/10/19.
//

#ifndef WEBRTCMESSAGEBROKER_NODEGRAPH_H
#define WEBRTCMESSAGEBROKER_NODEGRAPH_H

#include <vector>
#include <string>
#include <algorithm>

struct NodeUnit {
  int Lvl;
  std::string Id;
  std::vector<NodeUnit *> Parents;
  std::vector<NodeUnit *> Childs;
};


NodeUnit *findInGraphWithRememberingDirectConnections(NodeUnit *root, const std::string &searchId,
                                                      std::vector<std::string> &foundChain);

NodeUnit *findInGraph(NodeUnit *root, const std::string &searchId);

//search with support information from parent ids
NodeUnit *
findInGraphWithParents(NodeUnit *root, const std::vector<std::string> &parentSearchIds, const std::string &searchId);

//search with support information from parent ids
NodeUnit *
findInGraphByChain(NodeUnit *root, const std::vector<std::string> &searchIds);


//search with support information from parent ids
NodeUnit *findInGraphWithoutParentsInternal(NodeUnit *root, const std::vector<std::string> &parentNotSearchIds,
                                            const std::string &searchId, size_t position);

//search with support information from parent ids
NodeUnit *findInGraphWithoutParents(NodeUnit *root, const std::vector<std::string> &parentNotSearchIds,
                                    const std::string &searchId);

void collectAllNodeIds(std::vector<std::string> &result, NodeUnit *root);

void fillChain(NodeUnit *root, std::vector<std::vector<std::string>> &chains);

void fillChain(NodeUnit *root, std::vector<std::vector<std::string>> &chains, std::vector<std::string>& mychain);
bool deleteNodeFromGraph(NodeUnit *deleting);

#endif //WEBRTCMESSAGEBROKER_NODEGRAPH_H
