# Learning Guide

<!-- md-trans-meta sourceCommit=b7f5e4a67c5c41d02ea0a83a8f907089d6390ff2 translatedAt=2026-10-29T03:46:55.127Z pushedAt=2026-10-29T07:28:45.372Z nl=2 -->

## Audience

This document guides developers on how to use the Python APIs of LLM-DataDist to implement point-to-point data transfer and build a large model inference decoupled framework, thereby improving the throughput performance of large model inference systems.

Readers are expected to have the following knowledge, which will help them better understand this document:

- Understand the basic structure and concepts of large models
- Be familiar with the large model inference process
- Have the ability to develop programs in Python
- Have experience using large model inference frameworks, such as vLLM, for large model inference

## Reading Guide

If you are not familiar with LLM-DataDist or are reading this document for the first time, you are advised to read Table 1 first to quickly understand the content of this manual and access the corresponding sections as needed.

**Table 1** Reading Guide

|Scenario|Corresponding Chapter|Content Description|
|--|--|--|
|I want to learn what LLM-DataDist is and what it can do|[Overview](./introduction.md)|This chapter introduces the meaning, functions, and advantages of LLM-DataDist, as well as its usage scenarios.|
|I want to experience the development process of building a large model inference decoupled framework based on LLM-DataDist|[Quick Start](./quick_start.md)|This chapter introduces the overall development process and environment preparation for building a large model inference decoupled framework using LLM-DataDist, and provides a complete runnable reference sample.|
|I want to learn about the API capabilities of LLM-DataDist|[Feature Overview](./functions.md)|This chapter introduces the API functions of LLM-DataDist and provides usage examples.|
|I want to learn about more usage scenarios of LLM-DataDist|[Special Topics](./special_topics.md)|This chapter introduces how to build advanced features of a large model inference decoupled framework based on LLM-DataDist, including KV Cache swap-in/swap-out and common prefixes.|
|I Want to Learn some extended knowledge involved in this document|[Appendix](./appendices.md)|This chapter introduces some extended knowledge mentioned in this document, including basic concepts and the large model inference process.|
|What should I do when encountering problems during application development with LLM-DataDist|[Error Codes](../../llm_datadist_api/python/LLMStatusCode.md)|This chapter introduces the API error codes, as well as cause analysis and handling measures.|
