# aclmdlConfigAttr

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:59:01.328Z pushedAt=2026-10-29T02:59:13.548Z nl=2 -->

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
    ACL_MDL_WITHOUT_GRAPH_INT32
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

Pointer to the path of the om Model File. This option is required if the model is loaded from a file. For details about how to obtain the Model File, see "[--mode](../../../../user_guides/atc_tools/CLI_options/--mode.md)" in *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*.

## ACL\_MDL\_MEM\_ADDR\_PTR Value Description

Start address of the model in memory. This option is required if the model is loaded from memory.

## ACL\_MDL\_MEM\_SIZET Value Description

Size of the model in memory. If the model is loaded from memory, this option is required and is used together with the ACL\_MDL\_MEM\_ADDR\_PTR option. The value corresponding to this option is of the size\_t type.

## ACL\_MDL\_WEIGHT\_ADDR\_PTR Value Description

Pointer to the model weight memory (where weight data is stored) on the Device. This option is required if you need to manage the weight memory yourself. If this option is not configured, the memory is managed by the system.

## ACL\_MDL\_WEIGHT\_SIZET Value Description

Size of the weight memory, in bytes. This option is required if you need to manage the weight memory, and it is used together with the ACL\_MDL\_WEIGHT\_ADDR\_PTR option. The value of this option is of the size\_t type.

## ACL\_MDL\_WORKSPACE\_ADDR\_PTR Value Description

Pointer to the working memory required by the model on the Device (for storing temporary data during model execution). This option is required if you need to manage the working memory. If this option is not configured, the memory is managed by the system.

## ACL\_MDL\_WORKSPACE\_SIZET Value Description

Size of the working memory required by the model, in bytes. This option is required if you need to manage the working memory, and it is used together with the ACL\_MDL\_WORKSPACE\_ADDR\_PTR option. The value of this option is of the size\_t type.

## ACL\_MDL\_INPUTQ\_NUM\_SIZET Value Description

Model input queue size. When loading a model with a queue, this option is required and is used together with the ACL\_MDL\_INPUTQ\_ADDR\_PTR option. The value corresponding to this option is of the size\_t type.

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id1 -->

## ACL\_MDL\_INPUTQ\_ADDR\_PTR Value Description

Pointer to the model input queue ID. This option is required when loading a model with a queue. Each model input corresponds to one queue ID.

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id2 -->

## ACL\_MDL\_OUTPUTQ\_NUM\_SIZET Value Description

Size of the model output queue. This option is required when loading a model with a queue, and is used together with the ACL\_MDL\_OUTPUTQ\_ADDR\_PTR option. The value corresponding to this option is of the size\_t type.

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id3 -->

## ACL\_MDL\_OUTPUTQ\_ADDR\_PTR Value Description

Pointer to the model output queue ID. This option is required when loading a model with queues, and each model output corresponds to one queue ID.

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id4 -->

## ACL\_MDL\_WORKSPACE\_MEM\_OPTIMIZE Value Description

Whether to enable model working memory optimization. 1 enables it, and 0 disables it. If you care about memory planning or have limited memory resources, load the model in the mode where the system manages memory and enable working memory optimization. In this case, the working memory no longer contains the memory for storing model input and output data, so the working memory size decreases, saving memory. Before model execution, you still need to apply for the memory for storing model input and output data. Therefore, enabling working memory optimization during model loading does not affect subsequent model execution.

## ACL\_MDL\_WEIGHT\_PATH\_PTR Value Description

Pointer to the directory where the weight file is stored. In scenarios where the size of the om model file is limited, this parameter enables the external weight file feature.

If ACL\_MDL\_LOAD\_TYPE\_SIZET is set to ACL\_MDL\_LOAD\_FROM\_MEM, ACL\_MDL\_LOAD\_FROM\_MEM\_WITH\_MEM, or ACL\_MDL\_LOAD\_FROM\_MEM\_WITH\_Q, indicating that model data is loaded from memory, ACL\_MDL\_WEIGHT\_PATH\_PTR can be used to specify the weight file directory.

Generally, in scenarios where the size of the om model file is limited or the model file is encrypted, the weight file must be loaded separately. Therefore, when building the model, the weights must be saved in a separate file. For example, when using the ATC tool to generate an om file, set the --external\_weight parameter to 1 (1 indicates that the weights of the Const/Constant nodes in the original network are saved in a separate file).

**Note:** After the pointer to the weight file directory is configured through ACL\_MDL\_WEIGHT\_PATH\_PTR, during model loading, the Graph Engine (GE) applies for a piece of Device memory on its own, reads the external weight file, and copies the external weight data to this Device memory. This memory is released when the model is unloaded. If the [aclmdlSetExternalWeightAddress](aclmdlSetExternalWeightAddress.md) API has been called to configure the Device memory for storing external weights, and you manage the Device memory yourself, you do not need to configure ACL\_MDL\_WEIGHT\_PATH\_PTR. If both ACL\_MDL\_WEIGHT\_PATH\_PTR and the [aclmdlSetExternalWeightAddress](aclmdlSetExternalWeightAddress.md) API are configured for the same model, the [aclmdlSetExternalWeightAddress](aclmdlSetExternalWeightAddress.md) API takes precedence.

## ACL\_MDL\_MODEL\_DESC\_PTR Value Description

Memory pointer for storing model description information.

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
The current version does not support this configuration.
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
The current version does not support this configuration.
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

## ACL\_MDL\_FIFO\_PTR Value Description

Start address of the model-level global memory. This refers to the memory on the Device.

<!-- npu="950,A3,910b,910,310p,310b" id22 -->
The current version does not support this configuration.
<!-- end id22 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id16 -->

## ACL\_MDL\_FIFO\_SIZET Value Description

Size of the model-level global memory. The value corresponding to this option is of the size\_t type.

<!-- npu="950,A3,910b,910,310p,310b" id24 -->
This configuration is not supported in the current version.
<!-- end id24 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlConfigAttr_res.md#id17 -->

## ACL\_MDL\_WITHOUT\_GRAPH\_INT32 Value Description

Whether to retain the computation graph and its pre-cached information after model loading. This option applies only to scenarios where the input has a static shape. The value of this option is of the INT32 type, with the following value range:

- 0: Default value. The computation graph and its pre-cached information are retained.
- 1: The computation graph and its pre-cached information are not retained. When set to 1, the computation graph and its pre-cached information are released after the model is loaded successfully to save memory. However, releasing the pre-cached information affects Profiling data collection and operator information acquisition in abnormal scenarios. Therefore:
    - In Profiling data collection scenarios, call the aclprofStart API to enable Profiling before model loading. Otherwise, the Profiling function is unavailable.
    - In abnormal scenarios, calling the [aclmdlCreateAndGetOpDesc](aclmdlCreateAndGetOpDesc.md) API to obtain operator description information fails.
