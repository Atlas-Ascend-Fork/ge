# Basic Functions

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-16T11:47:31.887Z pushedAt=2026-10-28T07:40:22.207Z nl=2 -->

## INPUT\_FORMAT

Input data format. The actual **options** parameter corresponding to this parameter is `input_format`.

**Parameter values:**

Supports NCHW, NHWC, ND, NCDHW, and NDHWC formats.

**Configuration example:**

```c++
{ge::ir_option::INPUT_FORMAT, "NHWC"}
```

If AIPP is enabled at the same time, the input image data must be arranged in NHWC format during inference. In this scenario, the data format specified by the **INPUT\_FORMAT** parameter does not take effect.

> [!NOTE]
> This parameter applies only to dynamic **batch\_size**, dynamic resolution, and dynamic dimension scenarios.
> In the preceding scenarios, **INPUT\_FORMAT** must be set and must be consistent with the format of all Data operators. Otherwise, model compilation fails.

**Applicable products:**

Supported on all chips.

## OP\_NAME\_MAP

Path and file name of the mapping configuration file for extended operators (non-standard operators). When an extended operator has different functions in different networks, you can specify the mapping from this extended operator to the extended operator that actually runs in a specific network. The **options** parameter that this parameter actually corresponds to is `op_name_map`.

In a Caffe network, layers that have the same type name but different computation functions, such as the **DetectionOutput** layer, require operator mapping to specify the detection operator type, such as **FSRDetectionOutput** or **SSDDetectionOutput**. Otherwise, offline model generation will fail.

This parameter applies only to the Caffe framework.

Path and file name: supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

**Configuration example**:

```c++
{ge::ir_option::OP_NAME_MAP, "/home/test/opname_map.cfg"}
```

The following is an example of the content of the extended operator mapping configuration file:

```text
OpA:Network1OpA
```

**Applicable products:**

<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/basic_functions_res.md#id1 -->

<!-- npu="950" id1 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3 products: Not supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 series products: Not supported
<!-- end id3 -->
<!-- npu="310b" id4 -->
- Atlas 200I/500 A2 inference products: Supported
<!-- end id4 -->
<!-- npu="310p" id5 -->
- Atlas inference products: Supported
<!-- end id5 -->
<!-- npu="910" id6 -->
- Atlas training products: Supported
<!-- end id6 -->

## INSERT\_OP\_FILE

Configuration file path of the input preprocessing operator, for example, the Aipp operator. For details about the parameter, see [More Features \> AIPP](../../../../../user_guides/graph_dev/more_features/AIPP.md). The actual **options** parameter corresponding to this parameter is `ge.insertOpFile`.

If this parameter is configured, the **INPUT\_FP16\_NODES** parameter cannot be used on the same input node at the same time.

Configuration file path: supports uppercase and lowercase letters, digits, and underscores; file name part: supports uppercase and lowercase letters, digits, underscores, and dots \(.\)

**Configuration example**:

```c++
{ge::ir_option::INSERT_OP_FILE, "/home/test/aipp.cfg"}
```

The following is an example of the configuration file content:

```textproto
aipp_op {
    aipp_mode:static
    input_format:YUV420SP_U8
    csc_switch:true
    var_reci_chn_0:0.00392157
    var_reci_chn_1:0.00392157
    var_reci_chn_2:0.00392157
}
```

> [!NOTE]
>
>For details about the configuration file, see *[ATC Offline Model Compilation Tool](../../../../../user_guides/atc_tools/README.md)*.

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
- Atlas 200I/500 A2 inference products: Supported
<!-- end id11 -->
<!-- npu="310p" id12 -->
- Atlas inference products: Supported
<!-- end id12 -->
<!-- npu="910" id13 -->
- Atlas training products: Supported
<!-- end id13 -->
<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/basic_functions_res.md#id2 -->

## OUTPUT\_TYPE

Output data type of the network. This parameter actually corresponds to the `ge.outputDatatype` option.

**Parameter values:**

- **FP32**: Recommended for classification and detection networks.
- **FP16**: Recommended for classification and detection networks. Typically used when the output of one network serves as the input of another network.
- **UINT8**: Recommended for image super-resolution networks for better inference performance.
- INT8
- UINT16
- INT16
- UINT32
- INT32
- UINT64
- INT64
- DOUBLE
<!-- npu="950" id15 -->
- HIF8: Only Ascend 950PR&950DT products supports this type.
- FP8E5M2: Only Ascend 950PR&950DT products supports this type.
- FP8E4M3FN: Only Ascend 950PR&950DT products supports this type.
<!-- end id15 -->

After model compilation, the above data types are presented in the corresponding \*.om model file as follows:

- DT\_FLOAT
- DT\_FLOAT16
- DT\_UINT8
- DT\_INT8
- DT\_UINT16
- DT\_INT16
- DT\_UINT32
- DT\_INT32
- DT\_UINT64
- DT\_INT64
- DT\_DOUBLE
<!-- npu="950" id16 -->
- DT\_HIFLOAT8
- DT\_FLOAT8\_E5M2
- DT\_FLOAT8\_E4M3FN
<!-- end id16 -->

**Configuration example:**

```c++
{ge::ir_option::OUTPUT_TYPE, "FP32"}
```

**Parameter value constraints:**

- If no specific data type is specified, the data type of the operator output in the last layer of the original network model is used.
- If a type is specified, the type specified by this parameter is used.

**Applicable products:**

Supported on all chips.

## INPUT\_FP16\_NODES

Specifies the names of input nodes whose input data type is FP16. The corresponding **options** parameter of this parameter is `ge.INPUT_NODES_SET_FP16`.

For example: "node\_name1;node\_name2", the specified nodes must be enclosed in double quotation marks, and nodes are separated by English semicolons. If this parameter is configured, the **INSERT\_OP\_FILE** parameter cannot be used for the same input node at the same time.

Configuration example:

```c++
{ge::ir_option::INPUT_FP16_NODES, "node_name1;node_name2"}
```

**Applicable products:**

Supported on all chips.
