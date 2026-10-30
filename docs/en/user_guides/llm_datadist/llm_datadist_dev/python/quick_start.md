# Quick Start

<!-- md-trans-meta sourceCommit=316a371f647c811975e167b992deb0441ff5cb70 translatedAt=2026-10-29T03:48:25.830Z pushedAt=2026-10-29T07:30:42.095Z nl=2 -->

## Overall Development Process

**Overview**

For ease of subsequent description, we abstract different inference frameworks into the following modules:

- Resource initialization module.
- KV Cache management module. This includes the creation, allocation (in PA scenarios), and destruction of KV Cache memory.
- Model inference module.
- Resource release module.

This section mainly describes how developers can enable LLM-DataDist capabilities in an inference framework.

**Development Process**

1. Locate the resource initialization module in the inference framework, and call the LLM-DataDist initialization API and connection establishment API in this phase.
2. Locate the KV Cache management module in the inference framework, and call the LLM-DataDist KV Cache application API in this phase to convert the applied KV Cache into the KV Cache type of different frameworks for inference.
3. The inference framework needs to split the Prefill phase and the Decode phase, deploy the inference scripts separately, and deploy them to different cluster nodes. Before the Decode phase is executed, the output from the Prefill phase needs to be received as input, and the KV Cache transfer API provided by LLM-DataDist needs to be called to pull or push the KV Cache cached on the Prefill side.
4. Execute the Prefill inference script and the Decode inference script separately.
5. Release LLM-DataDist-related resources in the framework resource release module.

## Environment Preparation

Supported product forms are as follows:

<!-- npu="910b" id1 -->

- Atlas A2 training series products/Atlas A2 inference series products: For Atlas A2 training series products/Atlas A2 inference series products, only the Atlas 800I A2 inference server and the A200I A2 Box heterogeneous component are supported.
<!-- end id1 -->

<!-- npu="A3" id2 -->
- Atlas A3 products
<!-- end id2 -->

Install the driver, firmware, and CANN software by referring to [CANN Software Installation](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/920/softwareinst/instg/instg_0000.html?OS=openEuler&InstallType=netyum).

Use hccn\_tool to query the Device IP and perform inter-card network detection. RDMA links must be available between cards on each cluster; otherwise, the LLM-DataDist capability cannot be enabled. hccn\_tool For details, see [HCCN Tool API Reference](https://support.huawei.com/enterprise/en/ascend-computing/ascend-hdk-pid-252764743?category=developer-documents&subcategory=interface-reference). The following are common commands for reference.

|Command|Scenario|
|--|--|
|hccn_tool [-i %d] -link -g|Obtains the link status of the network port of a specified Device. -i specifies the Device. Sample: `hccn_tool -i 0 -link -g`|
|hccn_tool [-i %d] -ip -g|Obtains the IP address and subnet mask. -i specifies the Device. Sample: `hccn_tool -i 0 -ip -g`|
|hccn_tool [-i %d] -ping -g [address %s ]|Obtains the ping result from a specified device to the destination address. -i specifies a Device of the current server, and address specifies the destination address of the ping. Sample: `hccn_tool -i 0 -ping -g address 192.168.2.1`|

The following environment variables are also involved when using LLM-DataDist. For details, see [Environment Variables](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

|Name|Scenario|
|--|--|
|HCCL_RDMA_TC, HCCL_RDMA_SL|When customers have planned the parameter plane network by themselves, they specify types and priorities for various service traffic. These two environment variables set the traffic type and priority of parameter plane collective communication traffic on the network to adapt to the customer's network traffic planning requirements.|
|HCCL_RDMA_RETRY_CNT, HCCL_RDMA_TIMEOUT|These correspond to the configuration values of the RDMA hardware retransmission timeout feature, representing the retry count and retry timeout respectively. Setting them too large makes the system insensitive to network anomalies and unable to detect network faults. Setting them too small may cause transient network interruptions to directly interrupt services, which cannot be shielded by the NIC hardware. Users can set appropriate values based on their own network conditions. For example, they can configure the values based on the time range of most transient interruptions. It is recommended to configure them according to the following formula to reduce the impact of network jitter. HCCL_RDMA_TIMEOUT=log2(pull kv timeout * 10^6 / (HCCL_RDMA_RETRY_CNT + 1) / 4.096), rounded up. When both the pull kv timeout and HCCL_RDMA_RETRY_CNT are at their default values, it is recommended to set HCCL_RDMA_TIMEOUT to 15.|
|HCCL_INTRA_ROCE_ENABLE|Used to configure whether to use the RoCE loop for communication between multiple cards within a server.|
|AUTO_USE_UC_MEMORY|Controls whether the system allows operators to move data without going through the L2 Cache. Before using LLM-DataDist, if this environment variable is not configured, it will be set to 0 during the use of LLM-DataDist, indicating that all operators must move data through the L2 Cache.|

## Complete Sample Reference

This sample uses the LLAMA model of transformers as an example, mainly showing the script changes before and after PD separation, and provides a reference for how to change a non-separated script into a PD-separated script. For the PD-separated script sample, click [Gitee](https://gitee.com/ascend/torchair/tree/master#version_match), download the sample package of the corresponding version according to the version matching table, and obtain the sample from the “npu\_tuned\_model/llm/llama/benchmark/pd\_separate” directory. In the sample, the full model and the incremental model are separated and deployed to different cluster nodes for execution.

For how the separated scripts above are scheduled by the service layer during the entire inference process, refer to the following steps.

1. When a user request is triggered, the service layer schedules the user request to the entire cluster and executes inference using the full script. It transfers the information required by the incremental script to the execution node of the incremental script. At this point, the full cluster nodes can receive new user requests delivered by the service layer.
2. The incremental cluster receives the request information corresponding to the full cluster, pulls the KV Cache of the corresponding request (which has already been computed on the full cluster nodes), and performs batch grouping based on the batch size of the incremental model to execute incremental inference.
3. When a request on the incremental cluster completes inference and frees up the corresponding batch position, the incremental cluster receives a new request sent from the full cluster and repeats steps 2 and 3.
4. The full cluster repeats step 1, and the incremental cluster repeats steps 2 and 3 until the service ends, at which point the full and incremental clusters exit.

For more sample code, click [gitcode](https://gitcode.com/cann/ge), select the corresponding branch, and download it from the "examples/dflow/python" directory.
