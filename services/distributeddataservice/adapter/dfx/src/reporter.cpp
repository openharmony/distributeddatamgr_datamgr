/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "reporter.h"
#include "fault/communication_fault_impl.h"
#include "fault/database_fault_impl.h"
#include "fault/runtime_fault_impl.h"
#include "fault/service_fault_impl.h"

#include "statistic/traffic_statistic_impl.h"
#include "statistic/visit_statistic_impl.h"
#include "statistic/database_statistic_impl.h"
#include "statistic/api_performance_statistic_impl.h"

#include "security/permissions_security_impl.h"
#include "security/sensitive_level_security_impl.h"

#include "behaviour/behaviour_reporter_impl.h"

namespace OHOS {
namespace DistributedKv {
Reporter* Reporter::GetInstance()
{
    static Reporter reporter;
    return &reporter;
}

FaultReporter<CommFaultMsg>* Reporter::CommunicationFault()
{
    static CommunicationFaultImpl communicationFault;
    return &communicationFault;
}

FaultReporter<DBFaultMsg>* Reporter::DatabaseFault()
{
    static DatabaseFaultImpl databaseFault;
    return &databaseFault;
}

FaultReporter<FaultMsg>* Reporter::RuntimeFault()
{
    static RuntimeFaultImpl runtimeFault;
    return &runtimeFault;
}

FaultReporter<FaultMsg>* Reporter::ServiceFault()
{
    static ServiceFaultImpl serviceFault;
    return &serviceFault;
}

StatisticReporter<TrafficStat>* Reporter::TrafficStatistic()
{
    static TrafficStatisticImpl trafficStatistic;
    return &trafficStatistic;
}

StatisticReporter<struct VisitStat>* Reporter::VisitStatistic()
{
    static VisitStatisticImpl visitStatistic;
    return &visitStatistic;
}

StatisticReporter<struct DbStat>* Reporter::DatabaseStatistic()
{
    static DatabaseStatisticImpl databaseStatistic;
    return &databaseStatistic;
}

StatisticReporter<ApiPerformanceStat>* Reporter::ApiPerformanceStatistic()
{
    static ApiPerformanceStatisticImpl apiPerformanceStat;
    return &apiPerformanceStat;
}

SecurityReporter<struct SecurityPermissionsMsg>* Reporter::PermissionsSecurity()
{
    static PermissionsSecurityImpl permissionsSecurity;
    return &permissionsSecurity;
}

SecurityReporter<struct SecuritySensitiveLevelMsg>* Reporter::SensitiveLevelSecurity()
{
    static SensitiveLevelSecurityImpl sensitiveLevelSecurity;
    return &sensitiveLevelSecurity;
}

BehaviourReporter* Reporter::BehaviourReporter()
{
    static BehaviourReporterImpl behaviourReporter;
    return &behaviourReporter;
}
} // namespace DistributedKv
} // namespace OHOS
