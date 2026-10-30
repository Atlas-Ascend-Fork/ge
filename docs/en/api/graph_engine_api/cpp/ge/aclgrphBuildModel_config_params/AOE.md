# AOE

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-16T11:46:03.860Z pushedAt=2026-10-28T07:40:22.204Z nl=2 -->

## MDL\_BANK\_PATH

Path for loading the custom repository after subgraph tuning. The actual options parameter corresponding to this parameter is `ge.mdl_bank_path`.

This parameter must be used together with the **BUFFER\_OPTIMIZE** parameter in [precision comparison](../aclgrphBuildInitialize_config_params/precision_comparison.md). It takes effect only when the data cache optimization switch is enabled, and improves performance by temporarily storing data in the high-speed cache.

**Parameter value**: Path of the custom repository after model tuning.

**Parameter value format**: Supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), and periods (.).

**Default value**: $HOME/Ascend/latest/data/aoe/custom/graph/_<soc\_version\>_

**Configuration example:**

```c++
{ge::ir_option::MDL_BANK_PATH, "$HOME/custom_module_path"}
```

**Constraints:**

Priority of custom repository paths loaded after subgraph tuning: path loaded via the **MDL_BANK_PATH** parameter \> path set by the **TUNE_BANK_PATH** environment variable \> default custom repository path after subgraph tuning.

1. If the custom repository path for subgraph tuning is specified through the **TUNE\_BANK\_PATH** environment variable before model compilation, and the custom repository path is also loaded through the **MDL\_BANK\_PATH** parameter during model compilation, the path loaded through the **MDL\_BANK\_PATH** parameter takes precedence in this scenario, and the path set by the **TUNE\_BANK\_PATH** environment variable does not take effect.
2. If neither the path specified by the **MDL\_BANK\_PATH** parameter nor the path specified by the environment variable takes effect, or if no custom repository is available, the default custom repository path is used.
3. If no custom repository is available in any of the above paths, the built-in repository for subgraph tuning is searched for. The path is **`${INSTALL_DIR}/<arch>-linux/data/fusion_strategy/built-in`**, where **<arch\>** indicates the specific operating system architecture.

**Applicable products:**

<!-- npu="950" id1 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3 products: Supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 products: Supported
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
<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/AOE_res.md#id1 -->

## OP\_BANK\_PATH

Custom repository path after operator tuning. The **options** parameter that actually corresponds to this parameter is `ge.op_bank_path`.

**Parameter value format**: Supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), and periods (.).

**Default value**: $\{HOME\}/Ascend/latest/data/aoe/custom/op

**Configuration example:**

```c++
{ge::ir_option::OP_BANK_PATH, "$HOME/custom_tune_path"}
```

**Constraints:**

Priority for loading the custom repository path after operator tuning: **TUNE\_BANK\_PATH** environment variable path \> path loaded via the **OP_BANK_PATH** parameter \> default custom repository path after operator tuning.

1. If the custom repository path for operator tuning is specified through the **TUNE\_BANK\_PATH** environment variable before model conversion, and the custom repository path is also loaded through the **OP\_BANK\_PATH** parameter during model compilation, the path set by the **TUNE\_BANK\_PATH** environment variable takes precedence in this scenario, and the path loaded by the **OP\_BANK\_PATH** parameter does not take effect.
2. When neither the **OP\_BANK\_PATH** parameter nor the environment variable path takes effect, the default custom repository path is used.
3. If no usable custom repository is available in any of the above paths, the built-in operator tuning repository is searched.

**Applicable products:**

<!-- npu="950" id8 -->
- Ascend 950PR&950DT products: not supported
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
<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/AOE_res.md#id2 -->
