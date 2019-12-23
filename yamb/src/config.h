#ifndef SCY_WebRTC_WebRTCStreamer_Config_H
#define SCY_WebRTC_WebRTCStreamer_Config_H


#include "../base/base.h"

#define SERVER_HOST "127.0.0.1"
#define USE_SSL 0 // 1
#if USE_SSL
#define SERVER_PORT 443
#else
#define SERVER_PORT 8601
#endif


#endif
