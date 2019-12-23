//
// Created by panda on 01.07.19.
//

#include "Destination.h"


Destination::Destination(std::string u, std::string n) : NodeRole(u, n) {}

void Destination::OnCreateOffer(scy::wrtc::Peer *p, scy::wrtc::PeerFactoryContext *c) {
//  p->constraints().SetMandatoryReceiveVideo(false);
//  p->constraints().SetMandatoryReceiveAudio(false);
}

void Destination::onReceiveAnswer(scy::wrtc::Peer *p, scy::wrtc::PeerFactoryContext *c) {
//  p->constraints().SetMandatoryReceiveVideo(false);
//  p->constraints().SetMandatoryReceiveAudio(false);
}

void Destination::onStable(scy::wrtc::Peer *conn) {
}

void Destination::onClosed(scy::wrtc::Peer *conn) {
}

void Destination::onFailure(scy::wrtc::Peer *conn) {
}
