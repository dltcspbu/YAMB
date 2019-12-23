//#include <stdlib.h>
//#include "scy/logger.h"
//#include "rtc_base/ssladapter.h"
//#include "scy/util.h"
//
//using std::endl;
//using namespace scy;
#include <string>
#include "messageBroker.hpp"
#include <thread>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <functional>
#include <stdlib.h>
#include "CircularBuffer.h"
#include <iomanip>
#include <ctime>
#include <sstream>

#include <cmath>
#include "../json.hpp"


std::shared_ptr<messageBroker> A;

void menu();

void reader(const std::string &);

void writer(const std::string &);

void join(const std::string &);

void leave(const std::string &);


std::map<std::string, CircularBuffer<nlohmann::json, 10>> message_queues;
std::map<std::string, std::condition_variable> message_queues_cv;
std::map<std::string, std::atomic<bool>> message_queues_flags;
std::map<std::string, std::mutex> message_queues_mut;

class MH : public messageHandler {
public:
  MH(const std::string &r) : room(r), messageHandler() {
    message_queues_flags[room].store(false);
  }

  void handle(const std::string &m) override {
    nlohmann::json j = nlohmann::json::parse(m);
    message_queues[room].put(j);
    message_queues_cv[room].notify_all();
  }

private:
  std::string room;
};


template<class F, class... Args>
std::future<void> setInterval(std::atomic_bool &cancelToken, size_t interval, F &&f, Args &&... args) {
  cancelToken.store(true);
  //auto cb = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
  auto fut = std::async(std::launch::async, [=, &cancelToken]()mutable {
    while (cancelToken.load()) {
      //auto ts = std::chrono::system_clock::now();

      auto t = std::time(nullptr);
      auto tm = *std::localtime(&t);
      std::ostringstream oss;
      oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
      auto str = oss.str();

      auto msg =
          "Sendind some important data in string format time stamp=" + str;
      f(msg);
      std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    }
  });
  return fut;
}

void reader(const std::string &roomId) {
  std::cout << "To return to menu press q and enter." << std::endl;
  std::cout << "Reading messages from room " << roomId << "..." << std::endl;
  auto f = [roomId]() {
    while (!message_queues_flags[roomId].load()) {
      std::unique_lock<std::mutex> lock(message_queues_mut[roomId]);
      while (message_queues[roomId].empty() && !message_queues_flags[roomId].load()) {
        message_queues_cv[roomId].wait(lock);
      }
      if (message_queues_flags[roomId].load()) {
        message_queues_flags[roomId].store(false);
        return;
      }
      std::stringstream out;
      auto m = message_queues[roomId].get();
      out << "<" << m["from"].get<std::string>() << "|" << m["roomId"].get<std::string>() << ">: "
          << m["data"].get<std::string>();
      std::cout << out.str() << std::endl;
    }
  };
  auto t = std::thread(f);
  char ch;
  while (true) {
    ch = getchar();
    if (ch == 'q') {
      message_queues_flags[roomId].store(true);
      message_queues_cv[roomId].notify_all();
      if (t.joinable())
        t.join();
      return menu();
    }
  }
}

void writer(const std::string &roomId) {
  getchar();
  std::cout << "To return to menu enter q." << std::endl;
  while (true) {
    std::cout << "Input your message to room " << roomId << ": ";
    std::string msg;
    std::getline(std::cin, msg);
    if (msg == "q")
      break;
    A->broadcast2Room(roomId, msg);
    std::cout << "Message is sent..." << std::endl;
  }
  menu();
}

void join(const std::string &roomId) {
  std::cout << "Joining room: " << roomId << "..." << std::endl;
  auto govno = new MH(roomId);
  A->joinRoom(roomId);
  A->addCallbackToRoom(roomId, govno);
  menu();
}

void leave(const std::string &roomId) {
  std::cout << "Leaving room: " << roomId << "..." << std::endl;
  A->leaveRoom(roomId);
  menu();
}

void menu() {
  char control_ch;
  while (true) {
    std::cout
        << "Main menu: enter 'v' to see list of available rooms, 'j' and enter a roomId you would like to join, 'l' and roomNumber you would like to leave, 'r' to read messages from room, 'w' to write message to room, 'q' to exit"
        << std::endl;
    std::cin >> control_ch;
    if (control_ch == 'v') {
      const auto v = A->getRoomIdsVector();

      std::cout << "List of joined rooms:" << std::endl;
      for (auto i = 0; i < v.length; ++i) {
        std::cout << "[" << i << "]:" << v.roomIds[i] << std::endl;
      }
    }
    if (control_ch == 'r') {
      std::cout << "Input room number which u like to read messages from:" << std::endl;
      int n;
      std::cin >> n;
      const auto v = A->getRoomIdsVector();

      if (n >= 0 && n < v.roomIds.size()) {
        reader(v.roomIds[n]);
      } else {
        std::cout << "Inputed room number is incorrect. Choose digit from " << 0 << " and " << v.roomIds.size() - 1
                  << std::endl;
      }
    }
    if (control_ch == 'j') {
      std::cout << "Input roomId which u like to join:" << std::endl;
      std::string rid;
      std::cin >> rid;
      if (!rid.empty()) {
        join(rid);
      } else {
        std::cout << "Inputed roomId. SHould be non-empty string." << std::endl;
      }
    }
    if (control_ch == 'l') {
      std::cout << "Input room number which u like to leave:" << std::endl;
      int n;
      std::cin >> n;
      const auto v = A->getRoomIdsVector();

      if (n >= 0 && n < v.roomIds.size()) {
        leave(v.roomIds[n]);
      } else {
        std::cout << "Inputed room number is incorrect. Choose digit from " << 0 << " and " << v.roomIds.size() - 1
                  << std::endl;
      }
    }
    if (control_ch == 'w') {
      std::cout << "Input room number in which u like to write messages:" << std::endl;
      int n;
      std::cin >> n;
      const auto v = A->getRoomIdsVector();

      if (n >= 0 && n < v.roomIds.size()) {
        writer(v.roomIds[n]);
      } else {
        std::cout << "Inputed room number is incorrect. Choose digit from " << 0 << " and " << v.roomIds.size() - 1
                  << std::endl;
      }
    }
    if (control_ch == 'q') {
      std::cout << "Exiting..." << std::endl;
      A->exit();
      break;
    }
  }
  std::cout << "Exited" << std::endl;
}


int main(int argc, char **argv) {
  std::string roomId{"test"};
  //A.joinRoom(roomId);
  std::string host{"localhost"};//signaling server IP
  int port = 8601;

  if (argc > 1) {
    host = argv[1];
  }
  if (argc > 2) {
    port = atoi(argv[2]);
  }
  std::string name{"test123"};
  nodeConfig c1;
  c1.WSHost = host;
  c1.WSPort = port;
  //user will be generated inside
  c1.user = "";
  c1.name = name;
  c1.openPortsStart = 0;
  c1.openPortsEnd = 0;
  c1.sqlite3Path = "sqlite3";
  c1.hashSeed = "seedvalue";
  A = messageBroker::Create(c1);
  menu();
//  std::vector<std::shared_ptr<messageBroker>> v;
//  for (int i = 0; i < 10; ++i) {
//    nodeConfig c;
//    c.WSHost = host;
//    c.WSPort = 8601;
//    c.user = "";
//    c.name = name + std::to_string(i);
//    c.openPortsStart = 0;
//    c.openPortsEnd = 0;
//    c.sqlite3Path = "sqlite3";
//    c.hashSeed = "seedvalue";
//    v.push_back(messageBroker::Create(c));
//    std::cout << v.back().use_count() << std::endl;
//    v.back()->joinRoom(roomId);
//    //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//  }
//  std::for_each(v.begin(), v.end(), [](const auto &a) {
//    std::cout << a.use_count() << std::endl;
//  });
//  v.clear();
  return 0;
//  {
//    auto a = messageBroker::Create(c);
//  }
//  auto A1 = messageBroker::Create(c);
//  auto A2 = messageBroker::Create(c);
//  auto A3 = messageBroker::Create(c);
//  auto A4 = messageBroker::Create(c);
//  menu();
  //while (true) {
//    //std::cout << "ok" << std::endl;
  //}

}
