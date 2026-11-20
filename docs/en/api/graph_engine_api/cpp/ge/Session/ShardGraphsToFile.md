# ShardGraphsToFile

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T13:26:09.421Z pushedAt=2026-10-28T07:42:19.835Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

**This API is deprecated. Do not use it.**

**Used in distributed compilation sharding scenarios for large models**: When the graph is too large, call this API to shard the graph and save the sharded graphs as pb files.

This graph sharding API shards the graphs in the Session in the order in which they are added by AddGraph, and **saves the sharded graphs as pb files**.

The sharding method is configured by the ge.graphParallelOptionPath parameter in [options parameter description](../options_params/options_parameters_description.md). If parallel sharding is not enabled, this API does not perform sharding.

Naming rules for sharded graphs:

- In SPMD mode, the naming rule for sharded graphs is: \{original graph name\}\_\{ClusterId\}\_\{ItemId\}\_\{ChipId\}\_\{VirtualStageId\}\_\{original graph GraphId\}, and the number of sharded graphs equals the number of original graphs \* the number of shards.
- In non-SPMD mode, the naming rule for sharded graphs is: \{original graph name\}\_\{original graph GraphId\}, and the number of sharded graphs equals the number of original graphs.

After the graphs are sharded, the original graphs no longer exist in the Session, and the GraphId of the newly generated sharded graphs uses other IDs to replace the GraphId of the original graphs.

- **Differences between this API and [SaveGraphsToPb](SaveGraphsToPb.md):**

    ShardGraphsToFile applies to distributed compilation sharding scenarios for large models, whereas SaveGraphsToPb applies to any graph.

- **Differences between this API and [ShardGraphs](ShardGraphs.md):**
  - ShardGraphsToFile can complete strategy search and graph sharding, and output files such as the sharded graphs. **The sharded graphs are also persisted to disk through this API** (the file\_path parameter must specify a valid path).
  - ShardGraphs completes strategy search and graph sharding, and outputs files such as the sharded graphs. **The sharded graphs are kept in memory and then persisted to disk through [SaveGraphsToPb](SaveGraphsToPb.md).**

## Prototype

```c++
Status ShardGraphsToFile(const char_t *file_path = "./") const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| file_path | Input | Directory for saving graphs and weights, which must be a valid path.<br>If this parameter is empty, only graph sharding is performed and no pb file is generated. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Success.<br>**FAILED**: Failure. |

## Constraints

This takes effect only when parallel sharding mode is enabled.

## Examples

```c++
Session session(options); // Parallel sharding is enabled in options.
Graph init_graph("init_graph");
Graph first_graph("first_graph");
Graph second_graph("second_graph");
session.ShardGraphsToFile("/xxx/graph_path");
```
