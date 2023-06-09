/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_RESOURCEMONITOR_HPP_
#define AOS_RESOURCEMONITOR_HPP_

#include "aos/common/tools/error.hpp"
#include "aos/common/types.hpp"
#include <aos/common/tools/thread.hpp>
#include <aos/common/tools/timer.hpp>

namespace aos {
namespace monitoring {

/*
 * Partition name len.
 */
constexpr auto cPartitionNameLen = AOS_CONFIG_TYPES_PARTITION_NAME_LEN;

/*
 * Max number of partition types.
 */
constexpr auto cMaxNumPartitionTypes = AOS_CONFIG_TYPES_MAX_NUM_PARTITION_TYPES;

/*
 * Max number of partitions.
 */
constexpr auto cMaxNumPartitions = AOS_CONFIG_TYPES_MAX_NUM_PARTITIONS;

/**
 * Node ID len.
 */
constexpr auto cNodeIDLen = AOS_CONFIG_TYPES_NODE_ID_LEN;

/**
 * Partition types len.
 */
constexpr auto cPartitionTypesLen = AOS_CONFIG_TYPES_PARTITION_TYPES_LEN;

/**
 * Timeout for polling resource usage.
 */
constexpr auto cTimeoutGather = AOS_CONFIG_MONITORING_POLL_TIMEOUT_MS;

/**
 * Partition info.
 *
 */
struct PartitionInfo {
    StaticString<cPartitionNameLen>                                      mName;
    StaticString<cFilePathLen>                                           mPath;
    StaticArray<StaticString<cPartitionTypesLen>, cMaxNumPartitionTypes> mTypes;
    size_t                                                               mTotalSize;
    size_t                                                               mUsedSize;
};

/**
 * System info.
 */
struct NodeInfo {
    size_t                                        mNumCPUs;
    size_t                                        mTotalRAM;
    StaticArray<PartitionInfo, cMaxNumPartitions> mPartitions;
};

/**
 * MonitoringData monitoring data.
 */
struct MonitoringData {
    size_t                                        mRAM;
    size_t                                        mCPU;
    StaticArray<PartitionInfo, cMaxNumPartitions> mDisk;
};

/**
 * InstanceMonitoringData monitoring data for service.
 */
struct InstanceMonitoringData {
    /**
     * Construct a new Instance Monitoring Data object
     *
     */
    InstanceMonitoringData() = default;

    /**
     * Construct a new Instance Monitoring Data object
     *
     * @param instanceID instance ID
     * @param monitoringData monitoring data
     */
    InstanceMonitoringData(
        const String& instanceID, const InstanceIdent& instanceIdent, const MonitoringData& monitoringData)
        : mInstanceID(instanceID)
        , mInstanceIdent(instanceIdent)
        , mMonitoringData(monitoringData)
    {
    }

    StaticString<cInstanceIDLen> mInstanceID;
    InstanceIdent                mInstanceIdent;
    MonitoringData               mMonitoringData;
};

/**
 * NodeMonitoringData node monitoring data.
 */
struct NodeMonitoringData {
    StaticString<cNodeIDLen>                              mNodeID;
    MonitoringData                                        mMonitoringData;
    time_t                                                mTimestamp;
    StaticArray<InstanceMonitoringData, cMaxNumInstances> mServiceInstances;
};

/**
 * InstanceMonitorParams resource monitor parameters.
 */
struct InstanceMonitorParams {
    InstanceIdent                                 mInstanceIdent;
    StaticArray<PartitionInfo, cMaxNumPartitions> mPartitions;
};

/**
 * NodeMonitorParams resource monitor parameters.
 */
struct NodeMonitorParams {
    StaticString<cNodeIDLen>                      mNodeID;
    StaticArray<PartitionInfo, cMaxNumPartitions> mPartitions;
};

/**
 * Resource usage provider interface.
 */
class ResourceUsageProviderItf {
public:
    /**
     * Gatherings system info
     *
     * @param systemInfo system info
     * @return Error
     */
    virtual Error GetNodeInfo(NodeInfo& systemInfo) const = 0;

    /**
     * Gatherings node monitoring data
     *
     * @param nodeID node ident
     * @param monitoringData monitoring data
     * @return Error
     */
    virtual Error GetNodeMonitoringData(const String& nodeID, MonitoringData& monitoringData) = 0;

    /**
     * Gatherings instance monitoring data
     *
     * @param instance instance ident
     * @param monitoringData monitoring data
     * @return Error
     */
    virtual Error GetInstanceMonitoringData(const String& instanceID, MonitoringData& monitoringData) = 0;

    /**
     * Gatherings resource config
     *
     * @param nodeMonitorParams node monitor params
     * @return Error
     */
    virtual Error GetResourceConfig(NodeMonitorParams& nodeMonitorParams) const = 0;
};

/**
 * Monitor sender interface.
 */
class SenderItf {
public:
    /**
     * Sends monitoring data
     *
     * @param monitoringData monitoring data
     * @return Error
     */
    virtual Error SendMonitoringData(const NodeMonitoringData& monitoringData) = 0;
};

/**
 * Resource monitor interface.
 */
class ResourceMonitorItf {
public:
    /**
     * Destructors
     */
    virtual ~ResourceMonitorItf() = default;

    /**
     * Gets the node info object
     *
     * @param nodeInfo node info
     * @return Error
     */
    virtual Error GetNodeInfo(NodeInfo& nodeInfo) const = 0;

    /**
     * Starts instance monitoring
     *
     * @param instanceID instance ident
     * @param monitoringConfig monitoring config
     * @return Error
     */
    virtual Error StartInstanceMonitoring(const String& instanceID, const InstanceMonitorParams& monitoringConfig) = 0;

    /**
     * Stops instance monitoring
     *
     * @param instanceID instance ident
     * @return Error
     */
    virtual Error StopInstanceMonitoring(const String& instanceID) = 0;

    /**
     * Starts node monitoring
     *
     * @return Error
     */
    virtual Error StartSendMonitoring() = 0;

    /**
     * Stops node monitoring
     *
     * @return Error
     */
    virtual Error StopSendMonitoring() = 0;
};

/**
 * Resource monitor.
 */
class ResourceMonitor : public ResourceMonitorItf {
public:
    /**
     * Constructors
     */
    ResourceMonitor() = default;

    /**
     * Destructors
     */
    ~ResourceMonitor();

    /**
     * Initializes resource monitor
     *
     * @param resourceUsageProvider resource usage provider
     * @param monitorSender monitor sender
     * @return Error
     */
    Error Init(ResourceUsageProviderItf& resourceUsageProvider, SenderItf& monitorSender);

    /**
     * Gets the node info object
     *
     * @param nodeInfo node info
     * @return Error
     */
    Error GetNodeInfo(NodeInfo& nodeInfo) const override;

    /**
     * Starts instance monitoring
     *
     * @param instanceID instance ident
     * @param monitoringConfig monitoring config
     * @return Error
     */
    Error StartInstanceMonitoring(const String& instanceID, const InstanceMonitorParams& monitoringConfig) override;

    /**
     * Stops instance monitoring
     *
     * @param instanceID instance ident
     * @return Error
     */
    Error StopInstanceMonitoring(const String& instanceID) override;

    /**
     * Starts node monitoring
     *
     * @return Error
     */
    Error StartSendMonitoring() override;

    /**
     * Stops node monitoring
     *
     * @return Error
     */
    Error StopSendMonitoring() override;

private:
    static constexpr auto cTimeoutSend = AOS_CONFIG_MONITORING_SEND_TIMEOUT_MS;

    ResourceUsageProviderItf* mResourceUsageProvider {};
    SenderItf*                mMonitorSender {};
    NodeMonitoringData        mNodeMonitoringData {};
    Mutex                     mMutex;
    aos::ConditionalVariable  mMonitoringCondVar {mMutex};
    aos::Timer                mTimerSenders;
    aos::Timer                mTimerGatherers;
    bool                      mFinishMonitoring {};
    bool                      mNotifyMonitoring {};
    bool                      mNotifySendMonitoring {};
    aos::Thread<>             mThreadMonitoring = {};
    aos::Thread<>             mThreadSendMonitoring = {};

    Error RunGatheringNodeMonitoringData();
    Error RunSendMonitoringData();
};

} // namespace monitoring
} // namespace aos

#endif
