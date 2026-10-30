# --customize\_dtypes

<!-- md-trans-meta sourceCommit=73cc22fa193c049e7877714b651cc02a774ef134 translatedAt=2026-10-22T02:52:22.181Z pushedAt=2026-10-23T08:51:22.241Z -->

## Product Support

Supported on all chips

## Function Description

Customizes the computation precision of one or more operators during model compilation.

## Related Parameters

- If this parameter is used together with [--precision\_mode](--precision_mode.md) or [--precision\_mode\_v2](--precision_mode_v2.md), operators other than those specified by this parameter are compiled according to the precision mode configured by [--precision\_mode](--precision_mode.md) or [--precision\_mode\_v2](--precision_mode_v2.md).
- Both [--customize\_dtypes](--customize_dtypes.md) and [--keep\_dtype](--keep_dtype.md) are used to set the computation precision of operators. For scenarios where the model inference precision needs to be improved, it is recommended to first use [--keep\_dtype](--keep_dtype.md) to retain the original graph precision. If the precision still cannot be improved, you can try using [--customize\_dtypes](--customize_dtypes.md) to customize the computation precision of one or more operators.

    Note, however, that when [--customize\_dtypes](--customize_dtypes.md) is used and operators are configured by operator name, the operator names may change due to fusion, splitting, and other operations during internal model optimization, which may cause the configuration to fail to take effect and thus fail to improve the precision. In this case, you can obtain logs to locate the problem. For details about logs, see *Log Reference*.

- If both [--customize\_dtypes](--customize_dtypes.md) and [--keep\_dtype](--keep_dtype.md) are used, the precision set by [--customize\_dtypes](--customize_dtypes.md) takes effect.

## Parameter Values

**Parameter value:**
Path and file name of the operator configuration file. The configuration file lists the operator names or operator types for which the computation precision needs to be customized, with one operator per line.

**Parameter value constraints:**

- If the operator name is used, configure it in the **Opname::InputDtype:dtype1,...,OutputDtype:dtype1,...** format, with one Opname per line. dtype1, dtype2, ... must correspond one-to-one to the operator inputs and outputs for which the computation precision can be set **.**
- If the operator type is used, configure it in the **OpType::TypeName:InputDtype:dtype1,...,OutputDtype:dtype1,...** format, with one OpType per line. dtype1, dtype2, ... must correspond one-to-one to the operator inputs and outputs for which the computation precision can be set. In addition, the operator OpType must be the OpType of an operator defined based on Ascend IR. For details about how to view the operator type, see [How Do I Determine the Correspondence Between Operators in the Original Framework Network Model and Operators Supported by the AI Processor](../FAQ/operator_correspondence_guide.md).
- For the same operator, if both the **Opname** and **OpType** configuration items are configured, the **Opname** configuration item takes effect during compilation.
- When this parameter is used to specify the computation precision of an operator, if the operator is fused during model conversion, the specified computation precision of the operator does not take effect.

**Parameter value format:**
Path and file name: supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), English colons \(:\), and Chinese characters.

## Recommended Configuration and Benefits

None.

## Example

- If the configuration file contains operator names, the configuration example is as follows (the file name is _customize\_dtypes.cfg_ as an example):

    ```text
    Opname1::InputDtype:dtype1,dtype2,...,OutputDtype:dtype1,...
    Opname2::InputDtype:dtype1,dtype2,...,OutputDtype:dtype1,...
    ```

- If the configuration file contains operator types, the configuration example is as follows (the file name is _customize\_dtypes.cfg_ as an example):

    ```text
    OpType::TypeName1:InputDtype:dtype1,dtype2,...,OutputDtype:dtype1,...
    OpType::TypeName2:InputDtype:dtype1,dtype2,...,OutputDtype:dtype1,...
    ```

The computation precision supported by an operator can be found in the "Ascend IR Operator Specification" section of the *[Operator Library](https://hiascend.com/document/redirect/CannCommunityOplist)*.

Take the Relu operator in the TensorFlow ResNet50 network model as an example. Its corresponding operator type defined by Ascend IR is Relu, and this operator has only one input and one output. The configuration example is as follows:

- Operator name configuration example:

    ```text
    fp32_vars/Relu::InputDtype:float16,OutputDtype:int8
    ```

- Operator type configuration example:

    ```text
    OpType::Relu:InputDtype:float16,OutputDtype:int8
    ```

Upload the configured _customize\_dtypes.cfg_ file to any directory on the server where the ATC tool is located, for example, $HOME. The usage example is as follows:

```basj
atc --customize_dtypes=$HOME/customize_dtypes.cfg --precision_mode=force_fp16 ...
```

During model compilation, the operators in the _customize\_dtypes.cfg_ file use the specified computation precision, while the other operators in the network model are compiled using the precision mode specified by the [--precision\_mode](--precision_mode.md) or [--precision\_mode\_v2](--precision_mode_v2.md) parameter.

## Usage Constraints

1. When this parameter is used to specify the computation precision of operators, because its priority is higher than that of the [--precision\_mode](--precision_mode.md) or [--precision\_mode\_v2](--precision_mode_v2.md) and [--keep\_dtype](--keep_dtype.md) parameters, it may cause a decrease in subsequent inference precision or performance.
2. When this parameter is used to specify the computation precision of operators, if the specified precision is not supported by the operator itself, model compilation will fail.
