# Appendix

<!-- md-trans-meta sourceCommit=b7f5e4a67c5c41d02ea0a83a8f907089d6390ff2 translatedAt=2026-11-17T04:06:43.880Z pushedAt=2026-11-17T12:10:40.981Z nl=2 -->

## Basic Concepts

**Table 1** Basic concepts

|Concept|Meaning|
|--|--|
|Prefill phase|Also called the Prompt or full inference phase, it is the phase in which the input passed in by the user is inferred for the first time. The input of this phase is the complete-length statement passed in by the user.|
|Decode phase|Also called the incremental inference phase, it is the phase in which iterative inference is performed based on the output of the Prefill phase. Because the input of each iteration in this phase is the output of the previous inference, it is called incremental inference.|
|PD separation|That is, the Prefill phase and the Decode phase are executed separately.|
|KV Cache|KV Cache is a technology used in large language models to cache the **Key** and **Value** that have already been computed, avoiding repeated computation of the entire context during each generation step. In this document, it refers to the cached Key and Value after computation.|
|PagedAttention|Abbreviated as PA. It is a function that manages the KV Cache of requests by using discrete blocks. Compared with frameworks that do not use this function, it can reduce the memory usage of the KV Cache.|
|block_table|In the PA scenario, it represents the set of block indexes occupied by the KV Cache of a request.|
|Request|It can be understood as the question raised by a user. After the question goes through the Prefill and Decode phases, a corresponding answer is generated. This process is called a request.|
|role|Indicates the service role represented by LLM-DataDist. Service roles are divided into Prefill and Decoder.|
|cluster_id|Indicates the cluster identifier where LLM-DataDist resides. It is mainly used by the Decode side to find the corresponding link through cluster_id, thereby finding the KV Cache of the corresponding request cached on the Prefill side. For the scenario where different shards of a model are deployed to different devices, under the currently supported PD peer sharding scenario, cluster_id is set to the same value across different sharded models.|
|Cluster dynamic scaling|Dynamically adjusts the number of clusters and the ratio of PD clusters according to business busyness and idleness, thereby achieving the purpose of saving resources when idle and improving throughput when busy.|
|D2D transfer|Data transfer from a device to another device.|
|D2H transfer|Data transfer from a device to a host.|
|H2D transfer|Data transfer from a host to a device.|
|D2RH|Data transfer from a device to a remote host.|
|RH2D|Data transfer from a remote host to a device.|
|TTFT|Time To First Token, a metric of LLM inference. It refers to the latency from input to the output of the first token. That is, after a batch of users enters the inference system, the time required for the users to complete the Prefill phase. This is also the response time required for the system to generate the first character.|
|TBT|Time Between Tokens, the latency between tokens, refers to the duration required for each decoding. It reflects how long the interval is between each generated character during the online processing of a large language model system, that is, how smooth the generation process is.|
|Continuous Batching|Continuous batching technology is a method for optimizing computation efficiency in large model inference. It processes multiple inference requests by combining them into one batch, thereby fully utilizing computing resources and improving inference speed and throughput. The core idea of this technology is to reduce idle time during the computation process and achieve maximum utilization of computing resources.|
|Unilateral link establishment|The Client initiates link establishment to the Server.|
|Bilateral link establishment|All LLM-DataDist instances can initiate link establishment simultaneously.|

## Overview of the LLM Inference Process

The launch of ChatGPT means that interactive artificial intelligence has gradually become a commercially mature product, and it has also further promoted the research and progress of the underlying technology, the large language model (LLM). The main LLMs currently in use include the GPT series, the LLAMA series, and the GLM series. Their model base architectures all adopt the Transformer architecture, and are mainly Decode-Only. During inference prediction, such Transformer-Based-Decode-Only LLMs adopt the auto-regressive generation (AR) mode. That is, the generation of each token requires the forward inference process of the LLM model, that is, completing N Transformer Layer computations, which means that the complete generation of a statement containing M tokens requires M complete LLM forward inference processes.

In the Transformer inference process, the KV Cache technology is utilized to reduce the computational load of the Decoding phase, and it has now become a mandatory technology for LLM inference systems. The LLM inference process using KV Cache is usually divided into two phases: Prefill and Decode.

- Prefill phase: The Prompt of the user request is passed into the large model for computation, the intermediate results are written into the KV Cache, and the first token is inferred. As the Prompt Sequence Length grows linearly, services that have requirements on the time to first token (TTFT) usually execute LLM inference in single-batch mode. This phase is a computation-intensive operation.
- Decode phase: The first token of the request is passed into the large model, and the KV cache generated by the preceding text is read from the video memory for computation. After the KV Cache technology is adopted, the computational load of a single token is low, and the main bottleneck lies in the number of parameters transferred. Therefore, multi-batch mode is usually required to improve the utilization rate. This phase is a memory-access-intensive operation.

![](figures/260109142344517.png)

## Reason for PD Disaggregation

From the perspective of implementation, the entire LLM inference process consists of Prefill and multiple rounds of iterative Decode. The prerequisite for implementing Continuous batching in the Decode phase is that each scheduled request needs to have idle computing power to complete the Prefill computation. According to the existing deployment mode, Prefill and Decode are deployed together. When a new Prefill request arrives, it is processed first. As a result, the execution workflow of Decode cannot be effectively guaranteed due to the incremental token latency (TBT). For example, in the figure below, when request5 or request6 arrives, the system may execute the Prefill of request5 or request6 with priority. At this time, the response latency of request 2/3/4 is affected to a certain extent, resulting in unstable TBT.

![](figures/260109142618323.png)

In actual deep learning model deployment, due to the differences in the computation/communication characteristics of the Prefill and Decode phases, in order to improve performance and resource utilization efficiency, the PD separation deployment solution is used to deploy Prefill and Decode in clusters of different specifications and architectures respectively. In addition, with task scheduling at the service layer, when the TTFT and TBT metrics are met, increase the batch concurrency of the decoding phase by combining the continuous batching mechanism to the largest extent, thereby improving the computing power utilization rate.

Based on this solution, it can be seen from the figure below that the execution of Prefill and Decode does not affect each other, and the system can provide users with a stable TBT.

![](figures/260109142712834.png)
