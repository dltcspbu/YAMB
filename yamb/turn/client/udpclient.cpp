///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <https://sourcey.com>
//
// SPDX-License-Identifier: LGPL-2.1+
//
/// @addtogroup turn
/// @{


#include "udpclient.h"
#include "../../net/udpsocket.h"


using namespace std;


namespace scy {
namespace turn {


UDPClient::UDPClient(ClientObserver& observer, const Options& options)
    : Client(observer, options, net::makeSocket<net::UDPSocket>())
{
    LTrace("Create")
}


UDPClient::~UDPClient()
{
    LTrace("Destroy")
}


} } // namespace scy::turn


/// @\}
