# Overview

<!-- md-trans-meta sourceCommit=b7f5e4a67c5c41d02ea0a83a8f907089d6390ff2 translatedAt=2026-10-29T03:45:40.181Z pushedAt=2026-10-29T07:27:53.062Z nl=2 -->

## LLM-DataDist Overview

In large language model (LLM) inference scenarios, as the model batch size increases, the performance of the Prefill phase degrades linearly, and the Decode phase consumes additional memory. The two phases have different resource requirements, and deploying them together leads to uneven resource allocation and high costs. The detached frame for LLM inference built with LLM-DataDist effectively solves this problem. In the detached frame, Prefill and Decode are deployed separately in clusters with different specifications and architectures, which improves performance and resource utilization and increases the throughput of the LLM inference system.

As a distributed cluster and data management component for LLMs, LLM-DataDist provides high-performance, zero-copy, point-to-point data transfer capabilities, which are exposed to users through simple APIs. LLM-DataDist leverages the diverse communication links of Ascend clusters (RoCE/HCCS/UB) to implement efficient KV Cache transfer across instances and clusters, supports integration with mainstream LLM inference frameworks such as vLLM, and can be used to build distributed data management systems. The main functions of LLM-DataDist include link management and cache management.

- Link management is used to establish and disconnect links between clusters, enabling dynamic cluster scaling.
- Cache management is used to manage the KV Cache and provides point-to-point KV Cache transfer between the P side and the D side (hereinafter, P represents Prefill and D represents Decode).

![](figures/260109143238748.png)

## LLM-DataDist Scenarios

Build a PD detached frame for large model inference using LLM-DataDist.

In large model inference, the Prefill phase passes the user request prompt to the large model for computation, writes the intermediate results to the KV Cache, and outputs the first token. In the Decode phase, the first token of the request is passed to the large model, which reads the previously generated KV Cache from the video memory and then performs computation. For details about the KV Cache-based large model inference process, see [Introduction to the LLM Inference Process](appendices.md#reason-for-pd-disaggregation).

In the PD detached frame for large model inference, Prefill and Decode are deployed in clusters of different specifications and architectures to improve performance and resource utilization. The PD detached frame can improve the throughput of the large model inference system. For details, see [Introduction to the LLM Inference Process](appendices.md#reason-for-pd-disaggregation).

In the PD detached frame for large model inference, the KV Cache generated in the Prefill phase needs to be transferred to Decode, and then the Decode phase performs incremental iterative inference. As a large model distributed cluster and data management component, LLM-DataDist is exposed to users through simple APIs. The following figure shows how to build a PD detached frame for large model inference. LLM-DataDist provides KV Cache transfer and link management between the Prefill Node and the Decode Node.

![](figures/260107091904908.png)
