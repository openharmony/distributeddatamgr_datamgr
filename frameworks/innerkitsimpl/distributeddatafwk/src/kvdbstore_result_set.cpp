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

#define LOG_TAG "KvDBStoreResultSet"

#include "constant.h"
#include "log_print.h"
#include "kvdbstore_result_set.h"

namespace OHOS {
namespace DistributedKv {
KvDBStoreResultSet::KvDBStoreResultSet(std::shared_ptr<KvStoreResultSet> kvResultSet)
    :kvResultSet_(kvResultSet) {};

int KvDBStoreResultSet::GetRowCount(int &count)
{
    count = Count();
    return count == INVALID_COUNT ? E_ERROR : E_OK;
}

int KvDBStoreResultSet::GetAllColumnOrKeyName(std::vector<std::string> &columnOrKeyNames)
{
    columnOrKeyNames.clear();
    columnOrKeyNames.emplace_back("key", "value");
    return E_OK;
}

bool KvDBStoreResultSet::FillBlock(int startRowIndex, const std::shared_ptr<DataShareBlockWriter> &writer)
{
    if (kvResultSet_ == nullptr)
    {
        ZLOGE("kvResultSet_ nullptr");
        return false;
    }

    bool isMoved = kvResultSet_->MoveToPosition(startRowIndex);
    if (!isMoved)
    {
        ZLOGE("MoveToPosition failed");
        return false;
    }

    Entry entry;
    Status status = kvResultSet_->GetEntry(entry);
    if (status != Status::SUCCESS) 
    {
        ZLOGE("GetEntry failed %{public}d", status);
        return false;
    }

    int clearStatus = writer->Clear();
    if(clearStatus != E_OK){
        ZLOGE("clear writer failed: %{public}d", clearStatus);
        return false;
    }

    if ((entry.key.Size() + entry.value.Size()) > (writer->Size() - writer->GetUsedBytes()))
    {
        ZLOGE("not enought memory; entry size: %{public}d, writer size: %{public}d",
              entry.key.Size() + entry.value.Size(), writer->Size() - writer->GetUsedBytes());
        return false;
    }
    // uint8_t
    int keyStatus = writer->WriteBlob((uint32_t)startRowIndex, 0, &entry.key.Data(), entry.key.Size()); 
    if(keyStatus != E_OK)
    {
        ZLOGE("WriteBlob key error: %{public}d", keyStatus);
        return false;
    }

    int valueStatus = writer->WriteBlob((uint32_t)startRowIndex, 1, &entry.value.Data(), entry.value.Size());
    if(valueStatus != E_OK)
    {
        ZLOGE("WriteBlob value error: %{public}d", valueStatus);
        return false;
    }
    return true;
}

int KvDBStoreResultSet::Count()
{
    if (kvResultSet_ == nullptr)
    {
        ZLOGE("kvResultSet_ nullptr");
        return INVALID_COUNT;
    }
    if (resultRowCount != INVALID_COUNT)
    {
        return resultRowCount;
    }
    int count = kvResultSet_->GetCount(); 
    if (count < 0)
    {
        ZLOGE("kvResultSet count invalid: %{public}d", count);
        return INVALID_COUNT;
    }
    resultRowCount = count; 
    return count;
}

bool KvDBStoreResultSet::OnGo(int oldRowIndex, int newRowIndex, const std::shared_ptr<DataShareBlockWriter> &writer) 
{
    if (writer == nullptr || newRowIndex >= Count()) 
    {
        ZLOGE("invalid writer: {public} or newRowIndex: %{public}d", newRowIndex);
        return false;
    }
    return FillBlock(newRowIndex, writer);
}
}  // namespace DistributedKv
}  // namespace OHOS
