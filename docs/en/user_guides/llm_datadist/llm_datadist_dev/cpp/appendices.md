# Appendix

<!-- md-trans-meta sourceCommit=b7f5e4a67c5c41d02ea0a83a8f907089d6390ff2 translatedAt=2026-10-29T03:40:30.027Z pushedAt=2026-10-29T07:20:47.978Z nl=2 -->

## Basic Concepts

**Table 1** Basic concepts

|Concept|Description|
|--|--|
|Prefill stage|Also called the prompt stage or full inference stage, it is the stage in which the user's input is inferred for the first time. The input of this stage is the complete statement passed in by the user.|
|Decode stage|Also called the incremental inference stage, it is the stage in which iterative inference is performed based on the output of the Prefill stage. Because the input of each iteration in this stage is the output of the previous inference, it is called incremental inference.|
|PD separation|The Prefill stage and the Decode stage are executed separately.|
|KV Cache|KV Cache is a technique used in large language models to cache the **Key** and **Value** that have been computed, avoiding repeated computation of the entire context during each generation step. In this document, it refers to the cache of the computed Key and Value.|
|PagedAttention|Abbreviated as PA. It is a feature that manages the KV Cache of a request by using discrete blocks. Compared with frameworks that do not use this feature, it can reduce the memory usage of the KV Cache.|
|block_table|In the PA scenario, it represents the set of block indexes occupied by the KV Cache of a request.|
|Request|It can be understood as the question raised by the user. After the question goes through the Prefill and Decode stages, a corresponding answer is generated. This process is called a request.|
|role|Indicates the service role represented by LLM-DataDist. Service roles are divided into Prefill and Decoder.|
|cluster_id|Indicates the cluster identifier of the cluster where LLM-DataDist resides. It is mainly used by the Decode side to find the corresponding link through cluster_id, so as to find the KV Cache of the corresponding request cached on the Prefill side. For the scenario where different slices of a model are deployed to different devices, when PD peer slicing is supported, cluster_id is set to the same value across different sliced models.|
|Dynamic cluster scaling|Dynamically adjusts the number of clusters and the ratio of PD clusters based on service busyness, so as to save resources during idle periods and improve throughput during busy periods.|
|D2D transmission|Refers to data transmission from a Device to a Device.|
|D2H transmission|Refers to data transmission from a Device to a Host.|
|H2D transmission|Refers to data transmission from a Host to a Device.|
|D2RH|Refers to data transmission from a Device to a remote Host.|
|RH2D|Refers to data transmission from a remote Host to a Device.|
|TTFT|Time To First Token, an indicator of LLM inference. It refers to the latency from the input to the output of the first token. That is, after a batch of users enters the inference system, the time it takes for the users to complete the Prefill stage. This is also the response time required for the system to generate the first character.|
|TBT|Time Between Tokens, the latency between tokens, which refers to the time required for each decoding. It reflects the interval at which each character is generated during the online processing of a large language model system, that is, how smooth the generation process is.|
|Continuous Batching|A continuous batching technique, which is a method for optimizing computing efficiency in large model inference. It combines multiple inference requests into a batch for processing, thereby making full use of computing resources and improving inference speed and throughput. The core idea of this technique is to reduce idle time during computation and maximize the utilization of computing resources.|
|Unilateral link establishment|The Client initiates link establishment to the Server.|
|Bilateral link establishment|All LLM-DataDist instances can initiate link establishment simultaneously.|

## Overview of the LLM Inference Process

The launch of ChatGPT marks the gradual commercialization of interactive AI as a mature product, and it has further advanced the research and progress of the underlying large language model (LLM) technology. The LLMs in mainstream use today include the GPT series, the LLAMA series, and the GLM series. Their underlying model architectures all adopt the Transformer architecture, primarily in a Decode-Only form. During inference and prediction, such Transformer-based Decode-Only LLMs use the auto-regressive generation (AR) mode. That is, the generation of each token requires a forward inference process through the LLM model, which involves completing N Transformer Layer computations. This means that fully generating a statement containing M tokens requires M complete forward inference processes through the LLM.

In the Transformer inference process, the KV Cache technique can reduce the computational load in the Decode stage, and it has now become an essential technique for LLM inference systems. The LLM inference process using KV Cache is generally divided into two stages: Prefill and Decode.

- Prefill stage: The user's requested prompt is fed into the large model for computation. The intermediate results are written into the KV Cache, and the first token is inferred. As the prompt sequence length grows linearly, services that have requirements on the time to first token (TTFT) typically execute LLM inference in single-batch mode. This stage is a compute-intensive operation.
- Decode stage: The first token of the request is fed into the large model, and the KV Cache generated from the preceding text is read from the video memory for computation. After the KV Cache technique is adopted, the computational load of a single token is low, and the main bottleneck lies in moving the parameter volume. Therefore, multi-batch mode is usually required to improve utilization. This stage is a memory-intensive operation.

![](figures/260109142344517.png)

## Reason for PD Disaggregation

From an implementation perspective, the entire LLM inference process consists of Prefill and multiple rounds of iterative Decode. The prerequisite for implementing continuous batching in the Decode phase is that each scheduled request must have idle computing power to complete the Prefill computation. Under the existing deployment mode, Prefill and Decode are deployed together. When a new Prefill request arrives, it is processed with priority, which means that the execution flow of Decode cannot be effectively guaranteed due to the time between tokens (TBT). For example, in the following figure, when request5 or request6 arrives, the system may prioritize the Prefill of request5 or request6. In this case, the response latency of request2/3/4 is affected to a certain extent, resulting in unstable TBT.

![](figures/260109142618323.png)

In actual deep learning model deployment, because the Prefill and Decode phases differ in their computation and communication characteristics, the PD separation deployment solution is adopted to deploy Prefill and Decode in clusters of different specifications and architectures, so as to improve performance and resource utilization. Combined with task scheduling at the service layer, this solution maximizes the batch concurrency of the Decode phase through the continuous batching mechanism while meeting the TTFT and TBT metrics, thereby improving computing power utilization.

Based on this solution, as shown in the following figure, the execution of Prefill and Decode does not affect each other, and the system can provide users with a stable TBT.

![](figures/260109142712834.png)
