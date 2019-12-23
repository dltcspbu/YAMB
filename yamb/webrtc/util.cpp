///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier:	LGPL-2.1+
//
/// @addtogroup webrtc
/// @{


#include "util.h"


namespace scy {
  namespace wrtc {


    std::vector<std::string> getVideoCaptureDevices() {
      std::vector<std::string> deviceNames;
      std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
          webrtc::VideoCaptureFactory::CreateDeviceInfo());
      if (!info) {
        return deviceNames;
      }
      int numDevicess = info->NumberOfDevices();
      assert(numDevicess > 0);
      for (int i = 0; i < numDevicess; ++i) {
        const uint32_t kSize = 256;
        char name[kSize] = {0};
        char id[kSize] = {0};
        if (info->GetDeviceName(i, name, kSize, id, kSize) != -1) {
          deviceNames.push_back(name);
        }
      }
      return deviceNames;
    }

    std::vector<Device> getVideoCaptureDevicesStruct() {
      std::vector<Device> deviceNames;
      std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
          webrtc::VideoCaptureFactory::CreateDeviceInfo());
      if (!info) {
        return deviceNames;
      }
      int numDevicess = info->NumberOfDevices();
      assert(numDevicess > 0);
      for (int i = 0; i < numDevicess; ++i) {
        const uint32_t kSize = 256;
//        char name[kSize] = {0};
//        char id[kSize] = {0};
        Device d;
        if (info->GetDeviceName(i, d.name, kSize, d.id, kSize) != -1) {

          deviceNames.push_back(d);
        }
      }
      return deviceNames;
    }

// webrtc::VideoCaptureModule* openWebRtcVideoCaptureDevice(const std::string& deviceName)
    rtc::scoped_refptr<webrtc::VideoCaptureModule> openWebRtcVideoCaptureDevice(const std::string &deviceName) {
      auto devices = getVideoCaptureDevicesStruct();

      // std::unique_ptr<webrtc::VideoCaptureModule> capturer;
      rtc::scoped_refptr<webrtc::VideoCaptureModule> capturer = nullptr;

      for (const auto &d : devices) {
        if (std::string(d.name) == deviceName) {
          //rtc::scoped_refptr<webrtc::VideoCaptureModule> factory =
          capturer = webrtc::VideoCaptureFactory::Create(d.id);
          return capturer;
        }
      }
      if (!capturer && !devices.empty()) {
        return webrtc::VideoCaptureFactory::Create(devices[0].id);
      }

      assert(0 && "no video devices");
      return nullptr;
    }


  }
} // namespace scy::wrtc


/// @\}
