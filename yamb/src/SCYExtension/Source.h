//
// Created by panda on 01.07.19.
//

#ifndef WEBRTCSTREAMER_SOURCE_H
#define WEBRTCSTREAMER_SOURCE_H

#include "NodeRole.h"

class Source : public NodeRole {
public:
  Source(std::string u, std::string n);

  ~Source() override {}

  void OnCreateOffer(scy::wrtc::Peer *, scy::wrtc::PeerFactoryContext *) override;

  void onReceiveAnswer(scy::wrtc::Peer *, scy::wrtc::PeerFactoryContext *) override;

  void onStable(scy::wrtc::Peer *conn) override;

  void onClosed(scy::wrtc::Peer *conn) override;

  void onFailure(scy::wrtc::Peer *conn) override;
};

#endif //WEBRTCSTREAMER_SOURCE_H
