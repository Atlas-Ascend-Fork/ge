# Overview

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T04:06:07.497Z pushedAt=2026-11-18T01:52:00.477Z nl=2 -->

As a distributed cluster and data management component for large models, LLM-DataDist provides high-performance, zero-copy point-to-point KV transfer capabilities, which are exposed to users through simple APIs.
This document provides a development guide for developers on how to use the LLM-DataDist APIs to implement data transfer between clusters and build a disaggregated large model inference framework.

This document supports the following products:

- Atlas A3 products
- Atlas A2 inference products

**Table 1** Usage scenarios

|Manual|Overview|
|--|--|
|LLM DataDist Development (C++)|Introduces how to perform link management and KV Cache management through the LLM-DataDist APIs in C++. In this scenario, one-way link establishment is supported, that is, the Client initiates link establishment to the Server. Data transfer is restricted to pulling KV from Decode to Prompt and pushing KV from Prompt to Decode. In this scenario, only D2D transfer is supported.|
|LLM DataDist Development (Python)|Introduces how to perform link management and KV Cache management in KvCacheManager mode through the LLM-DataDist APIs in Python. In this scenario, one-way link establishment is supported. Data transfer is restricted to pulling KV from Decode to Prompt and pushing KV from Prompt to Decode. In this scenario, only D2D transfer is supported.|
