/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "DeviceStoreImpl"
#include "device_store_impl.h"

#include <endian.h>
#include <regex>

#include "dev_manager.h"
#include "log_print.h"
namespace OHOS::DistributedKv {
std::vector<uint8_t> DeviceStoreImpl::ToLocalDBKey(const Key &key) const
{
    return ToLocal(key, true);
}

std::vector<uint8_t> DeviceStoreImpl::ToLocal(const Key &in, bool withLen) const
{
    auto uuid = DevManager::GetInstance().GetLocalDevice().uuid;
    if (uuid.empty()) {
        return {};
    }

    std::vector<uint8_t> input = withLen ? SingleStoreImpl::GetPrefix(in) : in.Data();
    if (input.empty()) {
        return {};
    }

    // |local uuid|original key|uuid len|
    // |---- -----|------------|---4----|
    std::vector<uint8_t> dbKey;
    dbKey.insert(dbKey.end(), uuid.begin(), uuid.end());
    dbKey.insert(dbKey.end(), input.begin(), input.end());
    if (withLen) {
        uint32_t length = uuid.length();
        length = htole32(length);
        uint8_t *buf = reinterpret_cast<uint8_t *>(&length);
        dbKey.insert(dbKey.end(), buf, buf + sizeof(length));
    }
    return dbKey;
}

std::vector<uint8_t> DeviceStoreImpl::ToWholeDBKey(const Key &key) const
{
    // | device uuid | original key | uuid len |
    // |-------------|--------------|-----4----|
    return ConvertNetwork(key, true);
}

Key DeviceStoreImpl::ToKey(DBKey &&key) const
{
    // |  uuid    |original key|uuid len|
    // |---- -----|------------|---4----|
    if (key.size() < sizeof(uint32_t)) {
        return std::move(key);
    }

    uint32_t length = *(reinterpret_cast<uint32_t *>(&(*(key.end() - sizeof(uint32_t)))));
    if (length > key.size() - sizeof(uint32_t)) {
        return std::move(key);
    }

    length = le32toh(length);
    key.erase(key.begin(), key.begin() + length);
    key.erase(key.end() - sizeof(uint32_t), key.end());
    return std::move(key);
}

std::vector<uint8_t> DeviceStoreImpl::GetPrefix(const Key &prefix) const
{
    // |  uuid    |original key|
    // |---- -----|------------|
    return ConvertNetwork(prefix);
}

std::vector<uint8_t> DeviceStoreImpl::GetPrefix(const DataQuery &query) const
{
    std::vector<uint8_t> prefix;
    uint32_t length = query.deviceId_.size();
    prefix.insert(prefix.end(), &length, &length + sizeof(length));
    prefix.insert(prefix.end(), query.deviceId_.begin(), query.deviceId_.end());
    prefix.insert(prefix.end(), query.prefix_.begin(), query.prefix_.end());
    // |  uuid    |original key|
    // |---- -----|------------|
    return ConvertNetwork(std::move(prefix));
}

SingleStoreImpl::Convert DeviceStoreImpl::GetConvert() const
{
    return [](const DBKey &key, std::string &deviceId) {
        if (key.size() < sizeof(uint32_t)) {
            return std::move(key);
        }

        uint32_t length = *(reinterpret_cast<const uint32_t *>(&(*(key.end() - sizeof(uint32_t)))));
        if (length > key.size() - sizeof(uint32_t)) {
            return std::move(key);
        }

        length = le32toh(length);
        deviceId = { key.begin(), key.begin() + length };
        Key result(std::vector<uint8_t>(key.begin() + length, key.end() - sizeof(uint32_t)));
        return std::move(key);
    };
}

std::vector<uint8_t> DeviceStoreImpl::ConvertNetwork(const Key &in, bool withLen) const
{
    // input
    // | network ID len | networkID | original key|
    // |--------4-------|-----------|------------|
    if (in.Size() < sizeof(uint32_t)) {
        return ToLocal(in, withLen);
    }
    std::string deviceLen(in.Data().data(), in.Data().data() + sizeof(uint32_t));
    std::regex patten("^[0-9]*$");
    if (!std::regex_match(deviceLen, patten)) {
        return ToLocal(in, withLen);
    }

    uint32_t devLen = atol(deviceLen.c_str());
    if (devLen > in.Data().size() + sizeof(uint32_t)) {
        return ToLocal(in, withLen);
    }

    std::string networkId(in.Data().begin() + sizeof(uint32_t), in.Data().begin() + sizeof(uint32_t) + devLen);
    std::string uuid = DevManager::GetInstance().ToUUID(networkId);
    if (uuid.empty()) {
        return ToLocal(in, withLen);
    }

    // output
    // | device uuid | original key | uuid len |
    // |-------------|--------------|----4-----|
    // |  Mandatory  |   Mandatory  | Optional |
    std::vector<uint8_t> out;
    out.insert(out.end(), uuid.begin(), uuid.end());
    out.insert(out.end(), in.Data().begin() + sizeof(uint32_t) + devLen, in.Data().end());
    if (withLen) {
        uint32_t length = uuid.length();
        length = htole32(length);
        uint8_t *buf = reinterpret_cast<uint8_t *>(&length);
        out.insert(out.end(), buf, buf + sizeof(length));
    }
    return out;
}
}