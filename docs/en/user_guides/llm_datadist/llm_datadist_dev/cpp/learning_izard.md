# Learning Guide

<!-- md-trans-meta sourceCommit=b7f5e4a67c5c41d02ea0a83a8f907089d6390ff2 translatedAt=2026-11-17T04:07:30.790Z pushedAt=2026-11-17T12:26:50.717Z nl=2 -->

## Audience

This document guides developers on how to use the C++ API of LLM-DataDist to implement data transfer between clusters and build a separable framework for large model inference, thereby improving the throughput performance of large model inference systems.

Readers are expected to have the following knowledge, which helps them better understand this document:

- Understand the basic structure and concepts of large models
- Be familiar with the large model inference process
- Have the capability to develop programs in C++
- Have experience in using a large model inference framework to perform large model inference

## Reading Guide

If readers are not familiar with LLM-DataDist or are reading this document for the first time, it is recommended that they first read Table 1 to quickly understand the content of this manual and access the corresponding sections as needed.

**Table 1** Reading Guide

|Scenario|Corresponding Chapter|Content Description|
|--|--|--|
|I want to understand what LLM-DataDist is and what it can do|[Overview](./introduction.md)|This chapter describes the basic concepts, functions, and advantages of LLM-DataDist, as well as its usage scenarios.|
|I want to experience the development process of building a separable framework for large model inference based on LLM-DataDist|[Quick Start](./quick_start.md)|This chapter describes the overall development process and environment preparation for building a separable framework for large model inference using LLM-DataDist, and provides a simple reference example.|
|I want to understand the API capabilities of LLM-DataDist|[Feature Overview](./functions.md)|This chapter describes the API functions of LLM-DataDist and provides their implementations.|
|I want to learn more features of LLM-DataDist|[Special Topics](./special_topics.md)|This chapter describes how to build advanced features of a separable framework for large model inference based on LLM-DataDist, including role switching and common prefixes.|
|I Want to Learn some extended knowledge involved in this document|[Appendix](./appendices.md)|This chapter describes some extended knowledge mentioned in this document, including basic concepts and large model inference.|
|What should I do when encountering problems during application development with LLM-DataDist|[Error Codes](../../llm_datadist_api/cpp/error-code.md)|This chapter describes the API error codes, as well as cause analysis and handling measures.|
