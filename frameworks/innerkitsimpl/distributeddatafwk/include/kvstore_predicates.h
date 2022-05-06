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


#ifndef KVSTORE_PREDICATE_H
#define KVSTORE_PREDICATE_H

#include <map>
#include "data_query.h"
#include "types.h"
#include "datashare_predicates.h"

namespace OHOS {
namespace DistributedKv {
using namespace DataShare;

class KvStorePredicate
{
public:
	KvStorePredicate(const DataSharePredicates &predicates);

	~KvStorePredicate() = default;
	
	Status ToQuery(DataQuery &query);

private:
	Status InKeys(const OperationItem &oper, DataQuery &query);
	
	Status KeyPrefix(const OperationItem &oper, DataQuery &query);

	using QueryHandler = Status (KvStorePredicate::*)(const OperationItem &, DataQuery &);

	static const std::map<OperationType, QueryHandler> HANDLERS;

	DataSharePredicates predicates_;
};
}// namespace DistributedKv
}//namespace 

#endif  // KVSTORE_PREDICATE_H


