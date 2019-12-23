//
// Created by zharkov on 18.07.19.
//

#include "messageBroker.hpp"

#include "WebrtcNode.h"
#include "../base/idler.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/thread.h"

std::thread T;

class messageBroker::signalerWrapper {
public:
  signalerWrapper(const nodeConfig &conf) {
//    std::thread([this, &WSHost, &WSPort, &name, &user]() {
    scy::smpl::Client::Options options;

    options.host = conf.WSHost;
    options.port = conf.WSPort;
    options.name = conf.name;
    options.user = conf.user;

    if (options.user.empty()) {
      std::string uid = scy::util::randomString(24);
      std::replace(uid.begin(), uid.end(), '/', 'S');
      options.user = uid;
    }

    config c;
    c.simple_options = options;
    c.sqlite3Path = conf.sqlite3Path;
    c.hashSeed = conf.hashSeed;
//    if (!conf.stunUri.empty())
//      c.ice_servers.push_back(conf.stunUri);
//    if (!conf.turnUri.empty())
//      c.ice_servers.push_back(conf.turnUri);
//    if (!conf.stunUriReserve.empty())
//      c.ice_servers.push_back(conf.stunUriReserve);
//    if (!conf.turnUriReserve.empty())
//      c.ice_servers.push_back(conf.turnUriReserve);

    c.openPortsStart = conf.openPortsStart;
    c.openPortsEnd = conf.openPortsEnd;

#ifdef SCY_UNIX
    unsigned int ncores = std::thread::hardware_concurrency();
    setenv("UV_THREADPOOL_SIZE", std::to_string(ncores).c_str(), 1);
#endif
    _conf.WSHost = conf.WSHost;
    _conf.WSPort = conf.WSPort;
    _conf.user = conf.user;
    _conf.name = conf.name;
    _conf.openPortsStart = conf.openPortsStart;
    _conf.openPortsEnd = conf.openPortsEnd;
    _conf.sqlite3Path = conf.sqlite3Path;
    _conf.hashSeed = conf.hashSeed;
    stopped = false;
    app = new WebrtcNode(c);
  }

  void start() {
//    std::mutex m;
//    std::condition_variable cv;
//    std::unique_lock<std::mutex> lkt(m);
//    std::atomic<bool> inited(false);
//
//    auto t = std::thread([this, &cv, &inited]() {
//      inited = true;
//      cv.notify_all();
    app->start();
//    });
//
//    while (!inited)
//      cv.wait(lkt);
  }


  void addCallbackToRoom(const std::string &roomId, messageHandler *callback) {

    app->addCallback(roomId, callback);
  }

  bool removeCallbackFromRoom(const std::string &roomId, messageHandler *callback) {
    app->removeCallback(roomId, callback);
  }

  void addCallbackToRoom(const std::string &roomId, std::shared_ptr<messageHandler> callback) {
    app->addCallback(roomId, callback);
  }

  bool removeCallbackFromRoom(const std::string &roomId, std::shared_ptr<messageHandler> callback) {
    app->removeCallback(roomId, callback);
  }
  void joinRoom(const std::string &roomId) {
    app->joinRoom(roomId);
  }

  void leaveRoom(const std::string &roomId) {
    app->leaveRoom(roomId);
  }

  void send2client(const std::string &clientId, const std::string &roomId, const std::string &message) {
    app->send2client(clientId, roomId, message);
  }

  void broadcast2Room(const std::string &roomId, const std::string &message) {
    app->broadcast2Room(roomId, message);
  }

  roomIdsVector getRoomIdsVector() {
    auto roomIdsArray = app->getRoomIds();
    roomIdsVector result;
    result.length = roomIdsArray.size();
    result.roomIds = roomIdsArray;
    return result;
  }

  ~signalerWrapper() {
    std::cout << "Destructor2 is here" << std::endl;
    if (!stopped)
      app->stopapp();
    //rtc::CleanupSSL();
    //app->stopapp();
    if (app)
      delete app;
    std::cout << "Destructor2 is ended" << std::endl;
  }

  nodeConfig getNodeConfig() const {
    return _conf;
  }

  std::vector<std::string> getNodeIds(const std::string &roomId) const {
    return app->getNodeIds(roomId);
  }

  void exit() {
    app->stopapp();
    stopped = true;
  }

private:
  WebrtcNode *app;
  nodeConfig _conf;
  bool stopped;
};

messageBroker::messageBroker(nodeConfig conf) : _conf(conf) {
  api = new signalerWrapper(conf);
}

messageBroker::~messageBroker() {
  std::cout << "Destructor is here" << std::endl;
  if (api)
    delete api;
  std::cout << "Destructor is ended" << std::endl;
}

void messageBroker::start() {
  if (api)
    api->start();
}

void messageBroker::addCallbackToRoom(const std::string &roomId, messageHandler *callback) {
  api->addCallbackToRoom(roomId, callback);
}

void messageBroker::joinRoom(const std::string &roomId) {
  api->joinRoom(roomId);
}

void messageBroker::send2client(const std::string &clientId, const std::string &roomId, const std::string &message) {
  api->send2client(clientId, roomId, message);
}

roomIdsVector messageBroker::getRoomIdsVector() {
  return api->getRoomIdsVector();
}

void messageBroker::broadcast2Room(const std::string &roomId, const std::string &message) {
  api->broadcast2Room(roomId, message);
}

void messageBroker::leaveRoom(const std::string &roomId) {
  api->leaveRoom(roomId);
}

nodeConfig messageBroker::getNodeConfig() const {
  return api->getNodeConfig();
}

std::vector<std::string> messageBroker::getNodeIds(const std::string &roomId) const {
  return api->getNodeIds(roomId);
}

void messageBroker::exit() {
  api->exit();
}

std::shared_ptr<messageBroker> messageBroker::Create(nodeConfig conf) {
  std::mutex m;
  std::condition_variable cv;
  std::unique_lock<std::mutex> lkt(m);
  std::atomic<bool> inited(false);
  std::shared_ptr<messageBroker> MB;
  std::thread([&cv, &MB, &inited, &conf]() {
    MB = std::make_shared<messageBroker>(conf);
    inited = true;
    cv.notify_all();
    MB->start();
    std::cout << "I am here" << std::endl;
  }).detach();

  while (!inited)
    cv.wait(lkt);
  sleep(250);
  return MB;
}


void messageBroker::addCallbackToRoom(const std::string &roomId, std::shared_ptr<messageHandler> callback) {
  api->addCallbackToRoom(roomId, callback);
}

bool messageBroker::removeCallbackFromRoom(const std::string &roomId, std::shared_ptr<messageHandler> callback) {
  return api->removeCallbackFromRoom(roomId, callback);
}


bool messageBroker::removeCallbackFromRoom(const std::string &roomId, messageHandler *callback) {
  return api->removeCallbackFromRoom(roomId, callback);
}