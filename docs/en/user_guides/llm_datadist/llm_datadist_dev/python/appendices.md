# Appendix

<!-- md-trans-meta sourceCommit=b7f5e4a67c5c41d02ea0a83a8f907089d6390ff2 translatedAt=2026-11-17T04:11:32.708Z pushedAt=2026-11-17T12:33:13.137Z nl=2 -->

## Basic Concepts

**Table 1** Basic concepts

|Concept|Meaning|
|--|--|
|Prefill phase|Also called the Prompt or full inference phase, it is the phase in which the input from the user is inferred for the first time. The input of this phase is the full-length statement passed in by the user.|
|Decode phase|Also called the incremental inference phase, it is the phase in which iterative inference is performed based on the output of the Prefill phase. Since the input of each iteration in this phase is the output of the previous inference, it is called incremental inference.|
|PD separation|The Prefill phase and the Decode phase are executed separately.|
|KV Cache|KV Cache is a technology used in large language models to cache the **Key** and **Value** that have already been computed, avoiding repeated computation of the entire context in each generation step. In this document, it refers to the cache of the computed Key and Value.|
|PagedAttention|Abbreviated as PA. It is a feature that manages the KV Cache of requests by using discrete blocks. Compared with frameworks that do not use this feature, it can reduce the memory usage of the KV Cache.|
|block_table|In the PA scenario, it represents the set of block indexes occupied by the KV Cache of a request.|
|Request|It can be understood as the question raised by a user. The question has a corresponding answer after the Prefill and Decode phases, and this process is called a request.|
|role|Indicates the service role represented by LLM-DataDist. Service roles are divided into Prefill and Decoder.|
|cluster_id|Indicates the cluster identifier where LLM-DataDist resides. It is mainly used by the Decode side to find the corresponding link through cluster_id, so as to find the KV Cache of the corresponding request cached on the Prefill side. For the scenario where different slices of a model are deployed to different devices, under the currently supported PD peer slicing scenario, cluster_id is set to the same value across different model slices.|
|Dynamic cluster scaling|Dynamically adjusts the number of clusters and the ratio of PD clusters according to service busyness and idleness, so as to save resources when idle and improve throughput when busy.|
|D2D transfer|Refers to data transfer from a Device to a Device.|
|D2H transfer|Refers to data transfer from a Device to a Host.|
|H2D transfer|Refers to data transfer from a Host to a Device.|
|D2RH|Refers to data transfer from a Device to a remote Host.|
|RH2D|Refers to data transfer from a remote Host to a Device.|
|TTFT|Time To First Token, a metric of LLM inference. It refers to the latency from input to the output of the first token. That is, after a batch of users enters the inference system, the time required for the users to complete the Prefill phase. It is also the response time required for the system to generate the first character.|
|TBT|Time Between Tokens, the latency between tokens, refers to the duration required for each decoding. It reflects how long the interval is between generating each character during the online processing of a large language model system, that is, how smooth the generation process is.|
|Continuous Batching|Continuous batching is a method for optimizing computation efficiency in large model inference. It combines multiple inference requests into a batch for processing, thereby making full use of computing resources and improving inference speed and throughput. The core idea of this technology is to reduce idle time in the computation process and maximize the utilization of computing resources.|
|Unilateral link establishment|The Client initiates link establishment to the Server.|
|Bilateral link establishment|All LLM-DataDist instances can initiate link establishment simultaneously.|

## Overview of the LLM Inference Process

The launch of ChatGPT means that interactive artificial intelligence has gradually become a mature commercial product, and it has also further promoted the research and progress of the underlying technology, the large language model (LLM). The main LLMs currently in use include the GPT series, the LLAMA series, and the GLM series. Their model base architectures all adopt the Transformer architecture, with Decode-Only as the main form. During inference prediction, this type of Transformer-based Decode-Only LLM adopts the auto-regressive generation (AR) mode. That is, the generation of each token requires the forward inference process of the LLM model, that is, completing N Transformer Layer computations, which means that the complete generation of a statement containing M tokens requires M complete LLM forward inference processes.

In the Transformer inference process, the KV Cache technology can be used to reduce the computational load of the Decoding phase, and it has become a mandatory technology for LLM inference systems. The LLM inference process using KV Cache is usually divided into two phases: Prefill and Decode.

- Prefill phase: The Prompt of the user request is passed into the large model for computation, the intermediate results are written into the KV Cache, and the first token is inferred. As the prompt sequence length grows linearly, services that have requirements on the time to first token (TTFT) metric usually execute LLM inference in a single-batch mode. This phase is a compute-intensive operation.
- Decode phase: The previous token of the request is passed into the large model, and the KV Cache generated by the preceding text is read from the video memory for computation. After the KV Cache technology is adopted, the computational load of a single token is low, and the main bottleneck lies in moving the parameter volume. Therefore, a multi-batch mode is usually required to improve utilization. This phase is a memory-access-intensive operation.

![](figures/260109142344517.png)

## Reason for PD Disaggregation

From the implementation perspective, the entire LLM inference process consists of Prefill and multiple rounds of iterative Decode. The basis for implementing Continuous batching in the Decode phase is that each scheduled request needs idle computing power to complete the Prefill computation. In the existing deployment mode, Prefill and Decode are deployed together. When a new Prefill request arrives, it is processed first. As a result, the Decode execution workflow cannot be effectively guaranteed due to the time between tokens (TBT). For example, in the following figure, when request5 or request6 arrives, the system may execute the Prefill of request 5 or request 6 first. At this time, the response latency of request2/3/4 is affected to a certain extent, resulting in unstable TBT.

![](figures/260109142618323.png)

In actual deep learning model deployment, due to the differences in the computation/communication characteristics of the Prefill and Decode phases, in order to improve performance and resource utilization efficiency, the PD separation deployment solution deploys Prefill and Decode in clusters of different specifications and architectures respectively. Combined with task scheduling at the service layer, and when the TTFT and TBT metrics are met, the batch concurrency of the Decode phase is increased as much as possible in combination with the Continuous batching mechanism, thereby improving computing power utilization.

Based on this solution, it can be seen from the following figure that the execution of Prefill and Decode does not affect each other, and the system can provide users with a stable TBT.

![](figures/260109142712834.png)
