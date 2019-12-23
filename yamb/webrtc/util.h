///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier:	LGPL-2.1+
///
/// @addtogroup webrtc
/// @{


#ifndef SCY_WebRTC_WEBRTC_UTIL_H
#define SCY_WebRTC_WEBRTC_UTIL_H


#include "webrtc.h"

//#include "media/base/videocapturer.h"
//#include "media/engine/webrtcvideocapturerfactory.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"
#include "modules/video_capture/video_capture_factory.h"


namespace scy {
  namespace wrtc {


    struct Device {
      static const uint32_t kSize = 256;
      char name[kSize] = {0};
      char id[kSize] = {0};
    };

    std::vector<std::string> getVideoCaptureDevices();

    std::vector<Device> getVideoCaptureDevicesStruct();

    rtc::scoped_refptr<webrtc::VideoCaptureModule> openWebRtcVideoCaptureDevice(const std::string &deviceName = "");


  }
} // namespace scy::wrtc


#endif // SCY_WebRTC_WEBRTC_H


/// @\}
