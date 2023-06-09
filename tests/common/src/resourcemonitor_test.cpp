#include <gtest/gtest.h>

#include "aos/common/resourcemonitor.hpp"

namespace aos {
namespace monitoring {

/***********************************************************************************************************************
 * Mocks
 **********************************************************************************************************************/

class MockResourceUsageProvider : public ResourceUsageProviderItf {
public:
    Error GetNodeInfo(NodeInfo& nodeInfo) const override
    {
        nodeInfo.mNumCPUs = 1;
        nodeInfo.mTotalRAM = 4000;

        PartitionInfo partitionInfo {};
        partitionInfo.mName = "partitionName";
        partitionInfo.mTypes.PushBack("partitionType");
        partitionInfo.mTotalSize = 1000;

        nodeInfo.mPartitions.PushBack(partitionInfo);

        return ErrorEnum::eNone;
    }

    Error GetNodeMonitoringData(const String& nodeID, MonitoringData& monitoringData) override
    {
        EXPECT_TRUE(nodeID == "node1");

        monitoringData.mCPU = 1;
        monitoringData.mRAM = 1000;

        EXPECT_TRUE(monitoringData.mDisk.Size() == 1);
        EXPECT_TRUE(monitoringData.mDisk[0].mName == "partitionName");
        EXPECT_TRUE(monitoringData.mDisk[0].mPath == "partitionPath");

        monitoringData.mDisk[0].mUsedSize = 100;

        return ErrorEnum::eNone;
    }

    Error GetInstanceMonitoringData(const String& instanceID, MonitoringData& monitoringData) override
    {
        EXPECT_TRUE(instanceID == "instance1");

        monitoringData.mCPU = 1;
        monitoringData.mRAM = 1000;

        EXPECT_TRUE(monitoringData.mDisk.Size() == 1);
        EXPECT_TRUE(monitoringData.mDisk[0].mName == "partitionInstanceName");
        EXPECT_TRUE(monitoringData.mDisk[0].mPath == "partitionInstancePath");

        monitoringData.mDisk[0].mUsedSize = 100;

        return ErrorEnum::eNone;
    }

    Error GetResourceConfig(NodeMonitorParams& nodeMonitorParams) const override
    {
        nodeMonitorParams = mNodeMonitorParams;

        return ErrorEnum::eNone;
    }

    void SetNodeMonitorParams(const NodeMonitorParams& nodeMonitorParams) { mNodeMonitorParams = nodeMonitorParams; }

private:
    NodeMonitorParams mNodeMonitorParams {};
};

class MockSender : public SenderItf {
public:
    Error SendMonitoringData(const NodeMonitoringData& monitoringData) override
    {

        EXPECT_TRUE(monitoringData.mNodeID == "node1");

        EXPECT_TRUE(monitoringData.mMonitoringData.mCPU == 1);
        EXPECT_TRUE(monitoringData.mMonitoringData.mRAM == 1000);

        EXPECT_TRUE(monitoringData.mMonitoringData.mDisk.Size() == 1);

        EXPECT_TRUE(monitoringData.mMonitoringData.mDisk[0].mName == "partitionName");
        EXPECT_TRUE(monitoringData.mMonitoringData.mDisk[0].mPath == "partitionPath");

        EXPECT_TRUE(monitoringData.mMonitoringData.mDisk[0].mUsedSize == 100);

        if (!mExpectedInstanceMonitoring) {
            EXPECT_TRUE(monitoringData.mServiceInstances.Size() == 0);

            return ErrorEnum::eNone;
        }

        EXPECT_TRUE(monitoringData.mServiceInstances.Size() == 1);

        aos::InstanceIdent instanceIdent {};
        instanceIdent.mInstance = 1;
        instanceIdent.mServiceID = "serviceID";
        instanceIdent.mSubjectID = "subjectID";

        EXPECT_TRUE(monitoringData.mServiceInstances[0].mInstanceIdent == instanceIdent);

        EXPECT_TRUE(monitoringData.mServiceInstances[0].mInstanceID == "instance1");

        EXPECT_TRUE(monitoringData.mServiceInstances[0].mMonitoringData.mCPU == 1);
        EXPECT_TRUE(monitoringData.mServiceInstances[0].mMonitoringData.mRAM == 1000);

        EXPECT_TRUE(monitoringData.mServiceInstances[0].mMonitoringData.mDisk.Size() == 1);

        EXPECT_TRUE(monitoringData.mServiceInstances[0].mMonitoringData.mDisk[0].mName == "partitionInstanceName");

        EXPECT_TRUE(monitoringData.mServiceInstances[0].mMonitoringData.mDisk[0].mPath == "partitionInstancePath");

        EXPECT_TRUE(monitoringData.mServiceInstances[0].mMonitoringData.mDisk[0].mUsedSize == 100);

        return ErrorEnum::eNone;
    }

    void SetExpectedInstanceMonitoring(bool expectedInstanceMonitoring)
    {
        mExpectedInstanceMonitoring = expectedInstanceMonitoring;
    }

private:
    bool mExpectedInstanceMonitoring {};
};

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

TEST(common, ResourceMonitorInit)
{
    ResourceMonitor           monitor {};
    MockResourceUsageProvider resourceUsageProvider {};
    MockSender                sender {};

    NodeMonitorParams nodeMonitorParams {};
    nodeMonitorParams.mNodeID = "node1";

    resourceUsageProvider.SetNodeMonitorParams(nodeMonitorParams);

    EXPECT_TRUE(monitor.Init(resourceUsageProvider, sender).IsNone());
}

TEST(common, ResourceMonitorGetNodeInfo)
{
    ResourceMonitor           monitor {};
    MockResourceUsageProvider resourceUsageProvider {};
    MockSender                sender {};

    NodeMonitorParams nodeMonitorParams {};
    nodeMonitorParams.mNodeID = "node1";

    resourceUsageProvider.SetNodeMonitorParams(nodeMonitorParams);

    EXPECT_TRUE(monitor.Init(resourceUsageProvider, sender).IsNone());

    NodeInfo nodeInfo {};
    EXPECT_TRUE(monitor.GetNodeInfo(nodeInfo).IsNone());

    EXPECT_EQ(nodeInfo.mNumCPUs, 1);
    EXPECT_EQ(nodeInfo.mTotalRAM, 4000);

    EXPECT_EQ(nodeInfo.mPartitions.Size(), 1);
    EXPECT_EQ(nodeInfo.mPartitions[0].mName, "partitionName");
    EXPECT_EQ(nodeInfo.mPartitions[0].mTypes.Size(), 1);
    EXPECT_EQ(nodeInfo.mPartitions[0].mTypes[0], "partitionType");
    EXPECT_EQ(nodeInfo.mPartitions[0].mTotalSize, 1000);
}

TEST(common, ResourceMonitorGetNodeMonitoringData)
{
    ResourceMonitor           monitor {};
    MockResourceUsageProvider resourceUsageProvider {};
    MockSender                sender {};

    NodeMonitorParams nodeMonitorParams {};
    nodeMonitorParams.mNodeID = "node1";

    PartitionInfo partitionParam {};
    partitionParam.mName = "partitionName";
    partitionParam.mPath = "partitionPath";

    nodeMonitorParams.mPartitions.PushBack(partitionParam);

    resourceUsageProvider.SetNodeMonitorParams(nodeMonitorParams);

    EXPECT_TRUE(monitor.Init(resourceUsageProvider, sender).IsNone());

    sleep(1);
}

TEST(common, ResourceMonitorGetInstanceMonitoringData)
{
    ResourceMonitor           monitor {};
    MockResourceUsageProvider resourceUsageProvider {};
    MockSender                sender {};

    NodeMonitorParams nodeMonitorParams {};
    nodeMonitorParams.mNodeID = "node1";

    PartitionInfo partitionParam {};
    partitionParam.mName = "partitionName";
    partitionParam.mPath = "partitionPath";

    nodeMonitorParams.mPartitions.PushBack(partitionParam);

    resourceUsageProvider.SetNodeMonitorParams(nodeMonitorParams);

    EXPECT_TRUE(monitor.Init(resourceUsageProvider, sender).IsNone());

    InstanceMonitorParams instanceMonitorParams {};
    instanceMonitorParams.mInstanceIdent.mInstance = 1;
    instanceMonitorParams.mInstanceIdent.mServiceID = "serviceID";
    instanceMonitorParams.mInstanceIdent.mSubjectID = "subjectID";

    PartitionInfo partitionInstanceParam {};
    partitionInstanceParam.mName = "partitionInstanceName";
    partitionInstanceParam.mPath = "partitionInstancePath";

    instanceMonitorParams.mPartitions.PushBack(partitionInstanceParam);

    sender.SetExpectedInstanceMonitoring(true);

    EXPECT_TRUE(monitor.StartInstanceMonitoring("instance1", instanceMonitorParams).IsNone());

    sleep(1);
}

} // namespace monitoring
} // namespace aos
