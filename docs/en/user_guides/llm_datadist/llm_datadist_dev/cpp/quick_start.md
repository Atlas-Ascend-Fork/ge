# Quick Start

<!-- md-trans-meta sourceCommit=316a371f647c811975e167b992deb0441ff5cb70 translatedAt=2026-10-29T03:42:37.599Z pushedAt=2026-10-29T07:23:54.382Z nl=2 -->

## Overall Development Process

**Overview**

For ease of subsequent description, we abstract different inference frameworks into several modules:

- Resource initialization module.
- KV Cache management module, including the creation, allocation (PA scenario), and destruction of KV Cache memory.
- Model inference module.
- Resource release module.

This section mainly describes how developers enable the LLM-DataDist capability in an inference framework.

**Development Process**

1. Locate the resource initialization module in the inference framework, and call the LLM-DataDist initialization API and connection establishment API in this phase.
2. Locate the KV Cache management module in the inference framework, and call the LLM-DataDist KV Cache application API in this phase to convert the applied KV Cache into the KV Cache type of different frameworks for inference.
3. The inference framework needs to split the Prefill phase and the Decode phase, deploy the inference scripts separately, and deploy them to different cluster nodes. Before the Decode phase is executed, the output from the Prefill phase needs to be received as input, and the KV Cache transfer API provided by LLM-DataDist is called to pull or push the KV Cache cached on the Prefill side.
4. Run the Prefill inference script and the Decode inference script separately.
5. Release LLM-DataDist-related resources in the framework resource release module.

## Environment Preparation

Supported product forms are as follows:
<!-- npu="910b" id1 -->

- Atlas A2 Training Series/Atlas A2 Inference Series: For the Atlas A2 Training Series/Atlas A2 Inference Series, only the Atlas 800I A2 inference server and the A200I A2 Box heterogeneous component are supported.
<!-- end id1 -->

<!-- npu="A3" id2 -->
- Atlas A3 products
<!-- end id2 -->

Install the driver, firmware, and CANN software by referring to [CANN Software Installation](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/920/softwareinst/instg/instg_0000.html?OS=openEuler&InstallType=netyum).

Use hccn\_tool to query the Device IP and perform inter-card network detection. Ensure that RDMA links exist between cards on each cluster; otherwise, the LLM-DataDist capability cannot be enabled. hccn\_tool For details, see [HCCN Tool API Reference](https://support.huawei.com/enterprise/en/ascend-computing/ascend-hdk-pid-252764743?category=developer-documents&subcategory=interface-reference). The following are common commands for reference.

|Command|Scenario|
|--|--|
|hccn_tool [-i %d] -link -g|Obtains the link status of the network port of a specified Device. -i specifies the Device. Example: `hccn_tool -i 0 -link -g`|
|hccn_tool [-i %d] -ip -g|Obtains the IP address and subnet mask. -i specifies the Device. Example: `hccn_tool -i 0 -ip -g`|
|hccn_tool [-i %d] -ping -g [address %s ]|Obtains the ping result from a specified device to the destination address. -i specifies a Device on the current server, and address specifies the destination address to ping. Example: `hccn_tool -i 0 -ping -g address 192.168.2.1`|

The following environment variables are also involved when using LLM-DataDist. For details, see [Environment Variables](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

|Name|Scenario|
|--|--|
|HCCL_RDMA_TC, HCCL_RDMA_SL|When customers have planned the parameter plane network themselves, they specify types and priorities for various service traffic. These two environment variables are used to set the traffic type and priority of parameter plane collective communication traffic on the network, so as to adapt to the customer's network traffic planning requirements.|
|HCCL_RDMA_RETRY_CNT, HCCL_RDMA_TIMEOUT|Correspond to the retry count of the RDMA NIC and the coefficient timeout of the retransmission timeout, respectively. Setting them too large makes the system insensitive to network anomalies and unable to detect network faults. Setting them too small may cause transient network interruptions to directly interrupt services, which cannot be masked by the NIC hardware. Users can set appropriate values based on their own network conditions. For example, they can configure the values based on the time range of most transient interruptions. The following formula is recommended to reduce the impact of network jitter: HCCL_RDMA_TIMEOUT = log2(pull KV timeout * 10^6 / (HCCL_RDMA_RETRY_CNT + 1) / 4.096), rounded up. When both the pull KV timeout and HCCL_RDMA_RETRY_CNT are set to their default values, it is recommended to set HCCL_RDMA_TIMEOUT to 15.|
|HCCL_INTRA_ROCE_ENABLE|Used to configure whether to use the RoCE loop for multi-card communication within a server.|

## Complete Sample Reference

This example implements KV Cache management in the disaggregated deployment scenario through LLM-DataDist APIs, providing references and adaptation ideas for different frameworks to use LLM-DataDist capabilities.

Quick experience: Click [gitcode](https://gitcode.com/cann/ge), select the corresponding branch, and obtain the example from the "examples/dflow/cpp" directory.
