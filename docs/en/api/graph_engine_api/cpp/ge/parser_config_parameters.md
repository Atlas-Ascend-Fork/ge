# Configuration Parameters Supported by the Parser Parsing APIs

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-16T11:24:19.450Z pushedAt=2026-10-28T07:40:22.174Z nl=2 -->

**Applicable Products: The following parameters are supported by all chips.**

## INPUT\_FP16\_NODES

Specifies the name of the input node whose input data type is FP16.

For example: "node\_name1;node\_name2", the specified node must be placed in double quotes, and multiple nodes are separated by English semicolons.

**Configuration example:**

```c++
{ge::AscendString(ge::ir_option::INPUT_FP16_NODES), ge::AscendString("input1;input2")},
```

## IS\_INPUT\_ADJUST\_HW\_LAYOUT

Specifies whether the network input data type is FP16 and whether the data format is NC1HWC0.

This parameter must be used together with **INPUT\_FP16\_NODES**. If the **IS\_INPUT\_ADJUST\_HW\_LAYOUT** parameter is set to true, the input data type of the corresponding **INPUT\_FP16\_NODES** node is FP16, and the input data format is **NC1HWC0**.

**Parameter value**: The value range is **false** or **true**, and the default value is **false**.

**Configuration example:**

```c++
{ge::AscendString(ge::ir_option::INPUT_FP16_NODES), ge::AscendString("input1;input2")},
{ge::AscendString(ge::ir_option::IS_INPUT_ADJUST_HW_LAYOUT), ge::AscendString("true,true")},
```

## OUTPUT

Specifies the name of the graph after conversion.

**Configuration example:**

```c++
{ge::AscendString(ge::ir_option::OUTPUT), ge::AscendString("newIssue")},
```

## IS\_OUTPUT\_ADJUST\_HW\_LAYOUT

Specifies whether the data type of the network output is FP16 and whether the data format is NC1HWC0.

This parameter must be used together with **OUT\_NODES**.

If the **IS\_OUTPUT\_ADJUST\_HW\_LAYOUT** parameter is set to true, the output data type of the output node in the corresponding **OUT\_NODES** is FP16, and the data format is NC1HWC0.

**Parameter value**: **false** or **true**, defaulting to **false**.

**Configuration example:**

```c++
{ge::AscendString(ge::ir_option::OUT_NODES), ge::AscendString("add_input:0")},
{ge::AscendString(ge::ir_option::IS_OUTPUT_ADJUST_HW_LAYOUT), ge::AscendString("true")},
```

## OUT\_NODES

Specifies the output node (operator) of a layer as the output of the network model, or specifies the name of the network model output.

If no output node (operator name) is specified, the model output defaults to the operator information of the last layer. If specified, the specified one takes precedence.

In some cases, if users want to check whether the parameters of a layer's operator are appropriate, they need to output the parameters of that layer's operator. This can be done by using this parameter to specify the output of a layer's operator during model compilation. After model compilation, the parameter information of the specified output operator can be viewed at the end of the corresponding .om model file. If it cannot be viewed through the .om model file, the .om model file can be converted to JSON format for viewing.

Three formats are supported:

- **Format 1**: "node\_name1:0;node\_name1:1;node\_name2:0".

    The name of a node **(node\_name)** in the network model. The specified output node must be placed in double quotes, and multiple nodes are separated by English semicolons. **node\_name** must be the node name in the network model before compilation. The number after the colon indicates which output it is. For example, **node\_name1:0** indicates the first output of the node named **node\_name1**.

    **Configuration example:**

    ```c++
    {ge::AscendString(ge::ir_option::OUT_NODES), ge::AscendString("add_input:0")},
    ```

- **Format 2**: "topname1;topname2". \( is supported only by **CAFFE**\)

    The **topname** of a layer's output node. The specified output node must be placed in double quotes, and multiple nodes are separated by English semicolons. The **topname** must be a top name of a layer in the caffe network model before compilation. If several layers have the same **topname**, the last layer takes precedence.

    **Configuration example:**

    ```c++
    {ge::AscendString(ge::ir_option::OUT_NODES), ge::AscendString("res5c_branch2c")},
    ```

- **Format 3**: "output1;output2;output3" (only supported by **ONNX**)

    Specifies the name of the network model output (the **name** of the **output**). The specified output name must be placed in double quotes, and multiple output names are separated by English semicolons. The **output** must be an output of the network model.

    **Configuration example:**

    ```c++
    {ge::AscendString(ge::ir_option::OUT_NODES), ge::AscendString("output1")},
    ```

## ENABLE\_SCOPE\_FUSION\_PASSES

Specifies the list of fusion rules to take effect during compilation.

Fusion rules are classified as follows:

- Built-in fusion rules:

  - **General**: Scope fusion rules common to all networks. They take effect by default and do not support being disabled by the user.

  - **Non-General**: Scope fusion rules applicable to specific networks. They do not take effect by default. The user can specify the list of fusion rules to take effect through the **ENABLE\_SCOPE\_FUSION\_PASSES** parameter.

- User-defined fusion rules:

  - **General**: They take effect by default after being loaded. The function of disabling them by the user is not provided yet.

  - **Non-General**: They do not take effect by default after being loaded. The user can specify the list of fusion rules to take effect through the **ENABLE\_SCOPE\_FUSION\_PASSES** parameter.

The specified fusion rules must be placed in double quotes, and the rules are separated by English commas.

**Configuration example**:

```c++
{ge::AscendString(ge::ir_option::ENABLE_SCOPE_FUSION_PASSES), ge::AscendString("ScopePass1,ScopePass2")},
```

> [!NOTE]
> Only **aclgrphParseTensorFlow** supports this parameter.

## INPUT\_DATA\_NAMES

Specifies the mapping relationship between the **name** and **index** attributes of the input nodes in the model file. The system sets the **index** attribute of the corresponding input nodes in the order of the input **name** values.

**Configuration example**:

```c++
{ge::AscendString(ge::ir_option::INPUT_DATA_NAMES), ge::AscendString("Placeholder,Placeholder_1")},
```

## INPUT\_SHAPE

Specifies the shape information of the model input.

**Parameter value**:

- Static shape.
  - If the model has a single input, the shape information is **"input\_name:n,c,h,w"**; the specified node must be placed in double quotes.
  - If the model has multiple inputs, the shape information is **"input\_name1:n1,c1,h1,w1;input\_name2:n2,c2,h2,w2"**; different inputs are separated by English semicolons, and input\_name must be the node name in the network model before conversion.

- If one or more dimension values of the input data in the original model are not fixed, the model can currently be converted by setting the shape range.
  - Set the shape range.

    <!-- npu="310b" id1 -->
    Atlas 200I/500 A2 inference products **do not support setting the shape range**.
    <!-- end id1 -->

    When setting the **INPUT\_SHAPE** parameter, the value of the corresponding dimension can be set to a range.

    - Setting by **name** is supported: "input\_name1:n1,c1,h1,w1;input\_name2:n2,c2,h2,w2", for example: "input\_name1:8\~20,3,5,-1;input\_name2:5,3\~9,10,-1". The specified node must be placed in double quotes, and use English semicolons to separate nodes. input\_name must be the node name in the network model before conversion. If the user knows the name of the data node, setting by name is recommended.
    - Setting by index is supported: "n1,c1,h1,w1;n2,c2,h2,w2", for example: "8\~20,3,5,-1;5,3\~9,10,-1". The node name can be omitted, and nodes are arranged in index order, with English semicolons used to separate between nodes. When setting the shape range by index, the data node must have the index attribute set to indicate which input it is, with the index starting from 0.

        If the user does not want to specify the range or specific value of a dimension, they can set it to -1, indicating that this dimension can use any value of \>=0. In this scenario, the upper limit of the value is the expression range of the int64 data type, but it is limited by the physical memory size on the host and device. The user can support larger values by increasing the memory.

- The shape is scalar.
  - Non-dynamic shape segmentation scenario:

    For an input whose shape is scalar, the configuration is optional. For example, a model has two inputs, input\_name1 is scalar, that is, the shape is in the form of "\[\]", and the input shape of input\_name2 is \[n2,c2,h2,w2\], then the shape information is "input\_name1:;input\_name2:n2,c2,h2,w2". The specified node must be placed in double quotes, and use English semicolons to separate different inputs. input\_name must be the node name in the network model before conversion. If a scalar input is configured, leave it empty.

**Configuration example:**

- Static shape, for example, the input shape information of a network, input 1: **input\_0\_0 \[16,32,208,208\]**, input 2: **input\_1\_0 \[16,64,208,208\]**, then the configuration information of **INPUT\_SHAPE** is:

    ```c++
    {ge::AscendString(ge::ir_option::INPUT_SHAPE), ge::AscendString("input_0_0:16,32,208,208;input_1_0:16,64,208,208")},
    ```

- Example of setting the shape range:

    ```c++
    {ge::AscendString(ge::ir_option::INPUT_SHAPE), ge::AscendString("input_0_0:1~10,32,208,208;input_1_0:16,64,100~208,100~208")},
    ```

- **shape** is scalar
  - Non-dynamic shape scenario

    For an input whose shape is scalar, this is an optional configuration. For example, a model has two inputs, **input\_name1** is scalar, and the input shape of input\_name2 is \[16,32,208,208\]. The configuration example is:

    ```c++
    {ge::AscendString(ge::ir_option::INPUT_SHAPE), ge::AscendString("input_name1:;input_name2:16,32,208,208")},
    ```

    In the preceding example, **input\_name1** is an optional configuration.

> [!NOTE]
>
>**INPUT\_SHAPE** is an optional setting. If it is not set, the system directly reads the **shape** information of the corresponding Data node. If it is set, the value set here takes precedence, and the **shape** information of the corresponding Data node is refreshed accordingly.
