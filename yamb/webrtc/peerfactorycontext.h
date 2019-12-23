///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier: LGPL-2.1+
//
/// @addtogroup webrtc
/// @{


#ifndef SCY_WebRTC_PeerFactoryContext_H
#define SCY_WebRTC_PeerFactoryContext_H


#include "webrtc.h"

#include "pc/peer_connection_factory.h"
#include "media/engine/multiplex_codec_factory.h"

namespace scy {
  namespace wrtc {


    class PeerFactoryContext {
    public:
      PeerFactoryContext(
          webrtc::AudioDeviceModule *default_adm = nullptr,
          std::unique_ptr<webrtc::MultiplexEncoderFactory> video_encoder_factory = nullptr,
          std::unique_ptr<webrtc::MultiplexDecoderFactory> video_decoder_factory = nullptr,
          rtc::scoped_refptr<webrtc::AudioEncoderFactory> audio_encoder_factory = nullptr,
          rtc::scoped_refptr<webrtc::AudioDecoderFactory> audio_decoder_factory = nullptr);

      void initCustomNetworkManager();

      void cleanup() {
        workerThread->Quit();
        networkThread->Quit();
        if (rtc::Thread::Current())
          rtc::Thread::Current()->Quit();

        if (factory) {
          // errors occur here, either with setting to nullptr or with threads
          // quitting if I quit the threads after setting my factory to a nullptr
          factory = nullptr;
        }
      }

      std::unique_ptr<rtc::Thread> networkThread;
      std::unique_ptr<rtc::Thread> workerThread;
      std::unique_ptr<rtc::NetworkManager> networkManager;
      std::unique_ptr<rtc::PacketSocketFactory> socketFactory;
      rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory;
      // rtc::scoped_refptr<webrtc::AudioDeviceModule> audioDeviceManager;
    };


  }
} // namespace scy::wrtc


#endif


/// @\}
