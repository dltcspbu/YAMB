//
// Created by panda on 01.07.19.
//

#include "NodeRole.h"
#include "Source.h"

NodeRole::NodeRole(std::string u, std::string n) : _user(u), _name(n) {

}

bool NodeRole::isInitiator() const {
  return this->_user.find("stream") != std::string::npos;
}

bool NodeRole::isHeInitiator(const std::string &u) const {
  return u.find("stream") != std::string::npos;
}

void Source::OnCreateOffer(scy::wrtc::Peer *p, scy::wrtc::PeerFactoryContext *c) {
//  p->constraints().SetMandatoryReceiveVideo(true);
//  p->constraints().SetMandatoryReceiveAudio(false);
//  p->constraints().SetAllowDtlsSctpDataChannels();
}

void Source::onReceiveAnswer(scy::wrtc::Peer *p, scy::wrtc::PeerFactoryContext *c) {
//  p->constraints().SetMandatoryReceiveVideo(true);
//  p->constraints().SetMandatoryReceiveAudio(false);
//  p->constraints().SetAllowDtlsSctpDataChannels();
}


Source::Source(std::string u, std::string n) : NodeRole(u, n) {

}

void Source::onStable(scy::wrtc::Peer *conn) {

}

void Source::onClosed(scy::wrtc::Peer *conn) {

}

void Source::onFailure(scy::wrtc::Peer *conn) {

}