# Deprecated Configuration for Future Versions

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-16T11:31:44.168Z pushedAt=2026-10-28T07:40:22.189Z nl=2 -->

## OP\_SELECT\_IMPL\_MODE

**The functionality of this parameter has stopped evolving and will be deprecated in future versions.**

Some built-in operators of the AI processor have high-precision and high-performance implementation modes. You can use this parameter to configure which implementation mode the operator uses during model compilation.

High precision means that, in float16 input scenarios, the operator precision is further improved through methods such as Taylor expansion and Newton iteration. High performance means the optimal performance implementation that does not affect network precision when the input is float16.

**Parameter values:**

- **high\_precision**: Indicates that operators use the high-precision implementation mode.

    This option uses the system built-in configuration file to set the operator implementation mode. The built-in configuration file path is `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/impl_mode/high_precision.ini`.

    To maintain compatibility, this parameter takes effect only for the operator list in the high\_precision.ini file. Through this list, you can control the scope of operators that take effect and ensure that network models of previous versions are not affected.

- **high\_performance**: (Default value) Indicates that operators use the high-performance implementation mode.

    This option uses the system built-in configuration file to set the operator implementation mode. The built-in configuration file path is `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/impl_mode/high_performance.ini`.

    To maintain compatibility, this parameter takes effect only on the operator list in the high\_performance.ini file. This list controls the scope of operators affected and ensures that network models from previous versions are not affected.

- **high\_precision\_for\_all**: Indicates that operators use the high precision implementation mode.

    This option uses the system built-in configuration file to set the operator implementation mode. The built-in configuration file path is `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/impl_mode/high_precision_for_all.ini`, and the list in this file may be updated in future versions.

    **This implementation mode does not guarantee compatibility.** If an operator adds a new implementation mode in a future software package (that is, a new implementation mode for an operator is added to the configuration file), network models that used high\_precision\_for\_all in previous versions may experience performance degradation in the new version.

- **high\_performance\_for\_all**: Indicates that operators use the high performance implementation mode.

    This option uses the system built-in configuration file to set the operator implementation mode. The built-in configuration file path is `${INSTALL_DIR}/opp/built-in/op_impl/ai_core/tbe/impl_mode/high_performance_for_all.ini`, and the list in this file may be updated in future versions.

    **This implementation mode does not guarantee compatibility.** If an operator adds a new implementation mode in a future software package (that is, a new implementation mode for an operator is added to the configuration file), network models that used high\_performance\_for\_all in previous versions may experience precision degradation in the new version.

The implementation modes above are distinguished by the dtype of the operator. Replace **$\{INSTALL\_DIR\}** with the file storage path after CANN software installation. For example, when installing as the root user, the default file storage path after installation is **/usr/local/Ascend/cann**.

**Default value:**
high\_performance

**Configuration example:**

```c++
{ge::ir_option::OP_SELECT_IMPL_MODE, "high_performance"}
```

## OPTYPELIST\_FOR\_IMPLMODE

**This parameter is no longer evolving and will be deprecated in future versions. Do not use it.**

Sets the implementation mode of the operators in the optype list.

**Parameter value constraints:**

- The operators in the list use the mode specified by the **OP\_SELECT\_IMPL\_MODE** parameter. Currently, only **high\_precision** and **high\_performance** are supported. Multiple operators are separated by commas.
- This parameter must be used together with the **OP\_SELECT\_IMPL\_MODE** parameter, and it takes effect only on the specified operators. Operators that are not specified use the default implementation mode. For example, **OP\_SELECT\_IMPL\_MODE** is set to **high\_precision**, and **OPTYPELIST\_FOR\_IMPLMODE** is set to Pooling and SoftmaxV2. This means that the unified high precision mode is used for the Pooling and SoftmaxV2 operators, while unspecified operators use their default implementation mode.

**Configuration example:**

```c++
{ge::ir_option::OPTYPELIST_FOR_IMPLMODE, "Pooling,SoftmaxV2"}
```
