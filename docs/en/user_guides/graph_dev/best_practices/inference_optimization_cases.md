# Inference Tuning Cases

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-21T09:12:58.334Z pushedAt=2026-10-28T03:30:34.457Z nl=2 -->

## Case Introduction

This section targets the inference scenario of recommendation models. Through a complete code example and the Profiling data collected during the inference phase, it analyzes and locates the key bottlenecks that affect model throughput performance, and provides corresponding optimization strategies. The content covers the implementation of multi-instance parallelism, the AI Core control strategy, and batch H2D (Host-to-Device) transfer, and describes the impact of these techniques on inference throughput performance. Specifically:

- AI Core control: By configuring the **ge.aicoreNum** parameter in [Operator  and Graph Compilation](../../../api/graph_engine_api/cpp/ge/options_params/operator_and_graph_compilation.md), you can configure the number of AI Cores used during operator compilation.
- Batch H2D: By calling the aclrtMemcpyBatch API, you can implement batch memory copy.
- Multi-instance parallelism: By creating multiple inference instances through multiple threads, you can improve the concurrent processing capability of the system.

To quantitatively evaluate the impact of different features on inference throughput performance, this experiment executes 10,000 inference tasks (the input data of the sample model in this case is constructed random values), records the single inference duration and total latency, and calculates the model throughput (TPS) and average latency (ms).

The following table shows the throughput and latency of the Atlas A3 products under different configurations:

|Configuration Scheme|BatchSize=128|BatchSize=256|
|--|--|--|
|Single instance|745,55 TPS / 1.471ms|132,247 TPS / 1.685ms|
|Single instance + batch H2D|131,191 TPS / 0.792ms|209,927 TPS / 1.030ms|
|Multi-instance parallelism (4)|155,104 TPS / 2.089ms|360,253 TPS / 2.034ms|
|Multi-instance parallelism (4) + AI Core control (16\|16)|185,415 TPS / 1.797ms|384,163 TPS / 1.850ms|
|Multi-instance parallelism (4) + AI Core control (16\|16)+ batch H2D|251,877 TPS / 1.285ms|493,065 TPS / 1.317ms|

- Taking the single instance with BatchSize=128 as the benchmark data, the following can be observed from the table above:
  - **Batch H2D** can significantly reduce the data movement overhead. In the single-instance scenario, the throughput increases by 75.9% and the latency decreases by 52.9%.
  - **Multi-instance parallelism** increases the throughput to 2.08 times that of a single instance, but with a 41.7% increase in latency.

- Using multi-instance parallelism with BatchSize=128 as the benchmark data, the following can be observed from the table above:

    **AI  core control** effectively avoids resource contention during multi-instance parallelism, increasing the throughput by 19.5% and reducing the latency by 13.9%.

- Using multi-instance parallelism \(4\)+AI  core control \(16|16\)+ batch H2D with BatchSize=128 as the benchmark data, the following can be observed from the table above:

    **Increasing the BatchSize** significantly optimizes the scheduling density. In the 256 BatchSize scenario, the throughput increases by 95.7% compared with 128, with a latency increase of 7.9%.

<!-- npu="950,A3,910b" id1 -->
This feature is supported only on the following product models:

<!-- npu="950" id2 -->
Ascend 950PR & 950DT products
<!-- end id2 -->

<!-- npu="A3" id3 -->
Atlas A3 products
<!-- end id3 -->

<!-- npu="910b" id4 -->
Atlas A2 products
<!-- end id4 -->
<!-- end id1 -->

## Analyzing Bottlenecks

The benchmark case configuration is shown in the following table. The benchmark case uses a single instance, does not apply the AI core control strategy, and does not enable the batch H2D transfer technology. Refer to [Profiling Data Collection](../more_features/profiling_data_collection.md) to enable the Profiling performance data collection function.

**Table 1** Benchmark case configuration

|Inference Count|BatchSize|Multi-instance Parallelism|AI Core Control|Batch H2D|
|--|--|--|--|--|
|10000|128|1|Do not control the number of available cores|Not enabled|

- **Data movement bottleneck**

    Based on Profiling data analysis, multiple (28) independent H2D data movement operations must be performed before a single inference, causing the NPU to remain idle during the data preparation phase. The overall data movement time accounts for a significant proportion (53.6%, 0.740230/1.383355). The data analysis is shown in the following figure.

    **Figure 1** Profiling data analysis of a single inference
    ![Figure 1 Illustration](../figures/single_inference_profiling_analysis.png "Profiling data analysis of a single inference")

- **Insufficient kernel granularity, with gaps in operator scheduling**

    The operator execution time is very short (1 us to 5 us), and dependencies exist between operators, causing the NPU to generate fine-grained bubbles during operator execution gaps (Bubbles, Free idle count 539998). The resource utilization is insufficient (10%, computation time/total time = 1826.548713/17889.715647). For detailed data, see [Figure 3](#fig1).

    **Figure 2** Operator execution time
    ![Figure 2 example](../figures/op_execution_time.png "Operator execution time")

    **Figure 3**  Bubbles and resource utilization<a id="fig1"></a>
    ![Figure 3 example](../figures/bubble_resource_utilization.png "Bubbles and resource utilization")

## Designing the Optimization Scheme

To address the preceding issues, the following optimization strategies are available:

- **Multi-instance parallelism**

    Interleave inference tasks through multithreading to fill the bubbles between operators and improve NPU utilization. However, parallel inference may cause resource contention and increase inference latency. Therefore, the degree of parallelism should be properly controlled to avoid excessive resource contention.

- **Batch H2D optimization**

    Enable the batch data movement mechanism to combine multiple input data into a single H2D operation, reducing the number of data movement operations and increasing the amount moved per operation.

- **AI Core resource control**

    Used together with multiple instances, configure the ge.aicoreNum parameter to limit the number of AI Cores available to a single operator (for example, "8|8"), balancing resource allocation and improving parallelism while avoiding excessive contention.

- **Increasing BatchSize**

    Increase the BatchSize of a single inference to improve operator granularity and scheduling density, and reduce the proportion of scheduling overhead.

The following presents the inference effect verification for the combined scenario of batch H2D, multi-instance parallelism, and AI core control.

## Verifying the Optimization Scheme

The configuration of this scenario is shown in the following table. It uses multi-instance parallelism, configures the AI core control strategy, and enables the batch H2D transfer technology. For details about how to enable the Profiling performance data collection function, see [Profiling Data Collection](../more_features/profiling_data_collection.md).

**Table 2**  Batch H2D - multi-instance - core control case configuration

|Inference Count|BatchSize|Multi-instance Parallelism|AI core Control|Batch H2D|
|--|--|--|--|--|
|10000|5096|6|8\|8|true|

The verification results are as follows:

- **Data Movement**

    The batch data movement mechanism is enabled to combine multiple input data into a single H2D operation. The number of data movement operations per single inference is significantly reduced (28 --\>2), and the proportion of time consumed by data movement is reduced (53.6% --\>10.4%, 0.398185/3.820395). For detailed data, see the following figure.

    **Figure 4**  Profiling data analysis for batch data transfer
    ![Figure 4 example](../figures/batch_data_transfer_profiling_analysis.png "Profiling data analysis for batch data transfer")

- **Insufficient kernel granularity and operator scheduling gaps**

    Through reasonable parallelism and core control, operator gaps can be eliminated. By increasing BatchSize to improve operator granularity, the scheduling density is increased and the scheduling overhead proportion is reduced. The number of bubbles is reduced by 81.3% (compared with the Free idle count 539998 in [Figure 3](#fig1)), and the resource utilization is increased to 72% (computation time/total time = 4390.608297/6086.974405). For detailed data, see [Figure 6](#fig2).

    **Figure 5**  Improving operator granularity by increasing the batch size
    ![Figure 5 example](../figures/increase_batchsize_improve_op_granularity.png "Improving operator granularity by increasing the batch size")

    **Figure 6**  Bubbles and resource utilization<a id="fig2"></a>
    ![Figure 6 example](../figures/bubble_resource_utilization_13.png "Bubbles and resource utilization-13")
