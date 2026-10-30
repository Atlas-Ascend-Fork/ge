# Appendix

<!-- md-trans-meta sourceCommit=b7f5e4a67c5c41d02ea0a83a8f907089d6390ff2 translatedAt=2026-10-29T03:43:56.821Z pushedAt=2026-10-29T07:24:21.962Z nl=2 -->

## Basic Concepts

**Table 1** Basic concepts

|Concept|Description|
|--|--|
|Prefill phase|Also called the prompt or full inference phase, it is the phase in which the user's input is inferred for the first time. The input of this phase is the complete statement passed in by the user.|
|Decode phase|Also called the incremental inference phase, it is the phase in which iterative inference is performed based on the output of the Prefill phase. Because the input of each iteration in this phase is the output of the previous inference, it is called incremental inference.|
|PD separation|The Prefill phase and the Decode phase are executed separately.|
|KV Cache|KV Cache is a technique used in large language models to cache the **Key** and **Value** that have already been computed, avoiding repeated computation of the entire context during each generation step. In this document, it refers to the cached Key and Value after computation.|
|PagedAttention|Abbreviated as PA. It is a feature that manages the KV Cache of requests by using discrete blocks. Compared with frameworks that do not use this feature, it can reduce the memory usage of the KV Cache.|
|block_table|In the PA scenario, it represents the set of block indexes occupied by the KV Cache of a request.|
|Request|It can be understood as a question raised by a user. After the Prefill and Decode phases, a corresponding answer is generated. This process is called a request.|
|role|Indicates the service role represented by LLM-DataDist. Service roles are divided into Prefill and Decoder.|
|cluster_id|Indicates the cluster identifier where LLM-DataDist resides. It is mainly used by the Decode side to find the corresponding link through cluster_id, thereby locating the KV Cache of the corresponding request cached on the Prefill side. For the scenario where different shards of a model are deployed to different devices, when PD peer sharding is supported, cluster_id is set to the same value across different model shards.|
|Dynamic cluster scaling|Dynamically adjusts the number of clusters and the ratio of PD clusters according to service busyness and idleness, thereby saving resources when idle and improving throughput when busy.|
|D2D transmission|Refers to data transmission from a Device to a Device.|
|D2H transmission|Refers to data transmission from a Device to a Host.|
|H2D transmission|Refers to data transmission from a Host to a Device.|
|D2RH|Refers to data transmission from a Device to a remote Host.|
|RH2D|Refers to data transmission from a remote Host to a Device.|
|TTFT|Time To First Token, an indicator of LLM inference. It refers to the latency from input to the output of the first token. That is, after a batch of users enters the inference system, the time required for the users to complete the Prefill phase. It is also the response time required for the system to generate the first character.|
|TBT|Time Between Tokens, the inter-token latency, which refers to the time required for each decoding. It reflects the interval between generating each character during online processing of a large language model system, that is, how smoothly the generation process proceeds.|
|Continuous Batching|Continuous batching is a method for optimizing computation efficiency in large model inference. It combines multiple inference requests into a batch for processing, thereby fully utilizing computation resources and improving inference speed and throughput. The core idea of this technique is to reduce idle time during computation and maximize the utilization of computation resources.|
|Unilateral link establishment|The Client initiates link establishment to the Server.|
|Bilateral link establishment|All LLM-DataDist instances can initiate link establishment simultaneously.|

## Overview of the LLM Inference Process

The launch of ChatGPT marks the maturity of interactive AI as a commercial product, and it has further driven the research and advancement of the underlying large language model (LLM) technology. The mainstream LLMs in use today include the GPT series, the LLAMA series, and the GLM series. Their base architectures all adopt the Transformer architecture, primarily in the decode-only form. During inference and prediction, such Transformer-based decode-only LLMs use the auto-regressive generation (AR) mode. That is, the generation of each token requires a forward inference process through the LLM model, which completes N Transformer layer computations. This means that fully generating a statement containing M tokens requires M complete forward inference processes through the LLM.

In the Transformer inference process, the KV Cache technique is used to reduce the computation workload in the decoding stage, and it has become a mandatory technique for LLM inference systems. The LLM inference process that uses KV Cache is typically divided into two stages: prefill and decode.

- Prefill stage: The prompt requested by the user is fed into the large model for computation. The intermediate results are written into the KV Cache, and the first token is inferred. As the prompt sequence length grows linearly, services that have requirements on the time to first token (TTFT) metric usually execute LLM inference in single-batch mode. This stage is a computation-intensive operation.
- Decode stage: The first token of the request is fed into the large model, and the KV Cache generated earlier is read from the video memory for computation. After the KV Cache technique is adopted, the computation workload of a single token is low, and the main bottleneck lies in moving the parameter volume. Therefore, multi-batch mode is usually required to improve utilization. This stage is a memory-access-intensive operation.

![](figures/260109142344517.png)

## Reason for PD Disaggregation

From an implementation perspective, the entire LLM inference process consists of Prefill and multiple rounds of iterative Decode. The prerequisite for achieving continuous batching in the Decode phase is that each scheduled request must have idle computing power to complete the Prefill computation. Under the existing deployment mode, Prefill and Decode are deployed together. When a new Prefill request arrives, it is processed with priority, which means the Decode execution flow cannot be effectively guaranteed due to the incremental token latency (TBT). For example, in the following figure, when request5 or request6 arrives, the system may prioritize the Prefill of request5 or request6. In this case, the response latency of request2/3/4 is affected to a certain extent, resulting in unstable TBT.

![](figures/260109142618323.png)

In actual deep learning model deployment, because the Prefill and Decode phases differ in their computation/communication characteristics, the PD separation deployment scheme is adopted to deploy Prefill and Decode in clusters of different specifications and architectures, so as to improve performance and resource utilization efficiency. Combined with task scheduling at the service layer, and on the premise of meeting the TTFT and TBT metrics, the scheme works with the continuous batching mechanism to maximize the batch concurrency in the Decode phase and improve computing power utilization.

Based on this scheme, as shown in the following figure, the execution of Prefill and Decode does not affect each other, and the system can provide users with a stable TBT.

![](figures/260109142712834.png)
