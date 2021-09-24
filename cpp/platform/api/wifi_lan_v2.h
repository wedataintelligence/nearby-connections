// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef PLATFORM_API_WIFI_LAN__V2_H_
#define PLATFORM_API_WIFI_LAN__V2_H_

#include <string>

#include "platform/base/cancellation_flag.h"
#include "platform/base/input_stream.h"
#include "platform/base/listeners.h"
#include "platform/base/nsd_service_info.h"
#include "platform/base/output_stream.h"

namespace location {
namespace nearby {
namespace api {

// Opaque wrapper over a WifiLan service which contains |NsdServiceInfo|.
class WifiLanService {
 public:
  virtual ~WifiLanService() = default;

  // Returns the |NsdServiceInfo| which contains the packed string of
  // |WifiLanServiceInfo| and the endpoint info with named key in a TXTRecord
  // map.
  // The details refer to
  // https://developer.android.com/reference/android/net/nsd/NsdServiceInfo.html.
  virtual NsdServiceInfo GetServiceInfo() const = 0;
};

class WifiLanSocket {
 public:
  virtual ~WifiLanSocket() = default;

  // Returns the InputStream of the WifiLanSocket.
  // On error, returned stream will report Exception::kIo on any operation.
  //
  // The returned object is not owned by the caller, and can be invalidated once
  // the WifiLanSocket object is destroyed.
  virtual InputStream& GetInputStream() = 0;

  // Returns the OutputStream of the WifiLanSocket.
  // On error, returned stream will report Exception::kIo on any operation.
  //
  // The returned object is not owned by the caller, and can be invalidated once
  // the WifiLanSocket object is destroyed.
  virtual OutputStream& GetOutputStream() = 0;

  // Returns Exception::kIo on error, Exception::kSuccess otherwise.
  virtual Exception Close() = 0;

  // Returns valid WifiLanService pointer if there is a connection, and
  // nullptr otherwise.
  virtual WifiLanService* GetRemoteService() = 0;
};

class WifiLanServerSocket {
 public:
  virtual ~WifiLanServerSocket() = default;

  // Blocks until either:
  // - at least one incoming connection request is available, or
  // - ServerSocket is closed.
  // On success, returns connected socket, ready to exchange data.
  // Returns nullptr on error.
  // Once error is reported, it is permanent, and ServerSocket has to be closed.
  virtual std::unique_ptr<WifiLanSocket> Accept() = 0;

  // Returns Exception::kIo on error, Exception::kSuccess otherwise.
  virtual Exception Close() = 0;
};

// Container of operations that can be performed over the WifiLan medium.
class WifiLanMediumV2 {
 public:
  virtual ~WifiLanMediumV2() = default;

  // Turns on WifiLan advertising.
  //
  // nsd_service_info - NsdServiceInfo data that's advertised through mDNS
  //                    service.
  // On success if the service is now discoverable.
  // On error if the service is not discoverable.
  virtual bool StartAdvertising(const NsdServiceInfo& nsd_service_info) = 0;

  // Returns true once WifiLan is stopped;
  virtual bool StopAdvertising(const NsdServiceInfo& nsd_service_info) = 0;

  // Callback that is invoked when a discovered service is found or lost.
  struct DiscoveredServiceCallback {
    std::function<void(WifiLanService& wifi_lan_service)>
        service_discovered_cb = DefaultCallback<WifiLanService&>();
    std::function<void(WifiLanService& wifi_lan_service)> service_lost_cb =
        DefaultCallback<WifiLanService&>();
  };

  // Starts the discovery of nearby WifiLan services.
  // Returns true once the WifiLan discovery has been initiated.
  virtual bool StartDiscovery(DiscoveredServiceCallback callback) = 0;

  // Returns true once WifiLan discovery is well and truly stopped; after this
  // returns, there must be no more invocations of the DiscoveredServiceCallback
  // passed in to StartDiscovery().
  // callback - The one passed in StartDiscovery. If the callback instance is
  //            different then the StopDiscovery won't take effect.
  virtual bool StopDiscovery(DiscoveredServiceCallback callback) = 0;

  // Connects to a WifiLan service.
  // On success, returns a new WifiLanSocket.
  // On error, returns nullptr.
  virtual std::unique_ptr<WifiLanSocket> ConnectToService(
      WifiLanService& remote_service, CancellationFlag* cancellation_flag) = 0;

  // Listens for incoming connection.
  //
  // service_uuid - A uuid for current server socket identifer.
  // On success, returns a new WifiLanServerSocket.
  // On error, returns nullptr.
  virtual std::unique_ptr<WifiLanServerSocket> ListenForService(
      const std::string& service_uuid) = 0;

  virtual WifiLanService* GetRemoteService(const std::string& ip_address,
                                           int port) = 0;

  // Return pair of ip address and port.
  virtual std::pair<std::string, int> GetServiceAddress() = 0;
};

}  // namespace api
}  // namespace nearby
}  // namespace location

#endif  // PLATFORM_API_WIFI_LAN__V2_H_
