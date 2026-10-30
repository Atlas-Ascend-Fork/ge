# Auto Tiling

<!-- md-trans-meta sourceCommit=1c0a36284515311df07ca620682d500844a67127 translatedAt=2026-10-21T09:00:40.853Z pushedAt=2026-10-28T03:30:34.441Z nl=2 -->

In the schedule phase, the system generates multiple optimized tiling graphs based on the AscIR graph input on the frontend. The Auto Tiling module is required to evaluate the optimal tiling solution and the corresponding kernel performance of these graphs to select the optimal template.

Tiling strategies determine the number of cores required for kernel execution, the amount of data moved from the global memory (GM) to the unified buffer (UB) each time, the number of cycles in the UB, and the amount of data moved from the UB to the GM each time. All these factors affect the execution performance of the kernel. Auto Tiling aims to find a tiling strategy that helps achieve the optimal kernel execution performance.

## Core Process

**Figure 1**  Core process of Auto Tiling
![Figure 1](../figures/auto_tiling_core_flow.png "Core process of Auto Tiling")

The execution logic of the kernel is expressed in AscIR. Auto Tiling extracts key information based on the expression of the AscIR graph, including:

1. LocalBuffer usage modeling

    Auto Tiling must ensure that the local buffer usage of each level is within the range allowed by the hardware. For example, the sum of the sizes of the TQue/TBuf requested by the kernel and temporary buffer cannot exceed the UB size limit of the hardware. AscIR expresses the location of each tensor on the GM/UB and the reuse relationships between tensors. Based on the information, Auto Tiling symbolizes the constraints of the local buffer at each level.

    For details about TQue and TBuf, see [Ascend C Operator Development](https://gitcode.com/cann/asc-devkit/blob/9.2.0/docs/zh/guide/index.md).

2. Time consumption formula modeling

    Auto Tiling conducts performance modeling for each API and uses symbols to express the performance of these APIs in each pipeline. The number of API calls is determined based on the loop axis expressed by AscIR. In this way, the total time consumed by all APIs of AscIR in each pipeline is derived. Assume that the execution of the bottleneck pipeline can well hide the execution of non-bottleneck pipelines. Therefore, the total time consumed for task execution is mainly reflected in the time of the bottleneck pipeline, as shown in the following figure.

    The following figure shows three pipelines. The bottleneck pipeline is AIV\_MTE2. When AIV\_MTE2 is executed, the execution of AIV\_MTE3 and AIV\_VEC is hidden. The total time consumed by the task is reflected by the time consumed by AIV\_MTE2.

    **Figure 2**  Pipeline diagram
    ![Figure 2](../figures/pipe_pipeline_diagram.png "Pipeline diagram")

3. Tiling Solver

    Auto Tiling aims to minimize the time consumption of bottleneck execution units and solve the problem by optimizing the model through modeling data. The prerequisite is that the storage usage for operator implementation does not exceed the physical storage size of each NPU level. For example, a heuristic solution method is used to search for a solution space that meets the memory requirements in the feasible domain meeting the constraints. Starting from the initial solution, the method searches for the solution space based on the gradient descent direction of the performance formula modeling, until the value of the formula modeling reaches the minimum value. In this way, the optimization process is exited and the optimal tiling result in returned.

## Key Technologies

- Performance formula modeling
  - Objective: The simulation accuracy of the performance formula determines the accuracy of template selection and heuristic solution. Therefore, accurate performance modeling is required for APIs.
  - Implementation:
    - For stable basic AscendC APIs, the Auto Tiling module collects performance data based on different inputs, obtains the relationship between performance and inputs, establishes a performance model, and expresses the performance model using the symbolic technology.
    - For volatile AscendC APIs, the Auto Tiling module computes the input parameters and number of calls based on the API calling logic, and generates a complete performance model for the APIs to obtain relatively accurate performance modeling.

- Axis sorting solver
  - Objective: A performance-formula-driven gradient descent solver relies heavily on the accuracy of its performance model. In addition, the tiling solution time is uncertain, and it is likely that the host bound problem may occur. As the APIs have a specific reference for axis tiling, and axis sorting solver is designed as an alternative solution for iteration solving.
  - Implementation
    1. Determine the priority of the tiling axis based on the API. The sequence is as follows:
        1. The priority of the parent axis is higher than that of the child axis.
        2. The priority of the reduction axis is higher than that of the non-reduction axis.
        3. The priority of the broadcast axis is higher than that of the non-broadcast axis.
        4. The priority of the non-innermost axis is higher than that of the innermost axis.

    2. Then, divide the tiling operation into two parts.
        - Intra-core tiling: The variables are traversed in the reverse order of the axis sorting. The variables are preferentially adjusted to the maximum values to determine whether the hardware constraints are met. If not, the variables are adjusted using the dichotomy until these constraints are met. Then, the next intra-core tiling variable is adjusted until all variables meet the hardware constraints. For example, s1tt2, s1tt, s1t, and s2t are tiling-related axes. The tiling process is as follows:

            s2t is traversed first and adjusted to its maximum value 1024, which meets the hardware constraints. Then, s1t is traversed and adjusted to its maximum value 256, which meets the hardware constraints. Then, the next variable s1tt\>s1tt2 is adjusted  until all variables meet the hardware constraints.

            **Figure 3** Intra-core tiling
            ![Figure 3](../figures/intra_core_tiling_diagram.png "Intra-core tiling")

        - Multi-core tiling: Variables related to multiple cores are identified and traversed in descending order. The record with a large number of occupied cores is found. If the found number exceeds the number of physical cores (for example, the number of NPU cores is 48 for Atlas A2 training products/Atlas A2 inference products), this result is returned.

            The following figure shows the tiling process, in which bngs1T is the axis for multi-core tiling. If the number of occupied cores is different, the record with a larger number of occupied cores is preferentially selected. According to the preceding policy, the number of occupied cores is 47.

            **Figure 4** Multi-core tiling
            ![Figure 4](../figures/multi_core_tiling_diagram.png "Multi-core tiling")
