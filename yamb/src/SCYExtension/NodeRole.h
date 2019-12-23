//
// Created by panda on 01.07.19.
//

#ifndef WEBRTCSTREAMER_NODEROLE_H
#define WEBRTCSTREAMER_NODEROLE_H

#include <string>
#include "../../webrtc/peermanager.h"
#include "api/media_stream_track_proxy.h"

class NodeRole {
public:
  NodeRole(std::string u, std::string n);

  virtual ~NodeRole() {};

  virtual bool isInitiator() const;

  virtual bool isHeInitiator(const std::string &u) const;

  virtual void OnCreateOffer(scy::wrtc::Peer *, scy::wrtc::PeerFactoryContext *) = 0;

  virtual void onReceiveAnswer(scy::wrtc::Peer *, scy::wrtc::PeerFactoryContext *) = 0;

  virtual void onStable(scy::wrtc::Peer *conn) = 0;

  virtual void onClosed(scy::wrtc::Peer *conn) = 0;

  virtual void onFailure(scy::wrtc::Peer *conn) = 0;

  virtual void onStartStream(const std::string &uri, bool looping, bool realtime) {}

  // virtual rtc::scoped_refptr<scy::wrtc::AudioPacketModule> getAudioModule(){}

  virtual void onAddRemoteStream(webrtc::MediaStreamInterface *stream) {}

  virtual void onNewStream(const std::string &) {}

  virtual void onRemoveRemoteStream(scy::wrtc::Peer *conn, webrtc::MediaStreamInterface *stream) {}


protected:
  std::string _user;
  std::string _name;
};


#endif //WEBRTCSTREAMER_NODEROLE_H
