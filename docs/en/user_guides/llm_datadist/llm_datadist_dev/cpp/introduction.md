# Overview

<!-- md-trans-meta sourceCommit=b7f5e4a67c5c41d02ea0a83a8f907089d6390ff2 translatedAt=2026-10-29T03:41:07.468Z pushedAt=2026-10-29T07:20:33.823Z nl=2 -->

## Introduction to LLM-DataDist

In large language model (LLM) inference scenarios, as the model batch size increases, the performance of the Prefill phase decreases linearly, while the Decode phase consumes additional memory. The two phases have different resource requirements, and deploying them together leads to uneven resource allocation and persistently high costs. The detached framework for LLM inference built with LLM-DataDist effectively solves this problem. In the detached framework, Prefill and Decode are deployed in clusters with different specifications and architectures, which improves performance and resource utilization efficiency and increases the throughput of the LLM inference system.

As a distributed cluster and data management component for LLMs, LLM-DataDist provides high-performance, zero-copy point-to-point data transfer capabilities, which are exposed to users through simple APIs. LLM-DataDist leverages the diverse communication links of Ascend clusters (RoCE/HCCS/UB) to enable efficient KV Cache transfer across instances and clusters, supports integration with mainstream LLM inference frameworks such as vLLM, and can be used to build distributed data management systems. The main functions of LLM-DataDist include link management and cache management.

- Link management is used to establish and disconnect links between clusters, enabling dynamic scaling of clusters.
- Cache management is used to manage KV Cache and provides the capability of point-to-point KV Cache transfer between PD (hereinafter, the P side represents Prefill and the D side represents Decode).

![](figures/260109143238748.png)

## LLM-DataDist Scenarios

Build an LLM inference PD detached framework through LLM-DataDist.

In LLM inference, the Prefill phase passes the user request prompt to the LLM for computation, writes the intermediate results to the KV cache, and outputs the first token. In the Decode phase, the first token of the request is passed to the LLM, and the previously generated KV cache is read from the memory for further computation. For details about the KV cache-based LLM inference process, see [Introduction to the LLM Inference Process](appendices.md#reason-for-pd-disaggregation).

In the LLM inference PD detached framework, to improve performance and resource utilization, Prefill and Decode are deployed in clusters with different specifications and architectures. The PD detached framework can improve the throughput of the LLM inference system. For details, see [Introduction to the LLM Inference Process](appendices.md#reason-for-pd-disaggregation).

In the LLM inference PD detached framework, the KV cache generated in the Prefill phase needs to be transferred to Decode, and then the Decode phase performs incremental iterative inference. As a distributed cluster and data management component for LLMs, LLM-DataDist is exposed to users through simple APIs to build the LLM inference PD detached framework, as shown in the following figure. LLM-DataDist provides KV cache transfer and link management between the Prefill node and the Decode node.

![](figures/260107091904908.png)
