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
#define LOG_TAG "BackupManager"

#include "backup_manager.h"
#include "log_print.h"
#include "securec.h"
#include "security_manager.h"

namespace OHOS::DistributedKv {
namespace {
constexpr const char *BACKUP_POSTFIX = ".bak";
constexpr const int BACKUP_POSTFIX_SIZE = 4;
constexpr const char *BACKUP_TMP_POSTFIX = ".bk";
constexpr const int BACKUP_TMP_POSTFIX_SIZE = 3;
constexpr const char *BACKUP_KEY_POSTFIX = ".key";
constexpr const char *BACKUP_KEY_PREFIX = "Prefix_backup_";
constexpr const char *AUTO_BACKUP_NAME = "autoBackup";
constexpr const char *BACKUP_TOP_PATH = "/kvdb/backup";
constexpr const char *KEY_PATH = "/key";
}

BackupManager &BackupManager::GetInstance()
{
    static BackupManager instance;
    return instance;
}

BackupManager::BackupManager()
{
    pool_ = KvStoreThreadPool::GetPool(POOL_SIZE, true);
}

BackupManager::~BackupManager()
{
    if (pool_ != nullptr) {
        pool_->Stop();
        pool_ = nullptr;
    }
}

void BackupManager::Init(std::string baseDir)
{
    if (pool_ == nullptr) {
        ZLOGE("Backup Init, pool is null");
        return;
    }
    KvStoreTask task([this, baseDir]() {
        auto topPath = baseDir + BACKUP_TOP_PATH;
        auto keyPath = baseDir + KEY_PATH;
        auto storeIds = StoreUtil::GetSubPath(topPath);
        auto keyFiles = StoreUtil::GetFiles(keyPath);
        for (auto &storeId : storeIds) {
            if (storeId == "." || storeId == "..") {
                continue;
            }
            auto backupPath = topPath + "/" + storeId;
            auto backupFiles = StoreUtil::GetFiles(backupPath);
            if (HaveResidueFile(backupFiles) || HaveResidueKey(keyFiles, storeId)) {
                auto ResidueInfo = BuildResidueInfo(backupFiles, keyFiles, storeId);
                ClearResidueFile(ResidueInfo, baseDir, storeId);
            }
        }
    });
    pool_->AddTask(std::move(task));
}

void BackupManager::Prepare(std::string path, std::string storeId)
{
    std::string topPath = path + BACKUP_TOP_PATH;
    std::string storePath = topPath + "/" + storeId;
    std::string autoBackupName = storePath + "/" + AUTO_BACKUP_NAME + BACKUP_POSTFIX;
    (void)StoreUtil::InitPath(topPath);
    (void)StoreUtil::InitPath(storePath);
    (void)StoreUtil::CreateFile(autoBackupName);
}

void BackupManager::KeepData(std::string name, bool isCreate)
{
    auto tmpName = name + BACKUP_TMP_POSTFIX;
    if (isCreate) {
        StoreUtil::CreateFile(tmpName);
    } else {
        StoreUtil::Rename(name, tmpName);
    }
}

void BackupManager::RollBackData(std::string name, bool isCreate)
{
    auto tmpName = name + BACKUP_TMP_POSTFIX;
    if (isCreate) {
        StoreUtil::Remove(tmpName);
    } else {
        StoreUtil::Remove(name);
        StoreUtil::Rename(tmpName, name);
    }
}

void BackupManager::CleanTmpData(std::string name)
{
    auto tmpName = name + BACKUP_TMP_POSTFIX;
    StoreUtil::Remove(tmpName);
}

Status BackupManager::Backup(const std::string &name, const std::string &baseDir, const std::string &storeId,
    std::shared_ptr<DBStore> dbStore)
{
    if (name.size() == 0 || baseDir.size() == 0 || storeId.size() == 0 || name == AUTO_BACKUP_NAME) {
        return INVALID_ARGUMENT;
    }
    std::string topPath = baseDir + BACKUP_TOP_PATH;
    std::string storePath = topPath + "/" + storeId;
    std::string backupFullName = storePath + "/"+ name + BACKUP_POSTFIX;
    std::string keyName = BACKUP_KEY_PREFIX + storeId + "_" + name;
    std::string keyFullName = baseDir + KEY_PATH + "/" + keyName + BACKUP_KEY_POSTFIX;

    bool isCreate = !StoreUtil::IsFileExist(backupFullName);
    if ((StoreUtil::GetFiles(storePath).size() >= MAX_BACKUP_NUM) && isCreate) {
        return ERROR;
    }
    (void)StoreUtil::InitPath(topPath);
    (void)StoreUtil::InitPath(storePath);
    KeepData(backupFullName, isCreate);
    auto password = SecurityManager::GetInstance().GetKey(storeId, baseDir);
    if (password.GetSize() != 0) {
        KeepData(keyFullName, isCreate);
    }

    auto dbStatus = dbStore->Export(backupFullName, password);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status == SUCCESS) {
        if (password.GetSize() != 0) {
            SecurityManager::GetInstance().SaveKey(keyName, baseDir, password);
            CleanTmpData(keyFullName);
        }
        CleanTmpData(backupFullName);
    } else {
        RollBackData(backupFullName, isCreate);
        if (password.GetSize() != 0) {
            RollBackData(keyFullName, isCreate);
        }
    }
    return status;
}

StoreUtil::FileInfo BackupManager::GetBackupFileInfo(
    std::string name, std::string baseDir, std::string storeId)
{
    StoreUtil::FileInfo backupFile;
    std::string path = baseDir + BACKUP_TOP_PATH + "/" + storeId;
    std::string backupName = name + BACKUP_POSTFIX;

    auto files = StoreUtil::GetFiles(path);
    time_t modifyTime = 0;
    for (auto &file : files) {
        if (file.name == backupName) {
            backupFile = file;
            break;
        }
        if ((file.modifyTime > modifyTime) && (file.size != 0)) {
            modifyTime = file.modifyTime;
            backupFile = file;
        }
    }
    return backupFile;
}

Status BackupManager::Restore(const std::string &name, const std::string &baseDir, const std::string &storeId,
    std::shared_ptr<DBStore> dbStore)
{
    if (storeId.size() == 0 || baseDir.size() == 0) {
        return INVALID_ARGUMENT;
    }
    auto backupFile = GetBackupFileInfo(name, baseDir, storeId);
    if (backupFile.name.size() == 0) {
        return INVALID_ARGUMENT;
    }
    std::string keyName = BACKUP_KEY_PREFIX + storeId + "_" + name;
    std::string fullName = baseDir + BACKUP_TOP_PATH + "/" + storeId + "/" + backupFile.name;
    auto password = SecurityManager::GetInstance().GetKey(keyName, baseDir);
    auto dbStatus = dbStore->Import(fullName, password);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    return status;
}

Status BackupManager::DeleteBackup(std::map<std::string, Status> &deleteList, const std::string &baseDir,
    const std::string &storeId)
{
    if (deleteList.empty() || baseDir.size() == 0 || storeId.size() == 0) {
        return INVALID_ARGUMENT;
    }

    std::string path = baseDir + BACKUP_TOP_PATH + "/" + storeId;
    auto fileInfos = StoreUtil::GetFiles(path);
    for (auto &info : fileInfos) {
        auto it = deleteList.find(info.name.substr(0, info.name.length() - BACKUP_POSTFIX_SIZE));
        if (it == deleteList.end()) {
            continue;
        }
        if (info.name == AUTO_BACKUP_NAME) {
            it->second = INVALID_ARGUMENT;
            continue;
        }
        std::string keyName = BACKUP_KEY_PREFIX + storeId + "_" + info.name;
        SecurityManager::GetInstance().DelKey(keyName, baseDir);
        it->second = (StoreUtil::Remove(path + "/" + info.name)) ?  SUCCESS : ERROR;
    }
    return SUCCESS;
}

bool BackupManager::HaveResidueFile(const std::vector<StoreUtil::FileInfo> &fileList)
{
    for (auto &file : fileList) {
        if (IsEndWith(file.name, BACKUP_TMP_POSTFIX)) {
            return true;
        }
    }
    return false;
}

bool BackupManager::HaveResidueKey(const std::vector<StoreUtil::FileInfo> &fileList, std::string storeId)
{
    for (auto &file : fileList) {
        auto prefix = BACKUP_KEY_PREFIX + storeId;
        if (IsBeginWith(file.name, prefix) && IsEndWith(file.name, BACKUP_TMP_POSTFIX)) {
            return true;
        }
    }
    return false;
}

std::string BackupManager::GetBackupName(std::string fileName)
{
    int postFixLen = IsEndWith(fileName, BACKUP_TMP_POSTFIX) ?
        BACKUP_POSTFIX_SIZE + BACKUP_TMP_POSTFIX_SIZE : BACKUP_POSTFIX_SIZE;
    return fileName.substr(0, fileName.length() - postFixLen);
}

void BackupManager::SetResidueInfo(BackupManager::ResidueInfo &residueInfo,
    const std::vector<StoreUtil::FileInfo> &fileList, std::string name, std::string postFix)
{
    for (auto &file : fileList) {
        if (IsBeginWith(file.name, name)) {
            if (IsEndWith(file.name, postFix + BACKUP_TMP_POSTFIX) && (postFix == BACKUP_POSTFIX)) {
                residueInfo.hasTmpBackup = true;
                residueInfo.tmpBackupSize = file.size;
            }
            if (IsEndWith(file.name, postFix) && (postFix == BACKUP_POSTFIX)) {
                residueInfo.hasRawBackup = true;
            }
            if (IsEndWith(file.name, postFix + BACKUP_TMP_POSTFIX) && (postFix == BACKUP_KEY_POSTFIX)) {
                residueInfo.hasTmpKey = true;
                residueInfo.tmpKeySize = file.size;
            }
            if (IsEndWith(file.name, postFix) && (postFix == BACKUP_KEY_POSTFIX)) {
                residueInfo.hasRawKey = true;
            }
        }
    }
}

std::map<std::string, BackupManager::ResidueInfo> BackupManager::BuildResidueInfo(
    const std::vector<StoreUtil::FileInfo> &fileList,
    const std::vector<StoreUtil::FileInfo> &keyList, std::string storeId)
{
    std::map<std::string, ResidueInfo> residueInfoList;
    for (auto &file : fileList) {
        auto backupName = GetBackupName(file.name);
        auto it = residueInfoList.find(backupName);
        if (it == residueInfoList.end()) {
            ResidueInfo residueInfo;
            memset_s(&residueInfo, sizeof(ResidueInfo), 0, sizeof(ResidueInfo));
            SetResidueInfo(residueInfo, fileList, backupName, BACKUP_POSTFIX);
            SetResidueInfo(residueInfo, keyList, BACKUP_KEY_PREFIX + storeId + "_" + backupName, BACKUP_KEY_POSTFIX);
            residueInfoList.emplace(backupName, residueInfo);
        }
    }
    return residueInfoList;
}

/**
 *  in function NeedRollBack, use the number of tmp and raw file to charge who to do when start,
 *  learning by watching blow table,
 *  we can konw when the num of tmp file greater than or equal raw, interrupt happend druing backup
 *
 *  backup step             file status                         option          file num
 *  1, backup old data      -               storeId.key         rollback        raw = 1
 *                          storeId.bak.bk  -                                   tmp = 1
 *
 *  2, backup old key       -               -                   rollback        raw = 0
 *                          storeId.bak.bk, storeId.key.bk                      tmp = 2
 *
 *  3, do backup            storeId.bak     -                   rollback        raw = 1
 *                          storeId.bak.bk, storeId.key.bk                      tmp = 2
 *
 *  4, store key            storeId.bak     storeId.key         rollback        raw = 2
 *                          storeId.bak.bk, storeId.key.bk                      tmp = 2
 *
 *  5, delet tmp key        storeId.bak     storeId.key         clean data      raw = 1
 *                          storeId.bak.bk  -                                   tmp = 2
 *
 *  6, delet tmp data       storeId.bak     storeId.key         do nothing      raw = 0
 *                          -               -                                   tmp = 2
 * */
bool BackupManager::NeedRollBack(BackupManager::ResidueInfo residueInfo)
{
    int rawFile = 0;
    int tmpFile = 0;
    if (residueInfo.hasRawBackup) {
        rawFile++;
    }
    if (residueInfo.hasRawKey) {
        rawFile++;
    }
    if (residueInfo.hasTmpBackup) {
        tmpFile++;
    }
    if (residueInfo.hasTmpKey) {
        tmpFile++;
    }
    return (tmpFile >= rawFile) ? true : false;
}

void BackupManager::ClearResidueFile(std::map<std::string, ResidueInfo> residueInfo,
    std::string baseDir, std::string storeId)
{
    for (auto &info : residueInfo) {
        auto backupFullName = baseDir + BACKUP_TOP_PATH + "/" + storeId + "/" + info.first + BACKUP_POSTFIX;
        auto keyFullName =
            baseDir + KEY_PATH + "/" + BACKUP_KEY_PREFIX + storeId + "_" + info.first + BACKUP_KEY_POSTFIX;
        if (NeedRollBack(info.second)) {
            ZLOGE("store : %{public}s, %{public}d, %{public}d, %{public}d, %{public}d, need rollback",
                info.first.c_str(), info.second.hasRawBackup, info.second.hasTmpBackup,
                info.second.hasRawKey, info.second.hasTmpKey);

            if (info.second.hasTmpBackup) {
                RollBackData(backupFullName, (info.second.tmpBackupSize == 0));
            }
            if (info.second.hasTmpKey) {
                RollBackData(keyFullName, (info.second.tmpKeySize == 0));
            }
        } else {
            if (info.second.hasTmpBackup) {
                CleanTmpData(backupFullName);
            }
            if (info.second.hasTmpKey) {
                CleanTmpData(keyFullName);
            }
        }
    }
}

bool BackupManager::IsEndWith(const std::string &fullString, const std::string &end)
{
    if (fullString.length() >= end.length()) {
        return (fullString.compare(fullString.length() - end.length(), end.length(), end) == 0);
    } else {
        return false;
    }
}

bool BackupManager::IsBeginWith(const std::string &fullString, const std::string &begin)
{
    if (fullString.length() >= begin.length()) {
        return (fullString.compare(0, begin.length(), begin) == 0);
    } else {
        return false;
    }
}
} // namespace OHOS::DistributedKv