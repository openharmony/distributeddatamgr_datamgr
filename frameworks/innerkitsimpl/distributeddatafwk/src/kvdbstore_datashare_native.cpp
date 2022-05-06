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

#define LOG_TAG "kvDBDataShareNative"


#include "kvdbstore_result_set.h"
#include "kvstore_predicates.h"
#include "kvdbstore_datashare_native.h"
#include "log_print.h"
#include "data_query.h"

namespace OHOS {
namespace DistributedKv {

kvDBDataShareNative::kvDBDataShareNative(std::shared_ptr<SingleKvStore> kvStoreClient)
    :kvStoreClient_(kvStoreClient) {};

std::shared_ptr<DataShareAbstractResultSet> kvDBDataShareNative::GetDataShareResult(const DataSharePredicates &predicate) 
{
    if(kvStoreClient_ == nullptr)
    {
        ZLOGE("kvStoreClient_ nullptr");
        return nullptr;
    }
    
    DataQuery query;
    auto kvPredicates = std::make_shared<KvStorePredicate>(predicate);
    Status status = kvPredicates->ToQuery(query);
    if(status != Status::SUCCESS)
    {
        ZLOGE("ToQuery failed: %{public}d", status);
        return nullptr;
    }

    std::shared_ptr<KvStoreResultSet> resultSet;
    status = kvStoreClient_->GetResultSetWithQuery(query, resultSet);
    if (status != Status::SUCCESS)
    {
        ZLOGE("GetResultSetWithQuery failed: %{public}d", status);
        return nullptr;
    }

    return std::make_shared<KvDBStoreResultSet>(resultSet);
}
}  // namespace DistributedKv
}  // namespace OHOS
