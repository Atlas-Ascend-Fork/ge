# SuperKernel Fusion Scope Marking

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-21T10:08:54.033Z pushedAt=2026-10-28T11:21:43.586Z nl=2 -->

## Overview

### Feature Overview

SuperKernel is a binary-level operator fusion technology. Unlike traditional source-level fusion, its core lies in deep optimization of compiled binary kernel functions (Kernels). Through intelligent scheduling and combination, multiple independent kernel functions (sub-kernels) are integrated into an efficient "SuperKernel", which uniformly invokes multiple other kernel functions in the form of sub-function calls. Compared with dispatching individual operators, SuperKernel technology significantly reduces the waiting time and scheduling overhead of task dispatching, and can effectively utilize the idle resources between subtasks, further optimizing the execution efficiency and overhead of the overall operators.

To improve the flexibility of SuperKernel fusion, a SuperKernel calibration mechanism based on Scope tags is introduced: users can manually specify the operators to be fused into the same SuperKernel, and can also manually tag operators that do not participate in fusion. The "no fusion" tag has the highest priority.

<!-- npu="A3" id1 -->
**This feature is currently supported only on Atlas A3 products.**
<!-- end id1 -->

### Implementation Principle

When manual calibration by the user is detected, the system performs SuperKernel fusion according to the user-calibrated fusion scope. In terms of the SuperKernel generation strategy, the system groups operators based on their tag sets. Operators with exactly the same tag set are grouped together and enter the subsequent SuperKernel fusion process. The workflow is as follows:

![](figures/network_topo_example.png)

- Record calibration information

    The system traverses all operators and records the "tag set" calibrated by the user for each operator (that is, which operators belong to the same calibration region).

- Group by tag set

    All operators are grouped according to the principle that their "tag sets" are exactly identical. Only operators with exactly the same tag set are placed in the same group.

- Trigger the fusion process

    Operators in each group enter the SuperKernel fusion logic (such as sk1 and sk2 in the following figures).

The following two figures show examples of calibrating the SuperKernel fusion scope:

- Calibrate the SuperKernel fusion scope

    **Figure 1**  Diagram of marking the SuperKernel fusion scope
    ![](figures/fusion_scope_diagram.png "Diagram of marking the SuperKernel fusion scope")

  - Use "sk1" to calibrate op3, op4, and op5 into one fusion scope.
  - Use "sk2" to calibrate op7, op8, and op9 into one fusion scope.
  - op1, op2, and op6 are not included in any calibration group, so they do not participate in fusion.

- Calibrate operators that do not participate in SuperKernel fusion

    **Figure 2**  Diagram of marking operators that do not participate in fusion
    ![](figures/mark_op_no_fusion_diagram.png "Diagram of marking operators that do not participate in fusion")

    Operators marked with None: op3, op4, and op5 do not participate in SuperKernel fusion.

### Usage Constraints

If an operator that cannot be fused exists within the fusion scope, the first SuperKernel segment is generated, and the operator is automatically skipped before the second SuperKernel segment is fused. Therefore, the fusion scope does not represent the final fusion result.

## Fusion Scope Calibration Example

By calling the [aclskScopeBegin](../../api/graph_engine_api/c/super_kernel/aclskScopeBegin.md) and [aclskScopeEnd](../../api/graph_engine_api/c/super_kernel/aclskScopeEnd.md) APIs, you can pass a valid scopeName to mark operators for fusion into a SuperKernel, or pass nullptr to mark operators as not to be fused. This feature takes effect only when SuperKernel optimization is enabled. The following code demonstrates how to implement a simple addition operator on an NPU using the SuperKernel technology.

- **Example of Key Steps for Marking Operators for Fusion into a SuperKernel**

    The code shows only the core logic and cannot be compiled or run directly. It is for reference only:

    ```c++
    #include "acl/acl.h"
    #include "super_kernel/super_kernel.h"
    ...
    // Original kernel function (add_custom)
    __global__ __vector__ void add_custom(GM_ADDR x, GM_ADDR y, GM_ADDR z, AddCustomTilingData tiling)
    {
        KernelAdd op;
        op.Init(x, y, z, tiling.totalLength, tiling.tileNum);
        op.Process();
    }
    // Define a parameter structure to encapsulate the parameters passed to the SuperKernel
    struct ArgsStruct {
        GM_ADDR x;
        GM_ADDR y;
        GM_ADDR z;
        AddCustomTilingData tiling;
    };
    // Define a templated SuperKernel subfunction (add_custom_sk)
    template<uint32_t splitNum>
    __sk__ __vector__ void add_custom_sk(const ArgsStruct *args, sk::SkSystemArgs *sysArgs)
    {
        // Obtain parameters from the structure
        GM_ADDR x = args->x;
        GM_ADDR y = args->y;
        GM_ADDR z = args->z;
        AddCustomTilingData tiling = args->tiling;
        uint16_t blockNum = sysArgs->skNumBlocks;
        uint16_t blockIdx = sysArgs->skBlockIdx;
        // The logic is the same as that of the original kernel global function
        KernelAdd op;
        op.Init(x, y, z, tiling.totalLength, tiling.tileNum);
        op.Process();
    }

    // Use SK_BIND to bind the add_custom kernel with multiple SK subfunctions, and instantiate four different symbols by specifying template parameters
    SK_BIND(add_custom, 4, add_custom_sk<0>, add_custom_sk<1>, add_custom_sk<2>, add_custom_sk<3>);

    // Main function
    int main()
    {
        // Prepare operator data
        constexpr uint32_t totalLength = 8 * 2048;
        constexpr float valueX = 1.2f;
        constexpr float valueY = 2.3f;
        std::vector<float> x(totalLength, valueX);
        std::vector<float> y(totalLength, valueY);
        constexpr uint32_t numBlocks = 8;
        uint32_t totalLength = x.size();
        size_t totalByteSize = totalLength * sizeof(float);
        int32_t deviceId = 0;
        aclrtStream stream = nullptr;
        AddCustomTilingData tiling = {totalLength, 8};
        uint8_t *xHost = reinterpret_cast<uint8_t *>(x.data());
        // printf("0-host: x0:%d \n", xHost[0]);
        uint8_t *yHost = reinterpret_cast<uint8_t *>(y.data());
        uint8_t *zHost = nullptr;
        uint8_t *xDevice = nullptr;
        uint8_t *yDevice = nullptr;
        uint8_t *zDevice = nullptr;
        // Allocate memory
        aclrtMallocHost((void **)(&zHost), totalByteSize);
        aclrtMalloc((void **)&xDevice, totalByteSize, ACL_MEM_MALLOC_HUGE_FIRST);
        aclrtMalloc((void **)&yDevice, totalByteSize, ACL_MEM_MALLOC_HUGE_FIRST);
        aclrtMalloc((void **)&zDevice, totalByteSize, ACL_MEM_MALLOC_HUGE_FIRST);
        // Transfer data
        aclrtMemcpy(xDevice, totalByteSize, xHost, totalByteSize, ACL_MEMCPY_HOST_TO_DEVICE);
        aclrtMemcpy(yDevice, totalByteSize, yHost, totalByteSize, ACL_MEMCPY_HOST_TO_DEVICE);
        // Initialize
        aclInit(nullptr);
        aclrtSetDevice(deviceId);
        aclrtCreateStream(&stream);
        // Build the graph
        aclmdlRICaptureBegin(stream, ACL_MODEL_RI_CAPTURE_MODE_GLOBAL);
        aclmdlRI modelRI;
        // Mark the start position of SuperKernel fusion
        aclskScopeBegin("sk1", stream);
        add_custom<<<numBlocks, nullptr, stream>>>(xDevice, yDevice, zDevice, tiling);
        // Mark the end position of SuperKernel fusion
        aclskScopeEnd("sk1", stream);
        aclmdlRICaptureEnd(stream, &modelRI);
        // Enable SuperKernel optimization
        aclskOptimize(modelRI, nullptr);
        // Execute the graph
        aclmdlRIExecuteAsync(modelRI, stream);
        // Obtain the graph execution result
        aclrtSynchronizeStream(stream);
        aclrtMemcpy(zHost, totalByteSize, zDevice, totalByteSize, ACL_MEMCPY_DEVICE_TO_HOST);
        std::vector<float> z((float *)zHost, (float *)(zHost + totalLength));
        // Release resources
        aclrtFree(xDevice);
        aclrtFree(yDevice);
        aclrtFree(zDevice);
        aclrtFreeHost(zHost);
        aclrtDestroyStream(stream);
        aclrtResetDevice(deviceId);
        aclFinalize();
    }
    ```

    For details about the preceding ACL APIs, see *[Runtime APIs](https://gitcode.com/cann/runtime/blob/9.2.0/docs/zh/api_ref/README.md)*.

- **Example of key steps for marking an operator as not fused into SuperKernel**

    The following code shows only the core logic and cannot be directly compiled or run. It is for reference only:

    ```c++
    #include "acl/acl.h"
    #include "super_kernel/super_kernel.h"
    ...
    int main()
    {
        // Prepare operator data
        ...
        // Initialize
        aclInit(nullptr);
        aclrtSetDevice(deviceId);
        aclrtCreateStream(&stream);
        // Build the graph
        aclmdlRICaptureBegin(stream, ACL_MODEL_RI_CAPTURE_MODE_GLOBAL);
        aclmdlRI modelRI;
        // Mark the start position of SuperKernel fusion. Passing nullptr indicates no fusion
        aclskScopeBegin(nullptr, stream);
        add_custom<<<numBlocks, nullptr, stream>>>(xDevice, yDevice, zDevice, tiling);
        // Mark the end position of SuperKernel fusion. Passing nullptr indicates no fusion
        aclskScopeEnd(nullptr, stream);
        aclmdlRICaptureEnd(stream, &modelRI);
        // Enable SuperKernel optimization
        aclskOptimize(modelRI, nullptr);
        // Execute the graph
        aclmdlRIExecuteAsync(modelRI, stream);
        // Obtain the graph execution result
        ...
        // Release resources
        aclrtFree(xDevice);
        aclrtFree(yDevice);
        aclrtFree(zDevice);
        aclrtFreeHost(zHost);
        aclrtDestroyStream(stream);
        aclrtResetDevice(deviceId);
        aclFinalize();
    }
    ```

    For details about the preceding acl APIs, see [Runtime APIs](https://gitcode.com/cann/runtime/blob/9.2.0/docs/zh/api_ref/README.md).
