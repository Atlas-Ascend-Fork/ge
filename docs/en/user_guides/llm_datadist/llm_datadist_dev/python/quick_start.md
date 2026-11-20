# Quick Start

<!-- md-trans-meta sourceCommit=c2fc584b172edb77accd1ca282941523eaecad4c translatedAt=2026-11-17T04:15:30.769Z pushedAt=2026-11-18T01:51:00.822Z nl=2 -->

## Development Process

**Overview**

For ease of subsequent description, different inference frameworks are abstracted into the following modules:

- Resource initialization module.
- KV Cache management module. This includes the creation, allocation (PA scenario), and destruction of KV Cache memory.
- Model inference module.
- Resource release module.

This section mainly describes how developers enable the LLM-DataDist capability in an inference framework.

**Development Process**

1. Locate the resource initialization module in the inference framework, and call the LLM-DataDist initialization API and link establishment API in this phase.
2. Locate the KV Cache management module in the inference framework, and call the LLM-DataDist KV Cache application API in this phase to convert the applied KV Cache into the KV Cache type of different frameworks for inference.
3. The inference framework needs to split the Prefill phase and the Decode phase, separate and deploy the inference scripts to different cluster nodes. Before the Decode phase is executed, the output from the Prefill phase needs to be received as input, and the KV Cache transmission API provided by LLM-DataDist is called to pull or push the KV Cache cached on the Prefill side.
4. Execute the Prefill inference script and the Decode inference script separately.
5. Release LLM-DataDist-related resources in the framework resource release module.

## Environment Preparation

The supported product forms are as follows:

<!-- npu="910b" id1 -->

- Atlas A2 product series: For the Atlas A2 product series, only the Atlas 800I A2 inference server and the A200I A2 Box heterogeneous component are supported.
<!-- end id1 -->

<!-- npu="A3" id2 -->
- Atlas A3 product series
<!-- end id2 -->

Install the driver, firmware, and CANN software by referring to [CANN Software Installation](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/softwareinst/instg/instg_0000.html?OS=openEuler&InstallType=netyum).

Use hccn\_tool to query the Device IP and perform inter-card network detection. RDMA link connections are required between cards on each cluster; otherwise, the LLM-DataDist capability cannot be enabled. hccn\_tool For details, see [HCCN Tool API Reference](https://support.huawei.com/enterprise/en/ascend-computing/ascend-hdk-pid-252764743?category=developer-documents&subcategory=interface-reference). The following are common commands for reference.

|Command|Scenario|
|--|--|
|hccn_tool [-i %d] -link -g|Obtains the Link status of the network port of a specified Device. -i specifies the Device. Example: `hccn_tool -i 0 -link -g`|
|hccn_tool [-i %d] -ip -g|Obtains the IP address and subnet mask. -i specifies the Device. Example: `hccn_tool -i 0 -ip -g`|
|hccn_tool [-i %d] -ping -g [address %s ]|Obtains the ping result from a specified device to the destination address. -i specifies a Device of the current server, and address specifies the destination address of the ping. Example: `hccn_tool -i 0 -ping -g address 192.168.2.1`|

The following environment variables are also involved when using LLM-DataDist. For details, see [Environment Variables](https://gitcode.com/cann/docs/blob/master/docs/en/env-vars/README.md).

|Name|Scenario|
|--|--|
|HCCL_RDMA_TC, HCCL_RDMA_SL|When customers have made their own planning for the parameter plane network, they specify types and priorities for various service traffic. These two environment variables are used to set the traffic type and priority of parameter plane collective communication traffic on the network, so as to adapt to the customer's network traffic planning requirements.|
|HCCL_RDMA_RETRY_CNT, HCCL_RDMA_TIMEOUT|These correspond to the configuration values of the RDMA hardware retransmission timeout feature, corresponding to the retry count and retry timeout respectively. Setting them too large makes the system insensitive to network anomalies and unable to detect network faults. Setting them too small easily causes network flapping to directly interrupt services, which cannot be shielded by the NIC hardware. Users can set appropriate values based on their own network conditions. For example, they can be configured based on the time range of most flapping. The following formula is recommended to reduce the impact of network jitter. HCCL_RDMA_TIMEOUT=log2(pull kv timeout * 10^6 / (HCCL_RDMA_RETRY_CNT + 1) / 4.096), rounded up. When both the pull kv timeout and HCCL_RDMA_RETRY_CNT are at their default values, HCCL_RDMA_TIMEOUT is recommended to be configured as 15.|
|HCCL_INTRA_ROCE_ENABLE|Used to configure whether the RoCE loop is used for multi-card communication within a server.|
|AUTO_USE_UC_MEMORY|Controls whether the system allows operators to move data without passing through the L2 Cache. Before using LLM-DataDist, if this environment variable is not configured, it is set to 0 during the use of LLM-DataDist, indicating that all operator data movement must pass through the L2 Cache.|

## Complete Example Reference

The LLAMA model of transformers as an example, mainly showing the script change points before and after PD separation, and provides a reference for how to change a non-separation script into a PD separation script. For the PD separation script example, click [Gitcode](https://gitcode.com/Ascend/torchair/blob/master/COMPATIBILITY.md#%E8%AF%A6%E7%BB%86%E9%85%8D%E5%A5%97%E8%A1%A8), download the sample package of the corresponding version according to the version compatibility table, and obtain the example from the "npu\_tuned\_model/llm/llama/benchmark/pd\_separate" directory. In the example, the full model and the incremental model are separated and deployed to different cluster nodes for execution.

For how the above separation script is scheduled by the service layer in the entire inference process, refer to the following steps.

1. When a user request is triggered, the service layer schedules the user request to the full cluster to execute the inference of the full script. The information required by the incremental script is transmitted to the execution node of the incremental script. At this time, the full cluster node can receive new user requests delivered by the service layer.
2. The incremental cluster receives the request information corresponding to the full cluster, pulls the KV Cache of the corresponding request (which has been computed on the full cluster node), and performs batch grouping according to the batch size of the incremental model to execute incremental inference.
3. When a request on the incremental cluster completes inference and frees the corresponding batch position, the incremental cluster receives a new request from the full cluster and repeats steps 2 and 3.
4. The full cluster repeats step 1, and the incremental cluster repeats steps 2 and 3 until the service ends, at which point the full and incremental clusters exit.

For more example code, click [gitcode](https://gitcode.com/cann/ge), select the corresponding branch, and download it from the "examples/dflow/python" directory.
