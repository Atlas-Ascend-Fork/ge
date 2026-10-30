# Learning Guide

<!-- md-trans-meta sourceCommit=b7f5e4a67c5c41d02ea0a83a8f907089d6390ff2 translatedAt=2026-10-29T03:41:22.282Z pushedAt=2026-10-29T07:28:29.342Z nl=2 -->

## Audience

This document guides developers on how to use the C++ APIs of LLM-DataDist to implement data transfer between clusters and build a separated framework for large model inference, thereby improving the throughput performance of large model inference systems.

Readers of this document are expected to have the following knowledge, which will help them better understand the document:

- Understand the basic structure and concepts of large models
- Be familiar with the large model inference process
- Have the ability to develop programs in C++
- Have experience in large model inference using a large model inference framework

## Reading Guide

If you are not familiar with LLM-DataDist or are reading this document for the first time, it is recommended that you read Table 1 first to quickly understand the content of this manual and access the corresponding chapters as needed.

**Table 1** Reading Guide

|Scenario|Corresponding Chapter|Content Description|
|--|--|--|
|I want to learn what LLM-DataDist is and what it can do|[Overview](./introduction.md)|This chapter introduces the basic concepts, functions, and advantages of LLM-DataDist, as well as its usage scenarios.|
|I want to experience the development process of building a separated framework for large model inference based on LLM-DataDist|[Quick Start](./quick_start.md)|This chapter introduces the overall development process and environment preparation for building a separated framework for large model inference using LLM-DataDist, and provides a simple reference example.|
|I want to learn about the API capabilities of LLM-DataDist|[Feature Overview](./functions.md)|This chapter introduces the API functions of LLM-DataDist and provides their implementations.|
|I want to learn about more features of LLM-DataDist|[Special Topics](./special_topics.md)|This chapter introduces the advanced features of building a separated framework for large model inference based on LLM-DataDist, including role switching and common prefixes.|
|I Want to Learn some extended knowledge involved in this document|[Appendix](./appendices.md)|This chapter introduces some extended knowledge mentioned in this document, including basic concepts and large model inference.|
|What should I do if I encounter problems when developing applications with LLM-DataDist|[Error Codes](../../llm_datadist_api/cpp/error-code.md)|This chapter introduces the API error codes, as well as cause analysis and handling measures.|
