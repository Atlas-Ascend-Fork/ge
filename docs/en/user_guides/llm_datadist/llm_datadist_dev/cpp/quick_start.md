# Quick Start

<!-- md-trans-meta sourceCommit=f2e33566b0420150e0470a13f9852d17f1cfd42b translatedAt=2026-11-17T04:10:04.133Z pushedAt=2026-11-17T12:29:39.517Z nl=2 -->

## Development Process

**Overview**

For ease of subsequent description, different inference frameworks are abstracted into the following modules:

- Resource initialization module.
- KV Cache management module. It includes the creation, allocation (PA scenario), and destruction of KV Cache memory.
- Model inference module.
- Resource release module.

This section mainly introduces how developers enable the LLM-DataDist capability in the inference framework.

**Development Process**

1. Find the resource initialization module in the inference framework, and call the initialization API and connection establishment API of LLM-DataDist in this phase.
2. Find the KV Cache management module in the inference framework, call the KV Cache application API of LLM-DataDist in this phase, and convert the applied KV Cache into the KV Cache type of different frameworks for inference.
3. The inference framework needs to split the Prefill phase and the Decode phase. Deploy the inference scripts separately on different cluster nodes. Before the Decode phase is executed, the output from the Prefill phase needs to be received as input, and the KV Cache transfer API provided by LLM-DataDist is called to pull or push the KV Cache cached on the Prefill side.
4. Execute the Prefill inference script and the Decode inference script respectively.
5. Release LLM-DataDist related resources in the framework resource release module.

## Environment Preparation

The supported product forms are as follows:
<!-- npu="910b" id1 -->

- Atlas A2 product series: For the Atlas A2 product series, only the Atlas 800I A2 inference server and the A200I A2 Box heterogeneous component are supported.
<!-- end id1 -->

<!-- npu="A3" id2 -->
- Atlas A3 product series
<!-- end id2 -->

Install the driver, firmware, and CANN software by referring to [CANN Software Installation](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/softwareinst/instg/instg_0000.html?OS=openEuler&InstallType=netyum).

Use hccn\_tool to query the Device IP and perform inter-card network detection. It is required that the inter-card connections on each cluster have RDMA links; otherwise, the LLM-DataDist capability cannot be enabled. hccn\_tool For details, see [HCCN Tool API Reference](https://support.huawei.com/enterprise/en/ascend-computing/ascend-hdk-pid-252764743?category=developer-documents&subcategory=interface-reference). The following are common command references.

|Command|Scenario|
|--|--|
|hccn_tool [-i %d] -link -g|Obtains the link status of the network port of a specified Device. -i specifies the Device. Example: `hccn_tool -i 0 -link -g`|
|hccn_tool [-i %d] -ip -g|Obtains the IP address and subnet mask. -i specifies the Device. Example: `hccn_tool -i 0 -ip -g`|
|hccn_tool [-i %d] -ping -g [address %s ]|Obtains the ping result from a specified device to the destination address. -i specifies a Device of the current server, and address specifies the destination address of the ping. Example: `hccn_tool -i 0 -ping -g address 192.168.2.1`|

When using LLM-DataDist, the following environment variables are also involved. For details, see [Environment Variable Reference](https://gitcode.com/cann/docs/blob/master/docs/en/env-vars/README.md).

|Name|Scenario|
|--|--|
|HCCL_RDMA_TC, HCCL_RDMA_SL|When customers have made their own planning for the parameter plane network, they specify the type and priority of various service traffic. These two environment variables are used to set the traffic type and priority of parameter plane collective communication traffic on the network, so as to adapt to the requirements of the customer's network traffic planning.|
|HCCL_RDMA_RETRY_CNT, HCCL_RDMA_TIMEOUT|They correspond to the retry count of the RDMA NIC and the coefficient timeout of the retransmission timeout respectively. If the values are set too large, the system becomes insensitive to network anomalies and cannot detect network faults. If the values are set too small, network flapping may directly interrupt services and cannot be shielded by the NIC hardware. Users can set appropriate values based on their own network conditions. For example, they can configure the values based on the time range of most network flapping. It is recommended to configure the values according to the following formula to reduce the impact of network jitter. HCCL_RDMA_TIMEOUT=log2(pull kv timeout * 10^6 / (HCCL_RDMA_RETRY_CNT + 1) / 4.096), rounded up. When both the pull kv timeout and HCCL_RDMA_RETRY_CNT are at their default values, it is recommended to set HCCL_RDMA_TIMEOUT to 15.|
|HCCL_INTRA_ROCE_ENABLE|Used to configure whether the RoCE loop is used for multi-card communication within a server.|

## Complete Example Reference

This sample implements the KV Cache management function in the separate deployment scenario through the LLM-DataDist API. It provides some reference and transformation ideas for using the LLM-DataDist function in different frameworks.

Quick experience: Click [gitcode](https://gitcode.com/cann/ge), select the corresponding branch, and obtain the sample in the "examples/dflow/cpp" directory.
