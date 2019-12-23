//
// Created by panda on 10/10/19.
//

#include "NodeGraph.h"


NodeUnit *findInGraph(NodeUnit *root, const std::string &searchId) {
  if (root->Id == searchId)
    return root;
  NodeUnit *result = nullptr;
  for (const auto &child: root->Childs) {
    result = findInGraph(child, searchId);
    if (result)
      return result;
  }
  return result;
}

//search with support information from parent ids
NodeUnit *
findInGraphWithParents(NodeUnit *root, const std::vector<std::string> &parentSearchIds, const std::string &searchId) {
  auto result = root;
  for (auto parentSearchIdItr = parentSearchIds.rbegin();
       parentSearchIdItr != parentSearchIds.rend(); ++parentSearchIdItr) {
    auto &parentSearchId = *parentSearchIdItr;
    result = findInGraph(result, parentSearchId);
    if (!result) {
      return result;
    }
  }
  return findInGraph(result, searchId);
}

//search with support information from parent ids
NodeUnit *
findInGraphByChain(NodeUnit *root, const std::vector<std::string> &searchIds) {
  auto result = root;
  for (auto searchIdsItr = searchIds.rbegin();
       searchIdsItr != searchIds.rend(); ++searchIdsItr) {
    auto &searchId = *searchIdsItr;
    if (root->Id == searchId)
      continue;
    auto it = std::find_if(result->Childs.begin(), result->Childs.end(), [&searchId](NodeUnit *x) {
      return x->Id == searchId;
    });
    if (it != result->Childs.end())
      result = *it;
    else
      return nullptr;
  }
  return result;
}


//search with support information from parent ids
NodeUnit *findInGraphWithoutParentsInternal(NodeUnit *root, const std::vector<std::string> &parentNotSearchIds,
                                            const std::string &searchId, size_t position) {
  if (root->Id == searchId)
    return root;
  NodeUnit *result = nullptr;
  for (const auto &child: root->Childs) {
    if (child->Id == parentNotSearchIds[position])
      continue;
    auto newpos = position - 1;
    if (newpos > 0) {
      result = findInGraphWithoutParentsInternal(child, parentNotSearchIds, searchId, newpos);
    } else {
      result = findInGraph(child, searchId);
    }
    if (result)
      return result;
  }
  return result;
}

//search with support information from parent ids
NodeUnit *findInGraphWithoutParents(NodeUnit *root, const std::vector<std::string> &parentNotSearchIds,
                                    const std::string &searchId) {
  if (!parentNotSearchIds.empty()) {
    auto pos = parentNotSearchIds.size() - 1;
    return findInGraphWithoutParentsInternal(root, parentNotSearchIds, searchId, pos);
  }
  return findInGraph(root, searchId);
}

void fillChain(NodeUnit *root, std::vector<std::vector<std::string>> &chains) {
  auto curr = root;
  //while (curr && !curr->Parents.empty()) {
  for (const auto &p: curr->Parents) {
    chains.emplace_back(std::vector<std::string>{p->Id});
    fillChain(p, chains, chains.back());
  }

  // }
}

void fillChain(NodeUnit *root, std::vector<std::vector<std::string>> &chains, std::vector<std::string> &mychain) {
  for (size_t i = 0; i < root->Parents.size(); ++i) {
    if (i == 0)
      mychain.emplace_back(root->Parents[i]->Id);
    if (i > 0) {
      chains.emplace_back(std::vector<std::string>{root->Parents[i]->Id});
      fillChain(root->Parents[i], chains, chains.back());
    }
  }
}

NodeUnit *
findInGraphWithRememberingDirectConnections(NodeUnit *root, const std::string &searchId,
                                            std::vector<std::string> &foundChain) {
  if (root->Id == searchId)
    return root;
  NodeUnit *result = nullptr;
  for (const auto &child: root->Childs) {
    result = findInGraphWithRememberingDirectConnections(child, searchId, foundChain);
    if (result && child->Lvl == 0) {
      foundChain.emplace_back(result->Id);
      //return result;
    }
  }
  return result;
}

bool deleteNodeFromGraph(NodeUnit *deleting) {
  //this is empty root without childs
  if (!deleting)
    return true;
  for (const auto &child: deleting->Childs) {
    auto &childvec = child->Parents;
    childvec.erase(std::remove(childvec.begin(), childvec.end(), deleting), childvec.end());
    if (childvec.empty()) {
      for (const auto &parent: childvec) {
        auto &parentvec = parent->Childs;
        parentvec.erase(std::remove(parentvec.begin(), parentvec.end(), child), parentvec.end());
      }
      deleteNodeFromGraph(child);
    }
  }
  for (const auto &parent: deleting->Parents) {
    auto &parentvec = parent->Childs;
    parentvec.erase(std::remove(parentvec.begin(), parentvec.end(), deleting), parentvec.end());
  }

  delete deleting;
  return true;
}

void collectAllNodeIds(std::vector<std::string> &result, NodeUnit *root) {
  for (const auto &child: root->Childs) {
    if (std::find(result.begin(), result.end(), child->Id) == result.end()) {
      result.emplace_back(child->Id);
      collectAllNodeIds(result, child);
    }
  }
}