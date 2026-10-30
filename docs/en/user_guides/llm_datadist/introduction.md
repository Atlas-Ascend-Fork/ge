# Overview

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-29T03:50:46.373Z pushedAt=2026-10-29T07:31:12.060Z nl=2 -->

As a distributed cluster and data management component for large language models, LLM-DataDist provides high-performance, zero-copy point-to-point KV transmission capabilities, which are exposed to users through simple APIs. This document provides a development guide to help developers use LLM-DataDist APIs to implement data transmission between clusters and build a decoupled framework for large model inference.

The products supported by this document are as follows:

- Atlas A3 products
- Atlas A2 inference series products

**Table 1** Scenarios

|Manual Name|Overview|
|--|--|
|LLM DataDist Development (C++)|Describes how to perform link management and KV Cache management through the LLM-DataDist APIs in C++. In this scenario, one-way link establishment is supported, that is, the Client initiates link establishment to the Server. Data transmission is restricted to pulling KV from Decode to Prompt and pushing KV from Prompt to Decode. In this scenario, only D2D transmission is supported.|
|LLM DataDist Development (Python)|Describes how to perform link management and KV Cache management in KvCacheManager mode through the LLM-DataDist APIs in Python. In this scenario, one-way link establishment is supported. Data transmission is restricted to pulling KV from Decode to Prompt and pushing KV from Prompt to Decode. In this scenario, only D2D transmission is supported.|
