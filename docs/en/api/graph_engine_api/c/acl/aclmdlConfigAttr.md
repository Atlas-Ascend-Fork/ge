# aclmdlConfigAttr

<!-- md-trans-meta sourceCommit=fcba9ddddeed35cd2a7a0ade347fab638d62fcb2 translatedAt=2026-11-17T14:04:55.145Z pushedAt=2026-11-18T03:18:58.564Z nl=2 -->

## Definition

```c
typedef enum {
    ACL_MDL_PRIORITY_INT32 = 0,
    ACL_MDL_LOAD_TYPE_SIZET,
    ACL_MDL_PATH_PTR,
    ACL_MDL_MEM_ADDR_PTR,
    ACL_MDL_MEM_SIZET,
    ACL_MDL_WEIGHT_ADDR_PTR,
    ACL_MDL_WEIGHT_SIZET,
    ACL_MDL_WORKSPACE_ADDR_PTR,
    ACL_MDL_WORKSPACE_SIZET,
    ACL_MDL_INPUTQ_NUM_SIZET,
    ACL_MDL_INPUTQ_ADDR_PTR,
    ACL_MDL_OUTPUTQ_NUM_SIZET,
    ACL_MDL_OUTPUTQ_ADDR_PTR,
    ACL_MDL_WORKSPACE_MEM_OPTIMIZE,
    ACL_MDL_WEIGHT_PATH_PTR,
    ACL_MDL_MODEL_DESC_PTR,
    ACL_MDL_MODEL_DESC_SIZET,
    ACL_MDL_KERNEL_PTR,
    ACL_MDL_KERNEL_SIZET,
    ACL_MDL_KERNEL_ARGS_PTR,
    ACL_MDL_KERNEL_ARGS_SIZET,
    ACL_MDL_STATIC_TASK_PTR,
    ACL_MDL_STATIC_TASK_SIZET,
    ACL_MDL_DYNAMIC_TASK_PTR,
    ACL_MDL_DYNAMIC_TASK_SIZET,
    ACL_MDL_MEM_MALLOC_POLICY_SIZET,
    ACL_MDL_FIFO_PTR,
    ACL_MDL_FIFO_SIZET,
    ACL_MDL_WITHOUT_GRAPH_INT32,
    ACL_NPU_PERF_MODE,
    ACL_MDL_SCH_MODE_INT32
} aclmdlConfigAttr;
```

## ACL\_MDL\_PRIORITY\_INT32 Value Description

Priority of model execution. Optional. The value corresponding to this option is of the int32 type.

A smaller value indicates a higher priority. The value ranges from 0 to 7, and the default value is 0.

<!-- npu="910" id1 -->
For Atlas training products, this enumeration value does not take effect when CANN is installed in a Docker container and compute power grouping is configured.
<!-- end id1 -->

## ACL\_MDL\_LOAD\_TYPE\_SIZET Value Description

Model loading mode, required. The value corresponding to this option is of the size\_t type.

The values of ACL\_MDL\_LOAD\_TYPE\_SIZET (indicating the model loading mode) are as follows:

- ACL\_MDL\_LOAD\_FROM\_FILE

    ```c
    #define ACL_MDL_LOAD_FROM_FILE 1
    ```

- ACL\_MDL\_LOAD\_FROM\_FILE\_WITH\_MEM

    ```c
    #define ACL_MDL_LOAD_FROM_FILE_WITH_MEM 2
    ```

- ACL\_MDL\_LOAD\_FROM\_MEM

    ```c
    #define ACL_MDL_LOAD_FROM_MEM 3
    ```

- ACL\_MDL\_LOAD\_FROM\_MEM\_WITH\_MEM

    ```c
    #define ACL_MDL_LOAD_FROM_MEM_WITH_MEM 4
    ```

- ACL\_MDL\_LOAD\_FROM\_FILE\_WITH\_Q

    ```c
    #define ACL_MDL_LOAD_FROM_FILE_WITH_Q 5
    ```

- ACL\_MDL\_LOAD\_FROM\_MEM\_WITH\_Q

    ```c
    #define ACL_MDL_LOAD_FROM_MEM_WITH_Q 6
    ```

**Note**: If ACL\_MDL\_LOAD\_TYPE\_SIZET is set to ACL\_MDL\_LOAD\_FROM\_MEM, ACL\_MDL\_LOAD\_FROM\_MEM\_WITH\_MEM, or ACL\_MDL\_LOAD\_FROM\_MEM\_WITH\_Q, it indicates that the model data is loaded from memory, and the ACL\_MDL\_WEIGHT\_PATH\_PTR option can also be used to specify the weight file directory.

## ACL\_MDL\_PATH\_PTR Value Description

Pointer to the path of the om/om2 model file. This option is required if the model is loaded from a file. For details about how to obtain the model file, see "[--mode](../../../../user_guides/atc_tools/CLI_options/--mode.md)" in *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*.

<!-- npu="IPV350" id2 -->
On IPV350, model files with the exeom suffix are also supported. The differences between om and exeom model files are as follows:

- The \*.om file is not aware of the specific hardware scheduling capability and contains an abstract data structure in an intermediate state. During the model loading phase, the runtime data structure is generated based on the scheduling characteristics of the specific execution platform.
- The \*.exeom file is aware of the specific hardware scheduling capability and contains the runtime data structure of the target execution platform (this data is saved in binary form in the \*.exeom file). During the model loading phase, the binary content is loaded and restored. Based on the data area address passed by the user application or the actually allocated data address, the address pointer values in the binary are refreshed, and then the binary content is directly copied to the Device, improving the model loading performance and reducing the peak memory occupancy during model loading. **In some resource-constrained scenarios, it is recommended to use the \*.exeom model file to enhance product competitiveness.**
<!-- end id2 -->

## ACL\_MDL\_MEM\_ADDR\_PTR Value Description

Start address of the om/om2 model in memory. This option is required if the model is loaded from memory.

## ACL\_MDL\_MEM\_SIZET Value Description

Size of the model in memory. If the model is loaded from memory, this option is required and is used together with the ACL\_MDL\_MEM\_ADDR\_PTR option. The value corresponding to this option is of the size\_t type.

## ACL\_MDL\_WEIGHT\_ADDR\_PTR Value Description

Pointer to the om/om2 model weight memory (where weight data is stored) on the Device. This option is required if you need to manage the weight memory. If this option is not configured, the memory is managed by the system.

## ACL\_MDL\_WEIGHT\_SIZET Value Description

Size of the weight memory, in bytes. This option is required if you need to manage the weight memory, and it is used together with the ACL\_MDL\_WEIGHT\_ADDR\_PTR option. The value of this option is of the size\_t type.

## ACL\_MDL\_WORKSPACE\_ADDR\_PTR Value Description

Pointer to the working memory required by the om/om2 model on the Device (for storing temporary data during model execution). This option is required if you need to manage the working memory. If this option is not configured, the memory is managed by the system.

<!-- npu="IPV350" id3 -->
This configuration is not supported in the current version.
<!-- end id3 -->

## ACL\_MDL\_WORKSPACE\_SIZET Value Description

Size of the working memory required by the om/om2 model, in bytes. This option is required if you need to manage the working memory, and it is used together with the ACL\_MDL\_WORKSPACE\_ADDR\_PTR option. The value corresponding to this option is of the size\_t type.

<!-- npu="IPV350" id4 -->
This configuration is not supported in the current version.
<!-- end id4 -->

## ACL\_MDL\_INPUTQ\_NUM\_SIZET Value Description

om model input queue size. When loading a model with a queue, this option is required and is used together with the ACL\_MDL\_INPUTQ\_ADDR\_PTR option. The value corresponding to this option is of the size\_t type.

<!-- npu="IPV350" id5 -->
This configuration is not supported in the current version.
<!-- end id5 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id1 -->

## ACL\_MDL\_INPUTQ\_ADDR\_PTR Value Description

Pointer to the om model input queue ID. This option is required when loading a model with a queue. Each model input corresponds to one queue ID.

<!-- npu="IPV350" id6 -->
This configuration is not supported in the current version.
<!-- end id6 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id2 -->

## ACL\_MDL\_OUTPUTQ\_NUM\_SIZET Value Description

Size of the om model output queue. This option is required when loading a model with a queue, and is used together with the ACL\_MDL\_OUTPUTQ\_ADDR\_PTR option. The value corresponding to this option is of the size\_t type.

<!-- npu="IPV350" id7 -->
This configuration is not supported in the current version.
<!-- end id7 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id3 -->

## ACL\_MDL\_OUTPUTQ\_ADDR\_PTR Value Description

Pointer to the om model output queue ID. This option is required when loading a model with a queue. Each model output corresponds to one queue ID.

<!-- npu="IPV350" id8 -->
This configuration is not supported in the current version.
<!-- end id8 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id4 -->

## ACL\_MDL\_WORKSPACE\_MEM\_OPTIMIZE Value Description

Optimization method for the working memory of the om/om2 model. This is an optional item. The value corresponding to this option is of the size\_t type. The following values are currently supported:

- ACL\_WORKSPACE\_MEM\_OPTIMIZE\_DEFAULT (0): Default mode. No working memory optimization is performed.
- ACL\_WORKSPACE\_MEM\_OPTIMIZE\_INPUTOUTPUT (1): Input/output memory optimization mode. Reduces the memory reserved in the working memory for model inputs and outputs that support zero-copy. The actual amount saved depends on the memory planning of the model.

If memory planning is a concern or memory resources are limited, it is recommended to load the model with memory managed by the system and enable the working memory optimization function. In this case, the working memory does not include the memory for storing model input and output data, so the working memory size is reduced, achieving the goal of saving memory. Before model execution, you need to allocate memory for storing model input and output data. Therefore, even if the working memory optimization function is enabled during model loading, it will not affect subsequent model execution.

<!-- npu="IPV350" id9 -->
This configuration is not supported in the current version.
<!-- end id9 -->

## ACL\_MDL\_WEIGHT\_PATH\_PTR Value Description

Pointer to the directory where the weight file is stored. In scenarios where the size of the om/om2 model file is limited, this parameter enables the external weight file feature.

If ACL\_MDL\_LOAD\_TYPE\_SIZET is set to ACL\_MDL\_LOAD\_FROM\_MEM, ACL\_MDL\_LOAD\_FROM\_MEM\_WITH\_MEM, or ACL\_MDL\_LOAD\_FROM\_MEM\_WITH\_Q, indicating that model data is loaded from memory, ACL\_MDL\_WEIGHT\_PATH\_PTR can be used to specify the weight file directory.

Generally, in scenarios where the om/om2 model file size is limited or the model file is encrypted, the weight file needs to be loaded separately. Therefore, when building the model, the weights must be saved in a separate file. For example, when using the ATC tool to generate om/om2 files, set the --external\_weight parameter to 1 (1 indicates that the weights of Const/Constant nodes in the original network are saved in a separate file).

**Note:** After the pointer to the weight file directory is configured through ACL\_MDL\_WEIGHT\_PATH\_PTR, during model loading, the Graph Engine (GE) applies for a piece of Device memory on its own, reads the external weight file, and copies the external weight data to this Device memory. This memory is released when the model is unloaded. If the [aclmdlSetExternalWeightAddress](aclmdlSetExternalWeightAddress.md) API has been called to configure the Device memory for storing external weights, and you manage the Device memory yourself, you do not need to configure ACL\_MDL\_WEIGHT\_PATH\_PTR. If both ACL\_MDL\_WEIGHT\_PATH\_PTR and the [aclmdlSetExternalWeightAddress](aclmdlSetExternalWeightAddress.md) API are configured for the same model, the [aclmdlSetExternalWeightAddress](aclmdlSetExternalWeightAddress.md) API takes precedence.

## ACL\_MDL\_MODEL\_DESC\_PTR Value Description

Memory pointer for storing om model description information.

<!-- npu="950,A3,910b,910,310p,310b" id10 -->
This configuration is not supported in the current version.
<!-- end id10 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id5 -->

## ACL\_MDL\_MODEL\_DESC\_SIZET Value Description

Memory size required for storing model description information, in bytes. The value corresponding to this option is of the size\_t type.

You can call the [aclmdlQueryExeOMDesc](aclmdlQueryExeOMDesc.md) API in advance to obtain the memory size required for storing model description information. This option must be used together with the ACL\_MDL\_MODEL\_DESC\_PTR option.

<!-- npu="950,A3,910b,910,310p,310b" id11 -->
This configuration is not supported in the current version.
<!-- end id11 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id6 -->

## ACL\_MDL\_KERNEL\_PTR Value Description

Memory pointer that stores the TBE operator kernel (the operator's \*.o and \*.json).

<!-- npu="950,A3,910b,910,310p,310b" id12 -->
This configuration is not supported in the current version.
<!-- end id12 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id7 -->

## ACL\_MDL\_KERNEL\_SIZET Value Description

Memory size required to store the TBE operator kernel (the operator's \*.o and \*.json), in bytes. The value corresponding to this option is of the size\_t type.

You can call the [aclmdlQueryExeOMDesc](aclmdlQueryExeOMDesc.md) API in advance to obtain the memory size required to store the TBE operator kernel (the operator's \*.o and \*.json), and this option must be used together with the ACL\_MDL\_KERNEL\_PTR option.

<!-- npu="950,A3,910b,910,310p,310b" id13 -->
This configuration is not supported in the current version.
<!-- end id13 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id8 -->

## ACL\_MDL\_KERNEL\_ARGS\_PTR Value Description

Memory pointer for storing the kernel parameters of TBE operators.

<!-- npu="950,A3,910b,910,310p,310b" id14 -->
This configuration is not supported in the current version.
<!-- end id14 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id9 -->

## ACL\_MDL\_KERNEL\_ARGS\_SIZET Value Description

Memory size required for storing the kernel parameters of TBE operators, in bytes. The value of this option is of the size\_t type.

You can call [aclmdlQueryExeOMDesc](aclmdlQueryExeOMDesc.md) in advance to obtain the memory size required for storing the kernel parameters of TBE operators. This option must be used together with the ACL\_MDL\_KERNEL\_ARGS\_PTR option.

<!-- npu="950,A3,910b,910,310p,310b" id15 -->
This configuration is not supported in the current version.
<!-- end id15 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id10 -->

## ACL\_MDL\_STATIC\_TASK\_PTR Value Description

Memory pointer for storing the static shape task description information.

<!-- npu="950,A3,910b,910,310p,310b" id16 -->
This configuration is not supported in the current version.
<!-- end id16 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id11 -->

## ACL\_MDL\_STATIC\_TASK\_SIZET Value Description

Memory size required for storing static shape task description information, in bytes. The value corresponding to this option is of the size\_t type.

You can call [aclmdlQueryExeOMDesc](aclmdlQueryExeOMDesc.md) in advance to obtain the memory size required for storing static shape task description information. This option must be used together with the ACL\_MDL\_STATIC\_TASK\_PTR option.

<!-- npu="950,A3,910b,910,310p,310b" id17 -->
This configuration is not supported in the current version.
<!-- end id17 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id12 -->

## ACL\_MDL\_DYNAMIC\_TASK\_PTR Value Description

Memory pointer for storing the dynamic shape task description information.

<!-- npu="950,A3,910b,910,310p,310b" id18 -->
This configuration is not supported in the current version.
<!-- end id18 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id13 -->

## ACL\_MDL\_DYNAMIC\_TASK\_SIZET Value Description

Memory size required for storing dynamic shape task description information, in bytes. The value corresponding to this option is of the size\_t type.

You can call [aclmdlQueryExeOMDesc](aclmdlQueryExeOMDesc.md) in advance to obtain the memory size required for storing dynamic shape task description information. This option must be used together with the ACL\_MDL\_DYNAMIC\_TASK\_PTR option.

<!-- npu="950,A3,910b,910,310p,310b" id19 -->
This configuration is not supported in the current version.
<!-- end id19 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id14 -->

## ACL\_MDL\_MEM\_MALLOC\_POLICY\_SIZET Value Description

Memory allocation rule. The value corresponding to this option is of the size\_t type.

<!-- npu="950,A3,910b,910,310p,310b" id20 -->
This configuration is not supported in the current version.
<!-- end id20 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id15 -->

<!-- npu="IPV350" id21 -->
**The following values are supported:**

- ACL\_MEM\_MALLOC\_HUGE\_FIRST

    When the allocated memory is less than or equal to 1 MB, normal page memory is allocated even if this memory allocation rule is used. When the allocated memory is greater than 1 MB, large page memory is allocated preferentially. If large page memory is insufficient, normal page memory is used.

- ACL\_MEM\_MALLOC\_HUGE\_ONLY

    When this option is configured, only large pages are allocated. If large page memory is insufficient, an error is returned.

- ACL\_MEM\_MALLOC\_NORMAL\_ONLY

    Only normal pages are allocated. If normal page memory is insufficient, an error is returned.

- ACL\_MEM\_TYPE\_LOW\_BAND\_WIDTH

    Allocate memory from physical memory with low bandwidth.

- ACL\_MEM\_TYPE\_HIGH\_BAND\_WIDTH

    Allocate memory from physical memory with high bandwidth.

**A single value or multiple values combined with bitwise OR are supported here:**

- **Configure a single value**:

    If one of ACL\_MEM\_MALLOC\_HUGE\_FIRST, ACL\_MEM\_MALLOC\_HUGE\_ONLY, or ACL\_MEM\_MALLOC\_NORMAL\_ONLY is configured, the system will internally select to allocate memory from high-bandwidth or low-bandwidth physical memory according to the hardware support.

    If ACL\_MEM\_TYPE\_LOW\_BAND\_WIDTH or ACL\_MEM\_TYPE\_HIGH\_BAND\_WIDTH is configured, the system will internally adopt ACL\_MEM\_MALLOC\_HUGE\_FIRST by default and allocate large pages preferentially.

- **Configure multiple values with bitwise OR**: The three items (ACL\_MEM\_MALLOC\_HUGE\_FIRST, ACL\_MEM\_MALLOC\_HUGE\_ONLY, ACL\_MEM\_MALLOC\_NORMAL\_ONLY) can be combined with the two items (ACL\_MEM\_TYPE\_LOW\_BAND\_WIDTH, ACL\_MEM\_TYPE\_HIGH\_BAND\_WIDTH). **For example**: ACL\_MEM\_MALLOC\_HUGE\_FIRST | ACL\_MEM\_TYPE\_HIGH\_BAND\_WIDTH
<!-- end id21 -->

## ACL\_MDL\_FIFO\_PTR Value Description

Start address of the om model-level global memory. This refers to the memory on the Device.

<!-- npu="950,A3,910b,910,310p,310b" id22 -->
This configuration is not supported in the current version.
<!-- end id22 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id16 -->

<!-- npu="IPV350" id23 -->
If, during inference, the input of each layer of a model is formed by concatenating the output of the previous layer and the inference results of previous rounds, model-level global memory is required to save the input data needed by the model for subsequent inference.

**Note**: In the scenario where a model is loaded once and multiple inferences are executed in parallel, multiple inference tasks may access the model-level global memory simultaneously, causing abnormal inference results. It is recommended to load the model once and then execute multiple inferences serially.
<!-- end id23 -->

## ACL\_MDL\_FIFO\_SIZET Value Description

Size of the model-level global memory. The value corresponding to this option is of the size\_t type.

<!-- npu="950,A3,910b,910,310p,310b" id24 -->
This configuration is not supported in the current version.
<!-- end id24 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id17 -->

<!-- npu="IPV350" id25 -->
The [aclmdlQueryExeOMDesc](aclmdlQueryExeOMDesc.md) API can be called in advance to obtain the size of the model-level global memory.
<!-- end id25 -->

## ACL\_MDL\_WITHOUT\_GRAPH\_INT32 Value Description

Whether to retain the computation graph and its pre-cached information after the om/om2 model is loaded. Only scenarios with static input shapes are supported. The value corresponding to this option is of the INT32 type. The value range is as follows:

- 0: Default value. The computation graph and its pre-cached information are retained.
- 1: The computation graph and its pre-cached information are not retained. When set to 1, the computation graph and its pre-cached information are released after the model is loaded successfully to save memory. However, releasing the pre-cached information affects Profiling data collection and operator information acquisition in abnormal scenarios. Therefore:
    - In Profiling data collection scenarios, call the aclprofStart API to enable Profiling before model loading. Otherwise, the Profiling function is unavailable.
    - In abnormal scenarios, calling the [aclmdlCreateAndGetOpDesc](aclmdlCreateAndGetOpDesc.md) API to obtain operator description information fails.

## ACL\_MDL\_SCH\_MODE\_INT32 Value Description

Model scheduling mode. This is an optional item and must be configured before model loading. It takes effect when the [aclmdlLoadWithConfig](aclmdlLoadWithConfig.md) API is called to load the model. The value corresponding to this option is of the INT32 type. The value range is as follows:

- 0 (ACL\_MDL\_SCH\_MODE\_NORMAL): Default value. Normal scheduling mode, where multiple models share scheduling resources.
- 1 (ACL\_MDL\_SCH\_MODE\_EXCLUSIVE): Exclusive scheduling mode, where the model exclusively occupies scheduling resources. This mode is suitable for low-latency and deterministic-latency scenarios.

```c
#define ACL_MDL_SCH_MODE_NORMAL 0
#define ACL_MDL_SCH_MODE_EXCLUSIVE 1
```

**Constraints**:

- Only static graph models in OM2 format are supported. If this option is configured when loading an OM-format model or a dynamic graph model, model loading fails and ACL\_ERROR\_FEATURE\_UNSUPPORTED is returned.
- When exclusive scheduling mode is configured, the model file must be compiled and generated by the ATC tool of the matching version. If an existing model compiled by an older version of ATC is used, model loading fails and a recompilation prompt is displayed.
- The bundle model loading scenario is not supported ([aclmdlBundleLoadModelWithConfig](aclmdlBundleLoadModelWithConfig.md) API).
