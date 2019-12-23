///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier: LGPL-2.1+
//
/// @addtogroup net
/// @{


#ifndef SCY_Net_DNS_H
#define SCY_Net_DNS_H


#include "net.h"
#include "address.h"
#include "../base/request.h"
#include "../base/logger.h"
#include "../base/util.h"


namespace scy {
namespace net {

/// DNS utilities.
namespace dns {


inline auto resolve(const std::string& host, int port,
                    std::function<void(int,const net::Address&)> callback,
                    uv::Loop* loop = uv::defaultLoop())
{
    return uv::createRequest<uv::GetAddrInfoReq>([=](const uv::GetAddrInfoEvent& event) {
        if (event.status) {
            LWarn("Cannot resolve DNS for ", host, ": ", uv_strerror(event.status))
            callback(event.status, net::Address{});
        }
        else
            callback(event.status, net::Address{event.addr->ai_addr, static_cast<socklen_t>(event.addr->ai_addrlen)});
    }).resolve(host, port, loop);
}


} // namespace dns
} // namespace net
} // namespace scy


#endif // SCY_Net_DNS_H


/// @\}
