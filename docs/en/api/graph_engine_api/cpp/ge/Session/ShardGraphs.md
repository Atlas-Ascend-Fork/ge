# ShardGraphs

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T13:25:15.998Z pushedAt=2026-10-28T07:42:19.832Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

**This API is deprecated. Do not use it.**

**This API is not recommended. Use the [ShardGraphsToFile](ShardGraphsToFile.md) API instead, which delivers better performance than ShardGraphs.**

Graph sharding API. It shards the graphs in a Session in the order they were added by AddGraph. **The split graphs are stored in memory.**

The sharding method is configured by the ge.graphParallelOptionPath parameter in [options parameter description](../options_params/options_parameters_description.md). If parallel sharding is not enabled, this API does not perform sharding.

Naming rules for the split graphs:

- In SPMD mode, the split graphs are named as follows: \{original graph name\}\_\{ClusterId\}\_\{ItemId\}\_\{ChipId\}\_\{VirtualStageId\}\_\{original graph GraphId\}. The number of split graphs equals the number of original graphs \* the number of shards.
- In non-SPMD mode, the split graphs are named as follows: \{original graph name\}\_\{original graph GraphId\}. The number of split graphs equals the number of original graphs.

After sharding, the original graph no longer exists in the Session. The GraphId of the newly generated split graphs uses other IDs to replace the GraphId of the original graph.

**Differences between this API and [ShardGraphsToFile](ShardGraphsToFile.md)**:

- ShardGraphs can complete search strategy and graph sharding, and output files such as the split graphs. **The split graphs are stored in memory and then written to disk through [SaveGraphsToPb](SaveGraphsToPb.md)**.
- ShardGraphsToFile can complete search strategy and graph sharding, and output files such as the split graphs. **The split graphs are also written to disk through this API** (the file\_path parameter must specify a valid path).

## Prototype

```c++
Status Session::ShardGraphs()
```

## Parameters

None

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Success<br>**FAILED**: Failure |

## Constraints

This takes effect only when parallel sharding mode is enabled.

## Examples

```c++
Session session(options); // Parallel sharding is enabled in options.
Graph init_graph("init_graph");
Graph first_graph("first_graph");
Graph second_graph("second_graph");
session.ShardGraphs();
```
