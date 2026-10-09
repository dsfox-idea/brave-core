/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "net/proxy_resolution/configured_proxy_resolution_service.h"

#include <memory>
#include <optional>
#include <string>

#include "brave/net/proxy_resolution/proxy_config_service_tor.h"
#include "net/base/host_port_pair.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/proxy_server.h"
#include "net/base/schemeful_site.h"
#include "net/base/test_completion_callback.h"
#include "net/dns/mock_host_resolver.h"
#include "net/log/net_log_source_type.h"
#include "net/log/net_log_with_source.h"
#include "net/proxy_resolution/mock_proxy_resolver.h"
#include "net/proxy_resolution/proxy_resolution_request.h"
#include "net/test/gtest_util.h"
#include "net/test/test_with_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using net::test::IsOk;

namespace net {

class ConfiguredProxyResolutionServiceTest : public TestWithTaskEnvironment {
 public:
  ConfiguredProxyResolutionServiceTest() = default;
  ConfiguredProxyResolutionServiceTest(
      const ConfiguredProxyResolutionServiceTest&) = delete;
  ConfiguredProxyResolutionServiceTest& operator=(
      const ConfiguredProxyResolutionServiceTest&) = delete;
  ~ConfiguredProxyResolutionServiceTest() override = default;

  void SetUp() override {
    const std::string proxy_uri("socks5://127.0.0.1:5566");
    service_ = std::make_unique<ConfiguredProxyResolutionService>(
        std::make_unique<ProxyConfigServiceTor>(proxy_uri),
        std::make_unique<MockAsyncProxyResolverFactory>(false),
        mock_host_resolver_.get(), nullptr,
        /*quick_check_enabled=*/true);
  }

  ConfiguredProxyResolutionService* GetProxyResolutionService() {
    return service_.get();
  }

 private:
  std::unique_ptr<MockHostResolverBase> mock_host_resolver_{nullptr};
  std::unique_ptr<ConfiguredProxyResolutionService> service_;
};

TEST_F(ConfiguredProxyResolutionServiceTest, TorProxy) {
  ConfiguredProxyResolutionService* service = GetProxyResolutionService();
  const GURL url("https://check.torproject.org/");
  const std::string circuit_anonymization_key =
      ProxyConfigServiceTor::CircuitAnonymizationKey(url);
  const SchemefulSite url_site(url);
  const auto network_anonymization_key =
      NetworkAnonymizationKey::CreateFromFrameSite(url_site, url_site);

  ProxyInfo info;
  TestCompletionCallback callback;
  std::unique_ptr<ProxyResolutionRequest> request;
  int rv = service->ResolveProxy(
      url, std::string(), network_anonymization_key,
      handles::kInvalidNetworkHandle, &info, callback.callback(), &request,
      NetLogWithSource::Make(NetLogSourceType::NONE), DEFAULT_PRIORITY);
  EXPECT_THAT(rv, IsOk());

  ProxyServer server = info.proxy_chain().GetProxyServer(/*chain_index=*/0);
  HostPortPair host_port = server.host_port_pair();
  EXPECT_TRUE(server.scheme() == ProxyServer::SCHEME_SOCKS5);
  EXPECT_EQ(host_port.host(), "127.0.0.1");
  EXPECT_EQ(host_port.port(), 5566);
  EXPECT_EQ(host_port.username(), circuit_anonymization_key);
  EXPECT_FALSE(host_port.password().empty());
}

// Growser-260: a Tor window closed and a new one opened get services that the
// allocator may place at the same address, and the circuit passwords are
// keyed by that address. Construct the second service in the first one's
// storage, so the reuse is certain rather than likely, and expect a fresh
// circuit for the same site.
TEST_F(ConfiguredProxyResolutionServiceTest,
       NewServiceAtSameAddressGetsNewCircuit) {
  const GURL url("https://check.torproject.org/");
  const SchemefulSite url_site(url);
  const auto network_anonymization_key =
      NetworkAnonymizationKey::CreateFromFrameSite(url_site, url_site);

  std::optional<ConfiguredProxyResolutionService> storage;
  const ConfiguredProxyResolutionService* first_address = nullptr;
  auto resolve_in_storage = [&] {
    ConfiguredProxyResolutionService& service = storage.emplace(
        std::make_unique<ProxyConfigServiceTor>("socks5://127.0.0.1:5566"),
        std::make_unique<MockAsyncProxyResolverFactory>(false), nullptr,
        nullptr, /*quick_check_enabled=*/true);
    if (!first_address) {
      first_address = &service;
    }
    EXPECT_EQ(&service, first_address);
    ProxyInfo info;
    TestCompletionCallback callback;
    std::unique_ptr<ProxyResolutionRequest> request;
    EXPECT_THAT(
        service.ResolveProxy(
            url, std::string(), network_anonymization_key,
            handles::kInvalidNetworkHandle, &info, callback.callback(),
            &request, NetLogWithSource::Make(NetLogSourceType::NONE),
            DEFAULT_PRIORITY),
        IsOk());
    request.reset();
    storage.reset();
    return info.proxy_chain()
        .GetProxyServer(/*chain_index=*/0)
        .host_port_pair()
        .password();
  };

  const std::string first = resolve_in_storage();
  const std::string second = resolve_in_storage();
  EXPECT_FALSE(first.empty());
  EXPECT_FALSE(second.empty());
  EXPECT_NE(first, second);
}

}  // namespace net
