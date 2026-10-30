# DataFlow Overview

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-10-28T12:59:58.649Z pushedAt=2026-10-29T03:25:50.698Z nl=2 -->

## Audience

This document describes how to use the DataFlow APIs to build, modify, compile, and execute a compute graph. After reading this document, you will be able to achieve the following goals:

- Understand how to build a FlowGraph using FlowOperator.
- Master how to compile and run a FlowGraph.
- Understand the UDF development process and related APIs.

This document is intended for readers who are familiar with the basic architecture and features of CANN software, are capable of developing programs in C++/C, and have a certain understanding of machine learning and deep learning.

## DataFlow Overview

DataFlow is used to describe how one or more process points (ProcessPoint) are organized into a complete compute flow in a data-driven manner through data queues. The key difference between a ProcessPoint and an operator is that the ProcessPoint processes data asynchronously. DataFlow is carried by FlowGraph, and ProcessPoint is carried by FlowNode. The relationships between the APIs are as follows.

![](figures/relationships_between_datafow_api.png)

- FlowGraph: A DataFlow graph, which consists of input nodes (FlowData) and compute nodes (FlowNode).
- FlowOperator: The base class of FlowGraph nodes. Its derived classes include FlowData and FlowNode.
- FlowData: The input node of FlowGraph.
- FlowNode: The compute node of FlowGraph. It supports the following two types.
  - FunctionPp: A function process point that implements user-defined functions through UDF (User Defined Function).
  - GraphPp: A graph process point that implements user compute logic through IR graph construction.

DataFlow allows users to write custom processing functions through FunctionPp and GraphPp, and run them as a FlowModel through DataFlow graph construction.

The differences between DataFlow and IR graph construction are as follows.

**Table 1** Comparison between DataFlow and IR graph construction

|dimension|IR Graph Construction|DataFlow|
|--|--|--|
|Data flow processing|- The graph supports only one input correspondence to one output.<br>- Synchronous data flow is used between operators in the graph to express serial and synchronous execution.|- DataFlow supports one input correspondence to multiple outputs, one input correspondence to one output, or multiple inputs correspondence to one output, offering higher flexibility.<br>- ProcessPoint in DataFlow uses asynchronous data flow, which can express parallel and asynchronous execution, fully utilizing resources and improving throughput.|
|Custom function development|Implemented by developing custom operators.<br>Operator development involves processes such as operator prototype definition, operator code implementation, operator information library definition, and operator adaptation. It requires many deliverables from users and has a relatively high usage threshold.|Can be implemented by developing UDFs or custom operators.<br>UDF development only requires defining user functions and building the graph. It requires few deliverables from users and has a low usage threshold.|
|Memory allocation|The input and output memory of operators is pre-allocated.|The output memory of UDFs is user-defined and must be allocated by users themselves.|

## Overall Development Process

![](figures/260109145127059.png)

## Constraints

The model cannot modify the input data on either the host side or the device side.
