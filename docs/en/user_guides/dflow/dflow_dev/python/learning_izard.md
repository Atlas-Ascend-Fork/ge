# Learning Guide

<!-- md-trans-meta sourceCommit=b7f5e4a67c5c41d02ea0a83a8f907089d6390ff2 translatedAt=2026-11-16T08:56:31.564Z pushedAt=2026-11-17T03:39:11.995Z nl=2 -->

## Audience

This document guides developers on how to use DataFlow APIs to develop asynchronous pipeline applications, or to refactor existing applications into asynchronous pipelines, so as to achieve high throughput and high concurrency. By reading this document, you can achieve the following goals:

- Understand the basic concepts and scenarios of DataFlow.
- Understand the basic process and implementation methods of application development through the DataFlow API.
- Be able to extend the samples in this document to develop other applications.

This document is intended for readers who are familiar with the basic architecture and features of CANN software, are capable of developing programs in Python, and have a certain understanding of machine learning and deep learning.

## Reading Guide

If you are not familiar with DataFlow, or if you are reading this document for the first time, it is recommended that you start with the reading guide, which helps you quickly understand the content of this manual and access the corresponding chapters as needed.

**Table 1** Reading Guide

|Scenario|Corresponding Chapter|Content Description|
|--|--|--|
|I want to understand what DataFlow is and what it can do|[Overview](./introduction.md)|This chapter describes the basic concepts, functions, and advantages of DataFlow, and lists common DataFlow use cases.|
|I want to experience the process of developing asynchronous pipeline applications based on DataFlow|[Quick Start](./quick_start.md)|This chapter describes the overall development process and environment preparation for DataFlow applications, and provides complete reference samples.|
|I want to understand how to build and run a FlowGraph|[DataFlow Running](./dataflow_running.md)|This chapter describes how to construct a FlowGraph and how to execute a constructed FlowGraph.|
|I want to understand all the features of DataFlow|[Special Topics](./special_topics.md)|This chapter describes all DataFlow-related features, including the UDF development process, specifying node deployment locations, compilation caching, offline compilation, data alignment, dynamic shape configuration, multi-instance deployment, and streaming input and output.|
|I Want to Learn some extended knowledge involved in this document|[Appendix](./appendices.md)|This chapter describes some related concepts and extended knowledge mentioned in this document.|
