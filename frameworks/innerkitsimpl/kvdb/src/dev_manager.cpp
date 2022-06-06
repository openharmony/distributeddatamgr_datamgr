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
#define LOG_TAG "DevManager"
#include "dev_manager.h"
#include "softbus_bus_center.h"
#include "log_print.h"
#include "store_util.h"
namespace OHOS::DistributedKv {
constexpr int32_t SOFTBUS_OK = 0;
constexpr int32_t ID_BUF_LEN = 65;
DevManager &DevManager::GetInstance()
{
    static DevManager instance;
    return instance;
}

std::string DevManager::ToUUID(const std::string &networkId) const
{
    DevManager::DeviceInfo cacheInfo = GetDeviceCacheInfo(networkId);
    if (cacheInfo.uuid.empty()) {
        ZLOGW("unknown id.");
        return "";
    }
    return cacheInfo.uuid;
}

DevManager::DeviceInfo DevManager::GetLocalDevice()
{
    if (!localInfo_.uuid.empty()) {
        return localInfo_;
    }

    NodeBasicInfo info;
    int32_t ret = GetLocalNodeDeviceInfo("ohos.distributeddata", &info);
    if (ret != SOFTBUS_OK) {
        ZLOGE("GetLocalNodeDeviceInfo error");
        return {};
    }
    std::string uuid = GetUuidByNodeId(std::string(info.networkId));
    std::string udid = GetUdidByNodeId(std::string(info.networkId));
    ZLOGD("[LocalDevice] id:%{private}s, name:%{private}s, type:%{private}d",
          StoreUtil::Anonymous(uuid).c_str(), info.deviceName, info.deviceTypeId);
    localInfo_ = { uuid, udid, info.networkId };
    return localInfo_;
}

std::vector<DevManager::DeviceInfo> DevManager::GetRemoteDevices() const
{
    std::vector<DeviceInfo> dis;
    NodeBasicInfo *info = nullptr;
    int32_t infoNum = 0;
    dis.clear();

    int32_t ret = GetAllNodeDeviceInfo("ohos.distributeddata", &info, &infoNum);
    if (ret != SOFTBUS_OK) {
        ZLOGE("GetAllNodeDeviceInfo error");
        return dis;
    }
    ZLOGD("GetAllNodeDeviceInfo success infoNum=%{public}d", infoNum);

    for (int i = 0; i < infoNum; i++) {
        std::string uuid = GetUuidByNodeId(std::string(info[i].networkId));
        std::string udid = GetUdidByNodeId(std::string(info[i].networkId));
        DeviceInfo deviceInfo = { uuid, udid, info[i].networkId };
        dis.push_back(deviceInfo);
    }
    if (info != nullptr) {
        FreeNodeInfo(info);
    }
    return dis;
}

DevManager::DeviceInfo DevManager::GetDeviceInfo(const std::string &id) const
{
    DevManager::DeviceInfo cacheInfo = GetDeviceCacheInfo(id);
    if (cacheInfo.networkId.empty()) {
        ZLOGE("Get DeviceInfo failed.");
    }
    return cacheInfo;
}

std::string DevManager::ToNodeID(const std::string &nodeId) const
{
    DeviceInfo cacheInfo = GetDeviceCacheInfo(nodeId);
    if (cacheInfo.networkId.empty()) {
        ZLOGW("unknown id.");
        return "";
    }
    return cacheInfo.networkId;
}

std::string DevManager::GetUuidByNodeId(const std::string &nodeId) const
{
    char uuid[ID_BUF_LEN] = {0};
    int32_t ret = GetNodeKeyInfo("ohos.distributeddata", nodeId.c_str(),
                                 NodeDeviceInfoKey::NODE_KEY_UUID, reinterpret_cast<uint8_t *>(uuid), ID_BUF_LEN);
    if (ret != SOFTBUS_OK) {
        ZLOGW("GetNodeKeyInfo error, nodeId:%{public}s", StoreUtil::Anonymous(nodeId).c_str());
        return "";
    }
    return std::string(uuid);
}

std::string DevManager::GetUdidByNodeId(const std::string &nodeId) const
{
    char udid[ID_BUF_LEN] = {0};
    int32_t ret = GetNodeKeyInfo("ohos.distributeddata", nodeId.c_str(),
                                 NodeDeviceInfoKey::NODE_KEY_UDID, reinterpret_cast<uint8_t *>(udid), ID_BUF_LEN);
    if (ret != SOFTBUS_OK) {
        ZLOGW("GetNodeKeyInfo error, nodeId:%{public}s", StoreUtil::Anonymous(nodeId).c_str());
        return "";
    }
    return std::string(udid);
}

DevManager::DeviceInfo DevManager::GetDeviceInfoFromCache(const std::string &id) const
{
    auto deviceInfo = deviceInfos_.Find(id);
    if (deviceInfo.first) {
        return deviceInfo.second;
    }
    ZLOGI("did not get deviceInfo from cache. ");
    return {};
}

DevManager::DeviceInfo DevManager::GetDeviceCacheInfo(const std::string &id) const
{
    DeviceInfo cacheInfo = GetDeviceInfoFromCache(id);
    if (!cacheInfo.networkId.empty()) {
        return cacheInfo;
    }
    UpdateDeviceCacheInfo();
    cacheInfo = GetDeviceInfoFromCache(id);
    return cacheInfo;
}

void DevManager::UpdateDeviceCacheInfo() const
{
    ZLOGW("get the network id from devices.");
    NodeBasicInfo *info = nullptr;
    int32_t infoNum = 0;
    int32_t ret = GetAllNodeDeviceInfo("ohos.distributeddata", &info, &infoNum);  // to find from softbus
    if (ret != SOFTBUS_OK) {
        ZLOGE("GetAllNodeDeviceInfo error");
        return;
    }
    ZLOGD("GetAllNodeDeviceInfo success infoNum=%{public}d", infoNum);
    for (int i = 0; i < infoNum; i++) {
        auto networkId = info[i].networkId;
        auto uuid = GetUuidByNodeId(networkId);
        auto udid = GetUdidByNodeId(networkId);
        DevManager::DeviceInfo deviceInfo = { uuid, udid, networkId };
        deviceInfos_.InsertOrAssign(uuid, deviceInfo);
        deviceInfos_.InsertOrAssign(udid, deviceInfo);
        deviceInfos_.InsertOrAssign(networkId, deviceInfo);
    }
    if (info != nullptr) {
        FreeNodeInfo(info);
    }
}
} // namespace OHOS::DistributedKv