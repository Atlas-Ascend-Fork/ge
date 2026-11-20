# Learning Guide

<!-- md-trans-meta sourceCommit=b7f5e4a67c5c41d02ea0a83a8f907089d6390ff2 translatedAt=2026-11-17T04:14:04.072Z pushedAt=2026-11-18T01:47:39.977Z nl=2 -->

## Audience

This document guides developers on how to use the Python API of LLM-DataDist to implement point-to-point data transfer and build a disaggregated large language model inference framework, thereby improving the throughput performance of the large language model inference system.

Readers of this document are expected to have the following knowledge, which helps them better understand the document:

- Understand the basic structure and concepts of large language models
- Be familiar with the process of large language model inference
- Have the capability to develop programs in Python
- Have experience in using a large language model inference framework to perform large language model inference, such as vLLM

## Reading Guide

If readers are not familiar with LLM-DataDist or are reading this document for the first time, it is recommended that they first read Table 1 to quickly understand the content of the manual and access the corresponding chapters as needed.

**Table 1** Reading Guide

|Scenario|Corresponding Chapter|Content Description|
|--|--|--|
|I want to learn what LLM-DataDist is and what it can do|[Overview](./introduction.md)|This chapter describes the meaning, functions, and advantages of LLM-DataDist, as well as its usage scenarios.|
|I want to experience the development process of building a disaggregated large language model inference framework based on LLM-DataDist|[Quick Start](./quick_start.md)|This chapter describes the overall development process and environment preparation for building a disaggregated large language model inference framework using LLM-DataDist, and provides a complete runnable reference example.|
|I want to learn about the API capabilities of LLM-DataDist|[Feature Overview](./functions.md)|This chapter describes the API functions of LLM-DataDist and provides usage examples.|
|I want to learn about more usage scenarios of LLM-DataDist|[Special Topics](./special_topics.md)|This chapter describes how to build advanced features of a disaggregated large language model inference framework based on LLM-DataDist, including KV Cache swap-in/swap-out and common prefixes.|
|I Want to Learn some extended knowledge involved in this document|[Appendix](./appendices.md)|This chapter describes some extended knowledge mentioned in the document, including basic concepts and the large language model inference process.|
|What should I do when encountering problems during application development with LLM-DataDist|[Error Codes](../../llm_datadist_api/python/LLMStatusCode.md)|This chapter describes the API error codes, as well as cause analysis and handling measures.|
