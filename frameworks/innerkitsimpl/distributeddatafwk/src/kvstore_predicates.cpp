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

#define LOG_TAG "KvStorePredicate"

#include "kvstore_predicates.h"
#include "log_print.h"
#include "datashare_errno.h"

namespace OHOS {
namespace DistributedKv {
	
const std::map<OperationType, KvStorePredicate::QueryHandler> KvStorePredicate::HANDLERS = {
    {IN_KEY, &KvStorePredicate::InKeys},
    {KEY_PREFIX, &KvStorePredicate::KeyPrefix}
};

KvStorePredicate::KvStorePredicate(const DataSharePredicates &predicates)
    :predicates_(predicates){};

Status KvStorePredicate::ToQuery(DataQuery &query)
{
    std::list<OperationItem> operationList = predicates_.GetOperationList();
    for(const auto &oper : operationList)
    {
        auto it = HANDLERS.find(oper.operation);
        if(it == HANDLERS.end())
        {
            ZLOGE("This feature is not supported");
            return Status::NOT_SUPPORT;
        }
        Status status = (this->*(it->second))(oper, query);
        if(status != Status::SUCCESS){
            ZLOGE("ToQuery called failed");
            return status;
        }
    }
    return Status::SUCCESS;    
}

Status KvStorePredicate::InKeys(const OperationItem &oper, DataQuery &query)
{
    std::vector<std::string> keys;
    int status = oper.para1.GetStringVector(keys);
    if(status != E_OK){
        ZLOGE("GetStringVector failed");
        return Status::ERROR;
    }
    query.InKeys(keys);
    return Status::SUCCESS;
}

Status KvStorePredicate::KeyPrefix(const OperationItem &oper, DataQuery &query)
{
    std::string prefix;
    int status = oper.para1.GetString(prefix);
    if(status != E_OK){
        ZLOGE("KeyPrefix failed");
        return Status::ERROR;
    }
    query.KeyPrefix(prefix);
    return Status::SUCCESS;
}	
}// namespace DistributedKv
}// namespace OHOS