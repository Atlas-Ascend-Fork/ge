# --keep\_dtype

<!-- md-trans-meta sourceCommit=73cc22fa193c049e7877714b651cc02a774ef134 translatedAt=2026-10-22T03:17:10.836Z pushedAt=2026-10-26T03:02:16.599Z -->

## Product Support

Supported on all chips

## Function Description

Specifies, through a configuration file, that the data types of specific operators in the original model remain unchanged during model compilation.

In inference scenarios, when the [--precision\_mode](--precision_mode.md) or [--precision\_mode\_v2](--precision_mode_v2.md) parameter is used to set the precision mode of the entire network model, individual operators may have performance or precision issues. To address this, the [--keep\_dtype](--keep_dtype.md) parameter is introduced to keep the computation precision of individual operators unchanged when compiling the original network model. If the computation precision of an operator in the original network model is not supported on the AI processor, the system automatically uses a higher precision supported by the operator for computation.

## Related Parameters

- This parameter must be used together with the [--precision\_mode](--precision_mode.md) or [--precision\_mode\_v2](--precision_mode_v2.md) parameter. However, when [--precision\_mode](--precision_mode.md) is set to must\_keep\_origin\_dtype or [--precision\_mode\_v2](--precision_mode_v2.md) is set to origin, the --keep\_dtype parameter does not take effect.
- Both the [--customize\_dtypes](--customize_dtypes.md) parameter and the [--keep\_dtype](--keep_dtype.md) parameter are used to set the computation precision of operators. In scenarios where the model inference precision needs to be improved, it is recommended to first use the [--keep\_dtype](--keep_dtype.md) parameter to retain the original graph precision. If the precision still cannot be improved, you can try using the [--customize\_dtypes](--customize_dtypes.md) parameter to customize the computation precision of one or more operators.

    Note, however, that when the [--customize\_dtypes](--customize_dtypes.md) parameter is used and operators are configured by name, the operator names may change due to fusion, splitting, and other operations during internal model optimization, which may cause the configuration to fail to take effect and thus fail to improve precision. In this case, you can obtain logs to locate the problem. For details about logs, see *[Log Reference](https://gitcode.com/cann/runtime/blob/9.2.0/docs/zh/log_ref/README.md)*.

- If both the [--customize\_dtypes](--customize_dtypes.md) parameter and the [--keep\_dtype](--keep_dtype.md) parameter are used, the precision set by the [--customize\_dtypes](--customize_dtypes.md) parameter prevails.

## Parameter Values

**Parameter value:**
Path and file name of the operator configuration file. The configuration file lists the operator names or operator types whose computation precision must be retained, with each operator on a separate line.

**Parameter value constraints:**
If the value is an operator type, configure it in the **OpType::TypeName** format, with each OpType on a separate line. The operator OpType must be the OpType of an operator defined based on Ascend IR. For details about how to view operator types, see [How to Determine the Correspondence Between Operators in the Original Framework Network Model and Operators Supported by the AI Processor](../FAQ/operator_correspondence_guide.md).

**Parameter value format:**
Path and file name: supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

## Recommended Configuration and Benefits

None.

## Example

- If the configuration file contains operator names, the configuration example is as follows (the file name is _exceptionlist.cfg_ as an example):

    ```text
    Opname1
    Opname2
    …
    ```

- If the configuration file contains operator types, the configuration example is as follows (the file name is _exceptionlist.cfg_ as an example):

    ```text
    OpType::TypeName1
    OpType::TypeName2
    …
    ```

Take the Relu operator in the TensorFlow ResNet50 network model as an example. The operator type defined by its corresponding Ascend IR is Relu. The configuration example is as follows:

```text
#Operator name configuration example:
fp32_vars/Relu
#Operator type configuration example:
OpType::Relu
```

Upload the configured _exceptionlist.cfg_ file to any directory on the server where the ATC tool is located, for example, to $HOME. The usage example is as follows:

```bash
atc --keep_dtype=$HOME/exceptionlist.cfg --precision_mode=force_fp16 ...
```

During model compilation, the operators in the _exceptionlist.cfg_ file retain the precision of the original network model, that is, the precision does not change. The operators in the remaining network models are compiled in the precision mode specified by the [--precision\_mode](--precision_mode.md) or [--precision\_mode\_v2](--precision_mode_v2.md) parameter.

## Dependency Constraints

None.
