//
// Created by panda on 01.07.19.
//

#ifndef OPENCVANALYZER_DESTINATION_H
#define OPENCVANALYZER_DESTINATION_H

#include "NodeRole.h"

class Destination : public NodeRole {
public:
  Destination(std::string u, std::string n);

  ~Destination() override {}

  void OnCreateOffer(scy::wrtc::Peer *, scy::wrtc::PeerFactoryContext *) override;

  void onReceiveAnswer(scy::wrtc::Peer *, scy::wrtc::PeerFactoryContext *) override;

  void onStable(scy::wrtc::Peer *conn) override;

  void onClosed(scy::wrtc::Peer *conn) override;

  void onFailure(scy::wrtc::Peer *conn) override;
};


#endif //OPENCVANALYZER_DESTINATION_H
