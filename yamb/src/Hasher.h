//
// Created by panda on 10/3/19.
//

#ifndef WEBRTCMESSAGEBROKER_HASHER_H
#define WEBRTCMESSAGEBROKER_HASHER_H

#include <functional>

class Hasher {
  std::string _seed;
  std::hash<std::string> _hasher;
public:
  Hasher(const std::string &s) : _seed(s) {}

  size_t operator()(const std::string &hashable) {
    return _hasher(_seed + hashable);
  }

  size_t operator()(std::string &&hashable) {
    return _hasher(_seed + hashable);
  }
};


#endif //WEBRTCMESSAGEBROKER_HASHER_H
