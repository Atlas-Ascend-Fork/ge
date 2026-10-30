# --mdl\_bank\_path

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T03:18:08.341Z pushedAt=2026-10-26T04:00:26.316Z -->

## Product Support

<!-- npu="950" id7 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id7 -->
<!-- npu="A3" id6 -->
- Atlas A3 products: Supported
<!-- end id6 -->
<!-- npu="910b" id5 -->
- Atlas A2 products: Supported
<!-- end id5 -->
<!-- npu="310b" id4 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id4 -->
<!-- npu="310p" id3 -->
- Atlas inference products: Supported
<!-- end id3 -->
<!-- npu="910" id2 -->
- Atlas training products: Supported
<!-- end id2 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--mdl_bank_path_res.md#id1 -->

## Feature Description

Loads the path of the custom knowledge base after subgraph tuning.

For details about subgraph tuning, see [*AOE Tuning Tool*](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/920/devaids/aoe/auxiliarydevtool_aoe_0001.html).

## Related Parameters

This parameter must be used together with the [--buffer\_optimize](--buffer_optimize.md) parameter and takes effect only when the data cache optimization switch is enabled. It improves performance by using the cache [--buffer\_optimize](--buffer_optimize.md) to temporarily store data.

## Parameter Value

**Parameter value:**
Path of the custom knowledge base after subgraph tuning.

**Parameter value format:**
Supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), and periods (.).

**Default value:**
$HOME/Ascend/latest/data/aoe/custom/graph/_<soc\_version\>_

## Recommended Configuration and Benefits

None.

## Example

For example, if the path of the custom knowledge base after subgraph tuning is $HOME/custom\_module\_bank, the usage example is as follows:

```bash
atc --mdl_bank_path=$HOME/custom_module_path   --buffer_optimize=l2_optimize ...
```

## Dependencies and Constraints

Priority of the custom knowledge base path after subgraph tuning: [--mdl\_bank\_path](--mdl_bank_path.md) parameter loading path \>**TUNE\_BANK\_PATH** environment variable path \> default custom knowledge base path after subgraph tuning.

1. If the custom knowledge base path for subgraph tuning is specified through the **TUNE\_BANK\_PATH** environment variable before model conversion, and the custom knowledge base path is also loaded through the [--mdl\_bank\_path](--mdl_bank_path.md) parameter during model conversion, the path loaded through the [--mdl\_bank\_path](--mdl_bank_path.md) parameter takes effect, and the path set through the **TUNE\_BANK\_PATH** environment variable does not take effect.
2. If neither the path specified by the [--mdl\_bank\_path](--mdl_bank_path.md) parameter nor the path specified by the environment variable takes effect, or no custom knowledge base is available, the default custom knowledge base path is used.
3. If no custom knowledge base is available in any of the preceding paths, the ATC tool searches for the built-in knowledge base for subgraph tuning. The path is `${INSTALL_DIR}/<arch>-linux/data/fusion_strategy/built-in`, where <arch\> indicates the specific operating system architecture.
