# DUMP\_GRAPH\_LEVEL

<!-- md-trans-meta sourceCommit=cc5ba9391389e272005d4ade3dda4ccce784361d translatedAt=2026-10-20T12:46:38.525Z pushedAt=2026-10-28T11:30:05.986Z nl=2 -->

## Feature Description

Prints the graph description information of each stage in the entire graph compilation process to a file.

This environment variable supports the following two configuration methods. Both methods control the number of graphs to be dumped, and you can use either one as needed. Note that the two configuration methods cannot be used together:

- Configure a numeric value. The values are as follows:
  - 1: Dump graphs of all stages.
  - 2: Dump graphs of trustlisted stages. For details about the trustlisted graphs, see the "Whitelisted" column in [Table 1](../atc_tools/references/dump_graph_details.md).
  - 3: Dump the final generated graph, that is, the graph optimized and compiled by GE (Graph Engine).
  - 4: Dump the earliest generated graph, that is, the compilation entry graph provided to the software stack after GE parses and maps operators. At this point, the graph structure has not yet undergone GE compilation optimization.

- Configure a string separated by "|". The configuration is as follows:

    For example, if the value is set to "aa|bb", graphs whose names contain aa and bb are dumped. aa and bb must be valid strings in the graph compilation process. Valid strings can be obtained from the full dump graphs.

The DUMP\_GRAPH\_LEVEL environment variable takes effect only when [DUMP\_GE\_GRAPH](DUMP_GE_GRAPH.md) is enabled, and defaults to 2.

## Configuration Example

- Configured as a numeric value:

    ```bash
    export DUMP_GRAPH_LEVEL=1
    ```

- Configured as a string separated by |:

    ```bash
    export DUMP_GRAPH_LEVEL="PreRunBegin|AfterInfershape"
    ```

## Constraints

- If this environment variable is set to other invalid values, undefined behavior may occur.
- If the operator dump data collection feature is enabled, refer to the [ge.exec.enableDump](../../api/graph_engine_api/cpp/ge/options_params/precision_comparison.md#geexecenabledump) parameter. Even if the DUMP\_GRAPH\_LEVEL environment variable is not configured, or if `export DUMP\_GRAPH\_LEVEL="PreRunBegin|AfterInfershape"` is configured but does not include the "Build" string, the subgraph ge\_proto\_xxxx\_Build.txt will eventually be dumped.
- This environment variable must be used together with [DUMP\_GE\_GRAPH](DUMP_GE_GRAPH.md). That is, when [DUMP\_GE\_GRAPH](DUMP_GE_GRAPH.md) is enabled, the generated dump graph information can be controlled through DUMP\_GRAPH\_LEVEL. For details, see [Dump Graph Details](../atc_tools/references/dump_graph_details.md).

## Applicable Products

Supported on all chips
