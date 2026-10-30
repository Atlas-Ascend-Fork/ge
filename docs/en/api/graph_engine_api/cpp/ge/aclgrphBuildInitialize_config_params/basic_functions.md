# Basic Functions

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-16T11:32:01.476Z pushedAt=2026-10-28T07:40:22.192Z nl=2 -->

## CLUSTER\_CONFIG

Sets the **cluster** configuration file path, which is used to configure the target execution logical device information to generate HCCL tasks. It is used in multi-card distributed training scenarios. The corresponding **options** parameter of this parameter is `ge.cluster_config`.

If the model contains communication operators (currently only the AllGather communication operator is supported), this parameter is mandatory.

**Configuration example:**

```c++
{ge::ir_option::CLUSTER_CONFIG, "/home/test/cluster_config.json"}
```

The configuration example of **cluster_config.json** is as follows:

```json
{
    "RankTable": {
        "status": "completed",
        "version": "1.2",
        "server_count": "2",
        "server_list": [
            {
                "server_id": "node_0",
                "device": [
                    {
                        "device_id": "0",
                        "rank_id": "0"
                    }
                ]
            },
            {
                "server_id": "node_1",
                "device": [
                    {
                        "device_id": "0",
                        "rank_id": "1"
                    }
                ]
            }
        ]
    },
    "HcclCommConfig": {
        "hcclCommName": "global name"
    }
}
```

The parameter description is as follows:

- **RankTable**: Core of the configuration, which defines the nodes and rank information involved in the computation.
  - **status**: Required. Availability flag of the rank table.
    - **completed**: Used to indicate that the rank table is available.
    - **initializing**: Used to indicate that the rank table is unavailable.

  - **version**: Required. Rank table template version information. Set it to **1.2**.
  - **server\_count**: Optional. Number of AI Servers participating in collective communication.
  - **server\_list**: Required. List of AI servers participating in collective communication.
    - server\_id: Required. AI Server identifier, of string type, with a length less than or equal to 64. Ensure that it is globally unique. Configuration example: **node\_0**.
    - device: Required. Device list.
      - device\_id: Required. Physical ID of the AI processor, that is, the serial number of the device on the AI server.
      - rank\_id: Required. Unique rank identifier. Set it to an integer starting from 0, and ensure that it is globally unique. Value range: \[0, total number of devices -1\].

- **HcclCommConfig**: Global configuration information of the HCCL communication layer.
  - **hcclCommName**: Required. Name or identifier of the global communication group.

**Applicable products:**

<!-- npu="950" id1 -->
- Ascend 950PR & 950DT series products: Not supported
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3 series products: Not supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 series products: Not supported
<!-- end id3 -->
<!-- npu="310b" id4 -->
- Atlas 200I/500 A2 inference product: Not supported
<!-- end id4 -->
<!-- npu="310p" id5 -->
- Atlas inference series products: Not supported
<!-- end id5 -->
<!-- npu="910" id6 -->
- Atlas training series products: Not supported
<!-- end id6 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/cpp/ge/aclgrphBuildInitialize_config_params/basic_functions_res.md#id1 -->

## DETERMINISTIC

Whether to enable deterministic computing. The **options** parameter actually corresponding to this parameter is `ge.deterministic`.

By default, deterministic computing is disabled. Under the same hardware and input, an operator may produce different results across multiple executions. This difference generally arises because the operator implementation involves asynchronous multi-threaded execution, which changes the order of floating-point accumulation. When deterministic computing is enabled, the operator produces the same output across multiple executions under the same hardware and input.

It is generally recommended not to enable deterministic computing, because deterministic computing often slows down operator execution and thus affects performance. When you find that a model produces different results across multiple executions, or when performing precision tuning, you can enable deterministic computing to assist with model debugging and tuning.

**Parameter values:**

- **0**: (default value) Disables deterministic computing.
- **1**: Enables deterministic computing.

**Configuration example:**

```c++
{ge::ir_option::DETERMINISTIC, "1"}
```

**Constraints:**

When building an offline model via the GE graph engine API, it is recommended to set the deterministic and consistency configuration during the model construction phase via `ge::ir_option::DETERMINISTIC` and `ge::ir_option::DETERMINISTIC_LEVEL`. When loading and executing an offline model via the `aclmdl` API, it is not recommended to call `aclrtSetSysParamOpt` or `aclrtCtxSetSysParamOpt` to modify this configuration via `ACL_OPT_DETERMINISTIC`. Otherwise, the runtime configuration may become inconsistent with the configuration used during model construction, and the expected deterministic and consistency effects cannot be guaranteed. If you need to adjust this configuration, it is recommended to rebuild the offline model.

**Applicable products:**

<!-- npu="950" id8 -->
- Ascend 950PR&950DT products: Supported
<!-- end id8 -->
<!-- npu="A3" id9 -->
- Atlas A3 products: Supported
<!-- end id9 -->
<!-- npu="910b" id10 -->
- Atlas A2 products: Supported
<!-- end id10 -->
<!-- npu="310b" id11 -->
- Atlas 200I/500 A2 inference product: Not supported
<!-- end id11 -->
<!-- npu="310p" id12 -->
- Atlas inference series products: Supported
<!-- end id12 -->
<!-- npu="910" id13 -->
- Atlas training series products: Supported
<!-- end id13 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/cpp/ge/aclgrphBuildInitialize_config_params/basic_functions_res.md#id2 -->

## ENABLE\_SINGLE\_STREAM

In static shape scenarios, whether to enable serial execution of model inference in a single stream. The corresponding options parameter of this parameter is `ge.enableSingleStream`.

A stream is used to maintain the execution order of asynchronous operations, ensuring that they are executed on the device in the order in which the code is called in the application.

**Parameter values:**

- **true**: Enables the feature, so that model inference is executed serially in a single stream.
- **false**: (Default value) Disables the feature, so that model inference is executed in parallel across multiple streams.

**Parameter value constraints:**

When the model contains the **Cmo** operator and the following control operators, the single-stream feature cannot be used, and only the default value **false** can be used.

- Merge
- Switch
- Enter
- RefEnter

**Configuration example:**

```c++
{ge::ir_option::ENABLE_SINGLE_STREAM, "true"}
```

**Applicable products:**

<!-- npu="950" id15 -->
- Ascend 950PR&950DT series products: Supported
<!-- end id15 -->
<!-- npu="A3" id16 -->
- Atlas A3 series products: Supported
<!-- end id16 -->
<!-- npu="910b" id17 -->
- Atlas A2 series products: Supported
<!-- end id17 -->
<!-- npu="310b" id18 -->
- Atlas 200I/500 A2 inference products: Not supported
<!-- end id18 -->
<!-- npu="310p" id19 -->
- Atlas inference products: Supported
<!-- end id19 -->
<!-- npu="910" id20 -->
- Atlas training products: Supported
<!-- end id20 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/cpp/ge/aclgrphBuildInitialize_config_params/basic_functions_res.md#id3 -->

## HCCL\_SUB\_COMM\_CONFIG

Sets the path of the HCCL **sub-communication domain** configuration file, which is used to configure HCCL sub-communication parameters. It is used in multi-card distributed training scenarios. The actual options parameter corresponding to this parameter is `ge.hccl_sub_comm_config`.

If the model contains communication operators (currently only AllGather is supported), this parameter is mandatory. If the model operators contain sub-communication domain parameters but the **HCCL\_SUB\_COMM\_CONFIG** parameter is not specified during model compilation, the system executes based on the actual service configuration of the cluster by default.

**Configuration example:**

```c++
{ge::ir_option::CLUSTER_CONFIG, "/home/test/cluster_config.json"}
{ge::ir_option::HCCL_SUB_COMM_CONFIG, "/home/test/sub_comm_config.json"}
```

The sub\_comm\_config.json file example is as follows:

```json
{
  "GroupList": [
    {
      "RankIds": [0,1],
      "HcclCommconfig": {
        "hcclCommName": "test_group"
      }
    }
  ]
}
```

Where:

- **RankIds**: Unique rank identifier. Configure it as an integer, starting from 0, and globally unique. Value range: \[0, total number of devices -1\].

    **It is recommended to sort **rank\_id** by the physical connection order of devices, that is, to group devices that are physically closer together. Otherwise, performance may be affected.**

    For example, if **device\_ip** is set in ascending order of physical connections, **rank\_id** is also recommended to be set in ascending order.

- **hcclCommName**: Communication group name, used for program invocation and distinction.

**Applicable products:**

<!-- npu="950" id22 -->
- Ascend 950PR & 950DT series products: Not supported
<!-- end id22 -->
<!-- npu="A3" id23 -->
- Atlas A3 series products: Not supported
<!-- end id23 -->
<!-- npu="910b" id24 -->
- Atlas A2 series products: Not supported
<!-- end id24 -->
<!-- npu="310b" id25 -->
- Atlas 200I/500 A2 inference products: Not supported
<!-- end id25 -->
<!-- npu="310p" id26 -->
- Atlas inference products: Not supported
<!-- end id26 -->
<!-- npu="910" id27 -->
- Atlas training products: Not supported
<!-- end id27 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/cpp/ge/aclgrphBuildInitialize_config_params/basic_functions_res.md#id4 -->

## OPTION\_HOST\_ENV\_OS

The actual **options** parameter corresponding to this parameter is `ge.host_env_os`.

If the operating system and architecture of the model compilation environment differ from those of the model runtime environment, use this parameter to set the operating system type of the model runtime environment. If this parameter is not set, the operating system type of the model compilation environment is used by default.

When the operating system and architecture of the model compilation environment differ from those of the model runtime environment, this parameter must be used together with the **OPTION\_HOST\_ENV\_CPU** parameter. Set the operating system type via the **OPTION\_HOST\_ENV\_OS** parameter and the operating system architecture via the **OPTION\_HOST\_ENV\_CPU** parameter.

**Parameter values:**

View the OS type of the packaged operator .so files under `${INSTALL_DIR}/opp/built-in/op_graph/lib/`.

**Default value:**

View the value in the `${INSTALL_DIR}/opp/scene.info` file.

Replace `${INSTALL_DIR}` with the file storage path after CANN software installation. For example, when installed as the root user, the default file storage path is **/usr/local/Ascend/cann**.

**Configuration example:**

```c++
{ge::OPTION_HOST_ENV_OS, "linux"},
{ge::OPTION_HOST_ENV_CPU, "x86_64"}
```

**Applicable products:**

Supported on all chips.

## OPTION\_HOST\_ENV\_CPU

The actual **options** parameter corresponding to this parameter is `ge.host_env_cpu`.

If the operating system and its architecture of the model compilation environment differ from those of the model runtime environment, use this parameter to set the operating system architecture of the model runtime environment. If this parameter is not set, the operating system architecture of the model compilation environment is used by default.

If the operating system and its architecture of the model compilation environment differ from those of the model runtime environment, use this parameter together with the **OPTION\_HOST\_ENV\_OS** parameter: set the operating system type via the **OPTION\_HOST\_ENV\_OS** parameter and the operating system architecture via the **OPTION\_HOST\_ENV\_CPU** parameter.

**Parameter values:**

View the OS/CPU type of the packaged operator .so files under `${INSTALL_DIR}/opp/built-in/op_graph/lib/`.

**Default value**: View the value in the `${INSTALL_DIR}/opp/scene.info` file.

Replace `${INSTALL_DIR}` with the file storage path after CANN software installation. For example, when installed as the root user, the default file storage path after installation is **/usr/local/Ascend/cann**.

**Configuration example:**

```c++
{ge::OPTION_HOST_ENV_OS, "linux"},
{ge::OPTION_HOST_ENV_CPU, "x86_64"}
```

- If the converted offline model contains the operating system type and architecture, for example, _xxx\_linux\_x86\_64_.om, the environment in which the model runs can only be a Linux operating system with the x86\_64 architecture.
- If the converted offline model does not contain the operating system type or architecture, for example, _xxx_.om, all operating systems supported by the CANN software package support running this model.

**Applicable products:**

Supported on all chips.

## SOC\_VERSION

AI processor model used during graph compilation. The corresponding **options** parameter of this parameter is `ge.socVersion`.

- If an AI processor exists in the current environment, this parameter is optional.
- If no AI processor exists in the current environment, that is, in a development environment, this parameter is mandatory.

**Parameter values:**

For the query method of <soc\_version\>, see [--soc_version](../../../../../user_guides/atc_tools/CLI_options/--soc_version.md).

**Configuration example:**

```c++
{ge::ir_option::SOC_VERSION, "<soc_version>"}
```

**Applicable products:**

Supported on all chips.

<!-- npu="950,A3,910b,910,310p,310b" id44 -->
## VIRTUAL\_TYPE

Whether the offline model can run on virtual devices generated by the Ascend virtualization instance feature. The options parameter actually corresponding to this parameter is `ge.virtual_type`.

Current chips provide large computing power, which cloud users or small enterprises do not fully need. The Ascend virtualization instance feature supports partitioning chip computing power, allowing users to apply for computing power on demand based on their own services.

A virtual device refers to virtual acceleration resources applied for on a chip based on specified computing power.

**Parameter values:**

- **0**: (default value) The offline model does not run on virtual devices generated by the Ascend virtualization instance feature.
- **1**: The offline model runs on virtual devices with different computing power.

**Configuration example:**

```c++
{ge::ir_option::VIRTUAL_TYPE, "1"}
```

**Constraints:**

1. When the parameter value is **1** and model conversion is performed, the number of logical AI Core cores participating in computation in the converted offline model may be greater than the actual **aicore\_num** core count, being the least common multiple of the configurable range supported by **aicore\_num**:

    For example **aicore\_num**, the supported configuration range is \{1,2,4,8\}. For the offline model converted with the parameter value set to 1, the number of NPU running cores may be 8.

2. When the parameter value is set to 1, if the converted model contains the following operators, a single core is used by default. In this scenario, the model inference performance of the converted model degrades.
    - DynamicRNN
    - PadV2D
    - SquareSumV2
    - DynamicRNNV2
    - DynamicRNNV3
    - DynamicGRUV

**Applicable products:**

<!-- npu="950" id37 -->
- Ascend 950PR&950DT series products: Supported
<!-- end id37 -->
<!-- npu="A3" id38 -->
- Atlas A3 series products: Supported
<!-- end id38 -->
<!-- npu="910b" id39 -->
- Atlas A2 series products: Supported
<!-- end id39 -->
<!-- npu="310b" id40 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id40 -->
<!-- npu="310p" id41 -->
- Atlas inference series products: Supported
<!-- end id41 -->
<!-- npu="910" id42 -->
- Atlas training series products: Supported
<!-- end id42 -->

<!-- end id44 -->
