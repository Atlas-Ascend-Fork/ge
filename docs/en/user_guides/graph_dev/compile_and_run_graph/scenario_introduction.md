# Scenario Introduction

<!-- md-trans-meta sourceCommit=bd3de52c8a808742e9c01b524792105f46cad44c translatedAt=2026-10-21T09:23:15.936Z pushedAt=2026-10-28T03:30:34.473Z nl=2 -->

This section describes several typical business scenarios for building and running a graph. You can select the required business based on the actual situation.

**Table 1** Building and running a graph

|Scenario|Description|
|--|--|
|[Building a Graph into an Offline Model](#building-a-graph-into-an-offline-model)|**Builds a graph and saves it as an om offline model.** The built offline model is loaded through the model loading API and then used for inference through the model execution API. For details, see the "Model Inference" section in [Application Development](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/app-dev/c/00_acl_cpp_dev.md).|
|[Building and Running a Graph](#building-and-running-a-graph)|After a graph is built, **directly build and run the graph**, and load the graph object using the AddGraph API. When running the graph, the following two cases apply depending on the API called:<br>- Run the graph synchronously: After loading, use RunGraph to run the graph synchronously and **obtain the execution result of the graph**.<br>- Run the graph asynchronously: After loading, use RunGraphWithStreamAsync to run the graph asynchronously and **obtain the execution result of the graph**.|

Note:

- Building a graph into an offline model: supported by all chips.

- Building and running a graph: product support is as follows:

    <!-- npu="950" id1 -->
    Ascend 950PR&950DT products: supported
    <!-- end id1 -->

    <!-- npu="A3" id2 -->
    Atlas A3 products: supported
    <!-- end id2 -->

    <!-- npu="910b" id3 -->
    Atlas A2 products: supported
    <!-- end id3 -->

    <!-- npu="310b" id4 -->
    Atlas 200I/500 A2 inference products: Not supported
    <!-- end id4 -->

    <!-- npu="310p" id5 -->
    Atlas inference products: Supported
    <!-- end id5 -->

    <!-- npu="910" id6 -->
    Atlas training products: Supported
    <!-- end id6 -->

    <!-- @ref: ge/res/docs/zh/user_guides/graph_dev/scenario_introduction_res.md#id1 -->

## Building a Graph into an Offline Model

In this scenario, the **graph is built and saved as an om offline model**. The following figure shows the specific process. For details about the service, see [Building a Graph into an Offline Model](#building-a-graph-into-an-offline-model).

**Figure 1** Model building process
![Illustration](../figures/model_build_flow.png "Model building process")

## Building and Running a Graph

The following figure shows the specific process of building a graph and building and running the graph. For details about the business, see [Building and Running a Graph](#building-and-running-a-graph).

**Figure 2** Building and running flow diagram
![Illustration](../figures/compile_and_run_flow_diagram.png "Building and running flow diagram")
