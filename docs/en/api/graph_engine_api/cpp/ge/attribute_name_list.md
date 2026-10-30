# Attribute Name List

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-16T11:14:38.569Z pushedAt=2026-10-28T07:40:22.139Z nl=2 -->

The attribute names listed in this section are those that can be set through the [SetAttr](./GNode/SetAttr.md) API. All of them are operator-level, and the header file is located in **include/ge/ge\_api\_types.h** under the file storage path after CANN software installation.

## \_enable\_inner\_parallel

Whether operators within **\_user\_stream\_label** are allowed to be distributed according to the original GE concurrency strategy.

**Parameter value:**

- **true**: Operators within **\_user\_stream\_label** are distributed according to the original GE concurrency strategy.
- **false**: All operators within **\_user\_stream\_label** no longer distinguish the original concurrency strategy and are merged into the same compute stream for execution.

**Constraints:**

This attribute must be used together with **\_user\_stream\_label**. When the **\_enable\_inner\_parallel** attribute is true, operators within **\_user\_stream\_label** are still distributed according to the original GE concurrency strategy (for example, CMO operators are distributed separately).

**Applicable products:**

Supported on all chips.

<!-- npu="950,A3,910b" id1 -->
## \_op\_aicore\_num

Configures the number of Cube Cores in the AI Core used during operator compilation.

Parameter value: integer type. The integer must be greater than or equal to 0 and less than or equal to the maximum number of Cube Cores contained in the AI processor. Configuring it to 0 does not report an error, but it is treated as unconfigured and does not produce a core control effect.

The maximum number of Cube Cores contained in different AI processors can be viewed from the `${INSTALL_DIR}/<arch>-linux/data/platform_config/xxx.ini` file, as shown below, which indicates that there are 24 Cube Cores on the AI processor.

```ini
[SoCInfo]
# The parameter is configured to the default value, which is the maximum value.
ai_core_cnt=24
cube_core_cnt=24
vector_core_cnt=48
```

Here, **$\{INSTALL\_DIR\}** must be replaced with the file storage path after CANN software installation. For example, when installing as the root user, the default file storage path after installation is **/usr/local/Ascend/cann**. **<arch\>** indicates the specific operating system architecture. Select xxx based on the actual product.

**Applicable products:**

- Ascend 950PR&950DT products: Supported
- Atlas A3 products: Supported
- Atlas A2 products: Supported
- Atlas inference products: not supported
- Atlas training products: not supported
- Atlas 200I/500 A2 inference products: not supported
<!-- end id1 -->

<!-- npu="950,A3,910b" id2 -->
## \_op\_vectorcore\_num

Configures the number of Vector Cores in the AI Core used during operator compilation.

Parameter value: integer type. The integer must be greater than or equal to 0 and less than or equal to the maximum number of Vector Cores contained in the AI processor. Configuring it to `0` does not report an error, but it is treated as unconfigured and produces no core control effect.

The maximum number of Vector Cores contained in different AI processors can be viewed from the `${INSTALL_DIR}/<arch>-linux/data/platform_config/xxx.ini` file, as shown below, which indicates that there are 48 Vector Cores on the AI processor.

```ini
[SoCInfo]
# The parameter is configured to the default value, which is the maximum value.
ai_core_cnt=24
cube_core_cnt=24
vector_core_cnt=48
```

Here, **$\{INSTALL\_DIR\}** must be replaced with the file storage path after CANN software installation. Taking the root user installation as an example, the default file storage path after installation is **/usr/local/Ascend/cann**. **<arch\>** indicates the specific operating system architecture, and xxx must be selected based on the actual product.

**Applicable products:**

- Ascend 950PR&950DT products: Supported
- Atlas A3 products: Supported
- Atlas A2 products: Supported
- Atlas inference products: Not supported
- Atlas training products: Not supported
- Atlas 200I/500 A2 inference product: Not supported
<!-- end id2 -->

## \_op\_exec\_never\_timeout

Sets whether AI Core operator execution on a static graph is subject to a timeout limit.

**Parameter value:**

- **true**: Operator execution is not subject to a timeout limit and never times out.
- **false**: Operator execution is subject to a timeout limit.

You can set the operator execution timeout through the "Execution Control \> **aclrtSetOpExecuteTimeOutV2**" API in *Runtime API*.

**Constraints:**

If the AI Core operator has enabled the Tiling sinking feature, that is, the **ge.tiling\_schedule\_optimize** parameter in [Performance Tuning](./options_params/precision_tuning.md) is enabled, such operators do not support setting the never-timeout attribute.

**Applicable products:**

Supported on all chips.

## \_user\_stream\_label

String type. Specifies the label of the target stream for operator execution (the label of the stream to switch to). The same label represents the same stream, and it is controlled by the user.

During graph execution, if the "**in-graph multi-stream expression feature**" needs to be enabled, you can use **\_user\_stream\_label** and **\_user\_stream\_priority** to specify that multiple operators in the graph are dispatched to different streams for parallel computation, thereby improving resource utilization.

**Applicable products:**

Supported on all chips.

## \_user\_stream\_priority

int type, indicating the priority of switching to the **\_user\_stream\_label** stream, that is, the Runtime preferentially allocates core resources to higher-priority streams during concurrent execution. This parameter is reserved in the current version, and the default value 0 is recommended.

If the "**in-graph multi-stream expression feature**" needs to be enabled during graph execution, **\_user\_stream\_label** and **\_user\_stream\_priority** can be used to specify that multiple operators in the graph are dispatched to different streams for parallel computation, improving resource utilization.

**Applicable products:**

Supported on all chips.
