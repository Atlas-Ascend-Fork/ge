# Overview

<!-- md-trans-meta sourceCommit=b7f5e4a67c5c41d02ea0a83a8f907089d6390ff2 translatedAt=2026-11-17T04:13:09.404Z pushedAt=2026-11-18T01:46:01.986Z nl=2 -->

## LLM-DataDist Overview

In large model inference scenarios, as the model batch size increases, the performance of the Prefill phase decreases linearly, and the Decode phase consumes additional memory. The two phases have different resource requirements, and deploying them together leads to uneven resource allocation and persistently high costs. The decoupled large model inference framework built with LLM-DataDist effectively resolves this issue. In the decoupled framework, Prefill and Decode are deployed separately in clusters of different specifications and architectures, which improves performance and resource utilization efficiency and increases the throughput of the large model inference system.

As a component for managing large model distributed clusters and data, LLM-DataDist provides high-performance, zero-copy point-to-point data transmission capability, which is open to users through simple APIs. LLM-DataDist utilizes the diverse communication links of the Ascend cluster (RoCE/HCCS/UB) to implement efficient KV Cache transmission across instances and clusters, supports integration with mainstream LLM inference frameworks such as vLLM, and can be used to build distributed data management systems. The main functions of LLM-DataDist include link management and cache management.

- Link management is used to establish and disconnect links between clusters, implementing the capability of dynamic cluster scaling.
- Cache management is used to manage KV Cache, providing the capability of point-to-point KV Cache transmission between PD (hereinafter, P represents Prefill and D represents Decode).

![](figures/260109143238748.png)

## LLM-DataDist Scenarios

LLM-DataDist is used to build the decoupled PD framework for large model inference.

In large model inference, the Prefill phase passes the user request Prompt to the large model for computation, writes the intermediate results to the KV Cache, and outputs the first token. In the Decode phase, the first token of the request is passed to the large model, and the previously generated KV Cache is read from the video memory for further computation. For details about the large model inference process based on KV Cache, see [Overview of the LLM Inference Process](appendices.md#reason-for-pd-disaggregation).

In the decoupled PD framework for large model inference, Prefill and Decode are deployed separately in clusters of different specifications and architectures to improve performance and resource utilization efficiency. The decoupled PD framework can increase the throughput of the large model inference system. For details, see [Overview of the LLM Inference Process](appendices.md#reason-for-pd-disaggregation).

In the decoupled PD framework for large model inference, the KV Cache generated in the Prefill phase needs to be transmitted to Decode, and then the Decode phase performs incremental iterative inference. As a component for managing large model distributed clusters and data, LLM-DataDist is open to users through simple APIs to build the decoupled PD framework for large model inference, as shown in the following figure. LLM-DataDist provides KV Cache transmission and link management between the Prefill Node and the Decode Node.

![](figures/260107091904908.png)
