# --op\_select\_implmode

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T03:31:01.775Z pushedAt=2026-10-27T11:53:33.181Z -->

## Feature Overview

**This parameter is no longer evolving and will be deprecated in later versions. You are advised to use the [--op\_precision\_mode](--op_precision_mode.md) parameter instead.**

Some built-in operators on AI processors have both high-precision and high-performance implementation modes. You can use this parameter to configure which implementation mode is selected for operators during model compilation.

High precision means that in float16 input scenarios, the operator precision is further improved through methods such as Taylor expansion and Newton iteration. High performance means the optimal performance implementation that does not affect network precision in float16 input scenarios.

## Associated Parameters

None.

## Parameter Values

- **high\_precision**: indicates that operators use the high-precision implementation mode.

    This option uses the system built-in configuration file to set the operator implementation mode. The built-in configuration file path is `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/impl_mode/high_precision.ini`.

    For compatibility, this parameter takes effect only on the operator list in the high\_precision.ini file. This list controls the scope of operators that take effect and ensures that network models of earlier versions are not affected.

- **high\_performance**: (default value) indicates that operators use the high-performance implementation mode.

    This option uses the system built-in configuration file to set the operator implementation mode. The built-in configuration file path is `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/impl_mode/high_performance.ini`.

    For compatibility, this parameter takes effect only on the operator list in the high\_performance.ini file. This list controls the scope of operators that take effect and ensures that network models of earlier versions are not affected.

- **high\_precision\_for\_all**: indicates that operators use the high-precision implementation mode.

    This option uses the system built-in configuration file to set the operator implementation mode. The built-in configuration file path is `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/impl_mode/high_precision_for_all.ini`. The list in this file may be updated in later versions.

    **This implementation mode does not guarantee compatibility.** If an operator adds an implementation mode in a later software package (that is, an implementation mode of an operator is added to the configuration file), the performance of network models that use high\_precision\_for\_all in earlier versions may degrade in the new version.

- **high\_performance\_for\_all**: Indicates that the operator uses the high-performance implementation mode.

    This option uses the system built-in configuration file to set the operator implementation mode. The built-in configuration file path is `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/impl_mode/high_performance_for_all.ini`, and the list in this file may be updated in later versions.

    **This implementation mode does not guarantee compatibility.** If an operator adds an implementation mode in a later software package (that is, an implementation mode for an operator is added to the configuration file), the precision of a network model that uses high\_performance\_for\_all in an earlier version may decrease in the new version.

The preceding implementation modes are distinguished by the dtype of the operator. $\{INSTALL\_DIR\} must be replaced with the file storage path after CANN software installation. For example, when installed as the root user, the default file storage path is /usr/local/Ascend/cann.

## Recommended Configuration and Benefits

**It is not recommended that you use the --op\_select\_implmode parameter to set the implementation mode of operators. This parameter is intended for debugging only.** It is recommended that you set the operator precision mode by loading an ini configuration file through the [--op\_precision\_mode](--op_precision_mode.md) parameter:

- If you have higher performance requirements, it is recommended that you use the **high\_performance\_for\_all** parameter first. If the performance is verified to meet the requirements, copy the high\_performance\_for\_all.ini file, rename it to "**network model**.ini", and use it together with the network. Different network models use different ini files. During subsequent model conversion, you can directly use the [--op\_precision\_mode](--op_precision_mode.md) parameter to load the saved "**network model**.ini" configuration file.
- If you have higher precision requirements, it is recommended that you use the **high\_precision\_for\_all** parameter first. If the precision is verified to meet the requirements, copy the high\_precision\_for\_all.ini file, rename it to "**network model**.ini", and use it together with the network. Different network models use different ini files. During subsequent model conversion, you can directly use the [--op\_precision\_mode](--op_precision_mode.md) parameter to load the saved _"**network model**.ini"_ configuration file.
- If, when using **high\_performance\_for\_all**, the performance is greatly improved but the precision does not meet the requirements, and you find that this is caused by the xxx operator using the high-performance mode, copy the high\_performance\_for\_all.ini file, rename it to "**network model**.ini", and change the implementation mode of the xxx operator in the file to the high-precision mode. During subsequent model conversion, directly use the [--op\_precision\_mode](--op_precision_mode.md) parameter to load the "**network model**.ini" configuration file.
- If, when using **high\_precision\_for\_all**, the precision is greatly improved but the performance degrades significantly, and you find that this is caused by the xxx operator using the high-precision mode, copy the high\_precision\_for\_all.ini file, rename it to "**network model**.ini", and change the implementation mode of the xxx operator in the file to the high-performance mode. During subsequent model conversion, directly use the [--op\_precision\_mode](--op_precision_mode.md) parameter to load the "**network model**.ini" configuration file.

The implementation modes of operators in the high\_\*.ini file are subject to those listed in the all\_ops\_impl\_mode.ini file (path: `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/impl_mode`). Implementation modes not listed in that file are not supported for configuration.

Replace $\{INSTALL\_DIR\} in the preceding path with the file storage path after the CANN software is installed. For example, if you install the software as the root user, the default storage path after installation is /usr/local/Ascend/cann.

## Example

```bash
atc --op_select_implmode=high_precision ...
```

## Usage Constraints

- If operators that newly support the precision mode also select the high-performance or high-precision mode, and you do not want to compromise the precision or performance of the existing network, you can configure them in either of the following two ways:
  - Specify the newly added specific operators through the [--optypelist\_for\_implmode](--optypelist_for_implmode.md) parameter

    ```text
    --op_select_implmode=high_precision  --optypelist_for_implmode=operator optype
    ```

  - Set the precision mode of operators through the [--op\_precision\_mode](--op_precision_mode.md) parameter

    Construct the operator precision mode configuration file _op\_precision.ini_, and set the precision mode of operators in this file. Each line sets the precision mode of one operator. The following is an example:

    ```text
    optype1=high_precision
    optype2=high_performance
    ```

    Upload the configured _op\_precision.ini_ file to any directory on the server where the ATC tool is located, for example, $HOME/conf. The following is an example:

    ```text
    --op_precision_mode=$HOME/conf/op_precision.ini
    ```

- The [--op\_select\_implmode](--op_select_implmode.md) parameter specifies the high-precision or high-performance mode for all operators in the network model. If an operator implements both high-precision and high-performance modes, the mode specified by the [--op\_select\_implmode](--op_select_implmode.md) parameter is used at runtime. If an operator implements only one mode, it runs according to its operator implementation. For example:

    If an operator currently supports only high precision, but [--op\_select\_implmode](--op_select_implmode.md) is set to high performance, the [--op\_select\_implmode](--op_select_implmode.md) parameter does not take effect for this operator, and the operator runs in its currently implemented high-precision mode.
