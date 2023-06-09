/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aos/common/resourcemonitor.hpp"

#include "log.hpp"

namespace aos {
namespace monitoring {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error ResourceMonitor::Init(ResourceUsageProviderItf& resourceUsageProvider, SenderItf& monitorSender)
{
    LOG_DBG() << "Init resource monitor";

    mResourceUsageProvider = &resourceUsageProvider;
    mMonitorSender = &monitorSender;

    NodeMonitorParams nodeMonitorParams;
    auto              err = mResourceUsageProvider->GetResourceConfig(nodeMonitorParams);
    if (!err.IsNone()) {
        return err;
    }

    mNodeMonitoringData.mNodeID = nodeMonitorParams.mNodeID;

    for (const auto& disk : nodeMonitorParams.mPartitions) {
        mNodeMonitoringData.mMonitoringData.mDisk.EmplaceBack(disk);
    }

    mTimerGatherers.Create(
        cTimeoutGather,
        [this](void*) {
            mNotifyMonitoring = true;
            mMonitoringCondVar.NotifyAll();
        },
        false);

    err = RunGatheringNodeMonitoringData();
    if (!err.IsNone()) {
        return err;
    }

    return RunSendMonitoringData();
}

Error ResourceMonitor::StartSendMonitoring()
{
    LOG_DBG() << "Start send monitoring";

    return mTimerSenders.Create(
        cTimeoutSend,
        [this](void*) {
            mNotifySendMonitoring = true;
            mMonitoringCondVar.NotifyAll();
        },
        false);
}

Error ResourceMonitor::StopSendMonitoring()
{
    LOG_DBG() << "Stop send monitoring";

    return mTimerSenders.Stop();
}

Error ResourceMonitor::GetNodeInfo(NodeInfo& nodeInfo) const
{
    LOG_DBG() << "Get node info";

    return AOS_ERROR_WRAP(mResourceUsageProvider->GetNodeInfo(nodeInfo));
}

Error ResourceMonitor::StartInstanceMonitoring(const String& instanceID, const InstanceMonitorParams& monitoringConfig)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Start instance monitoring";

    auto findInstance = mNodeMonitoringData.mServiceInstances.Find(
        [&instanceID](const InstanceMonitoringData& instance) { return instance.mInstanceID == instanceID; });

    if (!findInstance.mError.IsNone()) {
        MonitoringData monitoringData;

        for (const auto& disk : monitoringConfig.mPartitions) {
            monitoringData.mDisk.EmplaceBack(disk);
        }

        mNodeMonitoringData.mServiceInstances.EmplaceBack(instanceID, monitoringConfig.mInstanceIdent, monitoringData);

        return ErrorEnum::eNone;
    }

    findInstance.mValue->mMonitoringData.mDisk.Clear();

    for (const auto& disk : monitoringConfig.mPartitions) {
        findInstance.mValue->mMonitoringData.mDisk.EmplaceBack(disk);
    }

    return ErrorEnum::eNone;
}

Error ResourceMonitor::StopInstanceMonitoring(const String& instanceID)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Stop instance monitoring";

    return mNodeMonitoringData.mServiceInstances
        .Remove([&instanceID](const InstanceMonitoringData& instance) { return instance.mInstanceID == instanceID; })
        .mError;
}

ResourceMonitor::~ResourceMonitor()
{
    {
        LockGuard lock(mMutex);
        mFinishMonitoring = true;
        mMonitoringCondVar.NotifyAll();
    }

    mThreadMonitoring.Join();
    mThreadSendMonitoring.Join();

    auto err = mTimerSenders.Stop();
    if (!err.IsNone()) {
        LOG_ERR() << "Failed to stop sender timer: " << err;
    }

    err = mTimerGatherers.Stop();
    if (!err.IsNone()) {
        LOG_ERR() << "Failed to stop gatherer timer: " << err;
    }
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

Error ResourceMonitor::RunSendMonitoringData()
{
    return mThreadSendMonitoring.Run([this](void*) {
        while (true) {

            aos::LockGuard lock(mMutex);

            if (lock.GetError() != ErrorEnum::eNone) {
                LOG_ERR() << "Failed to lock mutex: " << lock.GetError();

                continue;
            }

            mMonitoringCondVar.Wait([this] { return mFinishMonitoring || mNotifySendMonitoring; });
            if (mFinishMonitoring) {
                break;
            }

            mNotifySendMonitoring = false;

            LOG_DBG() << "Send monitoring data";

            auto err = mMonitorSender->SendMonitoringData(mNodeMonitoringData);
            if (!err.IsNone()) {
                LOG_ERR() << "Failed to send monitoring data: " << err;
            }
        }
    });
}

Error ResourceMonitor::RunGatheringNodeMonitoringData()
{
    return mThreadMonitoring.Run([this](void*) {
        while (true) {

            aos::LockGuard lock(mMutex);

            if (lock.GetError() != ErrorEnum::eNone) {
                LOG_ERR() << "Failed to lock mutex: " << lock.GetError();

                continue;
            }

            mMonitoringCondVar.Wait([this] { return mFinishMonitoring || mNotifyMonitoring; });
            if (mFinishMonitoring) {
                break;
            }

            mNotifyMonitoring = false;

            LOG_DBG() << "Gather monitoring data";

            auto err = mResourceUsageProvider->GetNodeMonitoringData(
                mNodeMonitoringData.mNodeID, mNodeMonitoringData.mMonitoringData);
            if (!err.IsNone()) {
                LOG_ERR() << "Failed to get node monitoring data: " << err;
            }

            for (auto& instance : mNodeMonitoringData.mServiceInstances) {
                auto err
                    = mResourceUsageProvider->GetInstanceMonitoringData(instance.mInstanceID, instance.mMonitoringData);

                if (!err.IsNone()) {
                    LOG_ERR() << "Failed to get instance monitoring data: " << err;
                }
            }
        }
    });
}

} // namespace monitoring
} // namespace aos
