// Copyright (c) 2022 by Apex.AI Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "iceoryx_posh/runtime/service_discovery.hpp"
#include "iceoryx_posh/testing/roudi_gtest.hpp"

using namespace iox;
using namespace iox::runtime;

extern "C" {
#include "iceoryx_binding_c/publisher.h"
#include "iceoryx_binding_c/runtime.h"
#include "iceoryx_binding_c/service_discovery.h"
}

namespace
{
using namespace ::testing;
using description_vector = cxx::vector<iox_service_description_t, MAX_FINDSERVICE_RESULT_SIZE>;

class iox_service_discovery_test : public RouDi_GTest
{
  public:
    void SetUp() override
    {
        iox_runtime_init("runtime");
        sut = iox_service_discovery_init(&sutStorage);
    }

    void TearDown() override
    {
        searchResult.clear();
        iox_service_discovery_deinit(sut);
    }

    iox_service_discovery_storage_t sutStorage;
    iox_service_discovery_t sut;

    static description_vector searchResult;
    static void findHandler(const iox_service_description_t s, void* contextData)
    {
        static_cast<description_vector*>(contextData)->emplace_back(s);
    }
};

description_vector iox_service_discovery_test::searchResult;

TEST(iox_service_discovery_DeathTest, InitServiceDiscoveryWithNullptrForStorageTerminates)
{
    ::testing::Test::RecordProperty("TEST_ID", "be551a9e-7dcf-406a-a74c-7dcb1ee16c30");
    EXPECT_DEATH({ iox_service_discovery_init(nullptr); }, ".*");
}

/// @note We test only if the arguments of iox_service_discovery_find_service are correctly passed to
/// ServiceDiscovery::findService.
TEST_F(iox_service_discovery_test, FindServiceWithContextDataWithNullptrsForServiceInstanceEventReturnsAllServices)
{
    ::testing::Test::RecordProperty("TEST_ID", "09a2cd6c-fba9-4b9d-af96-c5a6cc168d98");
    iox_service_discovery_find_service_with_context_data(sut, nullptr, nullptr, nullptr, findHandler, &searchResult);
    for (const auto& service : searchResult)
    {
        EXPECT_THAT(service.instanceString, StrEq("RouDi_ID"));
    }
}

TEST_F(iox_service_discovery_test, FindServiceWithContextDataReturnsOfferedService)
{
    ::testing::Test::RecordProperty("TEST_ID", "bb12e514-e7af-4946-b098-98b3cd0f43a5");
    iox_pub_options_t options;
    iox_pub_options_init(&options);
    iox_pub_storage_t storage;
    auto* publisher = iox_pub_init(&storage, "service", "instance", "event", &options);
    ASSERT_NE(publisher, nullptr);
    const iox_service_description_t SERVICE_DESCRIPTION = iox_pub_get_service_description(publisher);

    iox_service_discovery_find_service_with_context_data(sut,
                                                         SERVICE_DESCRIPTION.serviceString,
                                                         SERVICE_DESCRIPTION.instanceString,
                                                         SERVICE_DESCRIPTION.eventString,
                                                         findHandler,
                                                         &searchResult);
    ASSERT_THAT(searchResult.size(), Eq(1U));
    EXPECT_THAT(*searchResult.begin()->serviceString, Eq(*SERVICE_DESCRIPTION.serviceString));
    EXPECT_THAT(*searchResult.begin()->instanceString, Eq(*SERVICE_DESCRIPTION.instanceString));
    EXPECT_THAT(*searchResult.begin()->eventString, Eq(*SERVICE_DESCRIPTION.eventString));
}

TEST_F(iox_service_discovery_test, FindServiceWithNullptrsForServiceInstanceEventReturnsAllServices)
{
    const uint64_t SERVICE_CONTAINER_CAPACITY = 10U;
    iox_service_description_t serviceContainer[SERVICE_CONTAINER_CAPACITY];
    uint64_t missedServices = 0U;
    const auto numberFoundServices = iox_service_discovery_find_service(
        sut, nullptr, nullptr, nullptr, serviceContainer, SERVICE_CONTAINER_CAPACITY, &missedServices);

    EXPECT_THAT(numberFoundServices, Eq(6U));
    EXPECT_THAT(missedServices, Eq(0U));
    for (uint64_t i = 0U; i < numberFoundServices; ++i)
    {
        EXPECT_THAT(serviceContainer[i].instanceString, StrEq("RouDi_ID"));
    }
}

TEST_F(iox_service_discovery_test, FindServiceReturnsOfferedService)
{
    iox_pub_options_t options;
    iox_pub_options_init(&options);
    iox_pub_storage_t storage;
    auto* publisher = iox_pub_init(&storage, "service", "instance", "event", &options);
    ASSERT_NE(publisher, nullptr);
    const iox_service_description_t SERVICE_DESCRIPTION = iox_pub_get_service_description(publisher);

    const uint64_t SERVICE_CONTAINER_CAPACITY = 10U;
    iox_service_description_t serviceContainer[SERVICE_CONTAINER_CAPACITY];
    uint64_t missedServices = 0U;
    const auto numberFoundServices = iox_service_discovery_find_service(sut,
                                                                        SERVICE_DESCRIPTION.serviceString,
                                                                        SERVICE_DESCRIPTION.instanceString,
                                                                        SERVICE_DESCRIPTION.eventString,
                                                                        serviceContainer,
                                                                        SERVICE_CONTAINER_CAPACITY,
                                                                        &missedServices);

    EXPECT_THAT(numberFoundServices, Eq(1U));
    EXPECT_THAT(missedServices, Eq(0U));
    EXPECT_THAT(serviceContainer[0U].serviceString, StrEq(SERVICE_DESCRIPTION.serviceString));
    EXPECT_THAT(serviceContainer[0U].instanceString, StrEq(SERVICE_DESCRIPTION.instanceString));
    EXPECT_THAT(serviceContainer[0U].eventString, StrEq(SERVICE_DESCRIPTION.eventString));
}

TEST_F(iox_service_discovery_test, FindServiceReturnsCorrectNumberOfServicesWhenServiceContainerTooSmall)
{
    const uint64_t SERVICE_CONTAINER_CAPACITY = 3U;
    iox_service_description_t serviceContainer[SERVICE_CONTAINER_CAPACITY];
    uint64_t missedServices = 0U;
    const auto numberFoundServices = iox_service_discovery_find_service(
        sut, nullptr, nullptr, nullptr, serviceContainer, SERVICE_CONTAINER_CAPACITY, &missedServices);

    EXPECT_THAT(numberFoundServices, Eq(SERVICE_CONTAINER_CAPACITY));
    EXPECT_THAT(missedServices, Eq(6U - SERVICE_CONTAINER_CAPACITY));
}

} // namespace
