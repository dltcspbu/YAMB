///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier:	LGPL-2.1+
//
/// @addtogroup webrtc
/// @{


#ifndef SCY_WebRTC_StreamRecorder_H
#define SCY_WebRTC_StreamRecorder_H


#include "../base/base.h"
#include <functional>
#ifdef HAVE_FFMPEG

#include "scy/av/av.h"
#include "scy/av/multiplexencoder.h"

#include "api/peerconnectioninterface.h"


namespace scy {
namespace wrtc {


class StreamRecorder : public rtc::VideoSinkInterface<webrtc::VideoFrame>,
                       public webrtc::AudioTrackSinkInterface
{
public:
    StreamRecorder(const av::EncoderOptions& options);
    StreamRecorder(const av::EncoderOptions& options, std::function<void(const webrtc::VideoFrame&)> f);
    ~StreamRecorder();

    void setVideoTrack(webrtc::VideoTrackInterface* track);
    void setAudioTrack(webrtc::AudioTrackInterface* track);

    void cleanup() {
      _encoder.uninit();
    }
    /// VideoSinkInterface implementation
    void OnFrame(const webrtc::VideoFrame& frame) override;

    /// AudioTrackSinkInterface implementation
    void OnData(const void* audio_data, int bits_per_sample, int sample_rate,
                size_t number_of_channels, size_t number_of_frames) override;

protected:
    av::MultiplexEncoder _encoder;
    rtc::scoped_refptr<webrtc::VideoTrackInterface> _videoTrack;
    rtc::scoped_refptr<webrtc::AudioTrackInterface> _audioTrack;
    bool _awaitingVideo = false;
    bool _awaitingAudio = false;
    bool _shouldInit = true;
    std::function<void(const webrtc::VideoFrame&)> _f;
};


} } // namespace scy::wrtc


#endif // HAVE_FFMPEG
#endif


/// @\}
