# --op\_bank\_path

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T03:25:07.380Z pushedAt=2026-10-26T12:57:16.081Z -->

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

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--op_bank_path_res.md#id1 -->

## Function Description

Loads the path of the custom knowledge base after operator tuning.

For details about operator tuning, see *[AOE Tuning Tool](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/920/devaids/aoe/auxiliarydevtool_aoe_0001.html)*.

## Parameter Values

**Parameter Value:**
Path of the custom knowledge base after operator tuning.

**Parameter Value Format:**
Supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), and periods (.).

**Default Value:**
Default custom knowledge base path $HOME/Ascend/latest/data/aoe/custom/op

## Recommended Configuration and Benefits

None.

## Example

For example, if the path of the custom knowledge base after operator tuning is $HOME/custom\_tune\_bank, the usage example is as follows:

```bash
atc --op_bank_path=$HOME/custom_tune_bank ...
```

## Usage Constraints

Priority of loading the custom knowledge base path after operator tuning: **TUNE\_BANK\_PATH** path set by the environment variable \>[--op\_bank\_path](--op_bank_path.md) parameter loading path \> default custom knowledge base path after operator tuning.

1. If the custom knowledge base path for operator tuning is specified through the **TUNE\_BANK\_PATH** environment variable before model conversion, and the custom knowledge base path is also loaded through the [--op\_bank\_path](--op_bank_path.md) parameter during model conversion, the path set by the **TUNE\_BANK\_PATH** environment variable takes effect, and the path loaded by the [--op\_bank\_path](--op_bank_path.md) parameter does not take effect.
2. If neither the path specified by the [--op\_bank\_path](--op_bank_path.md) parameter nor the path specified by the environment variable takes effect, the default custom knowledge base path is used.
3. If no usable custom knowledge base exists in any of the preceding paths, the ATC tool searches for the built-in operator tuning knowledge base.
