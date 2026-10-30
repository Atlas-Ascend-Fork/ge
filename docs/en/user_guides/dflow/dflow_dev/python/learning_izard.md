# Learning Guide

<!-- md-trans-meta sourceCommit=b7f5e4a67c5c41d02ea0a83a8f907089d6390ff2 translatedAt=2026-10-28T12:54:16.692Z pushedAt=2026-10-29T03:25:50.683Z nl=2 -->

## Target Audience

This document guides developers on how to use DataFlow APIs to develop asynchronous pipeline applications, or to refactor existing applications into asynchronous pipelines, so as to achieve high throughput and high concurrency. By reading this document, you can achieve the following goals:

- Understand the basic concepts and application scenarios of DataFlow.
- Understand the basic process and implementation methods for developing applications using the DataFlow APIs.
- Extend the samples in this document to develop other applications.

Readers who are familiar with the basic architecture and features of CANN, are capable of developing programs in Python, and have some knowledge of machine learning and deep learning can better understand this document.

## Reading Guide

If you are not familiar with DataFlow or are reading this document for the first time, it is recommended that you start with the Reading Guide, which helps you quickly understand the manual content and access the corresponding chapters as needed.

**Table 1** Reading Guide

|Scenario|Corresponding Chapter|Content Description|
|--|--|--|
|I Want to Learn what DataFlow is and what it can do|[Overview](./introduction.md)|This Chapter Introduces the basic concepts, functions, and advantages of DataFlow, and lists common DataFlow usage scenarios.|
|I Want to Learn the process of developing asynchronous pipeline applications based on DataFlow|[Quick Start](./quick_start.md)|This Chapter Introduces the overall development process and environment preparation for DataFlow applications, and provides complete reference samples.|
|I Want to Learn how to build and run a FlowGraph|[DataFlow Running](./dataflow_running.md)|This Chapter Introduces how to construct a FlowGraph and how to execute a constructed FlowGraph.|
|I Want to Learn all the features of DataFlow|[Special Topics](./special_topics.md)|This Chapter Introduces all DataFlow-related features, including the UDF development process, specifying node deployment locations, compilation caching, offline compilation, data alignment, configuring dynamic shapes, multi-instance deployment, and streaming input and output.|
|I Want to Learn some extended knowledge involved in this document|[Appendices](./appendices.md)|This Chapter Introduces some related concepts and extended knowledge mentioned in this document.|
