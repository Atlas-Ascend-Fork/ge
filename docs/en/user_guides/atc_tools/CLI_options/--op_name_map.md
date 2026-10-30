# --op\_name\_map

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T03:28:15.026Z pushedAt=2026-10-27T11:33:37.375Z -->

## Product Support

<!-- npu="950" id6 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id6 -->
<!-- npu="A3" id5 -->
- Atlas A3 products: Not supported
<!-- end id5 -->
<!-- npu="910b" id4 -->
- Atlas A2 products: Not supported
<!-- end id4 -->
<!-- npu="310b" id3 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id3 -->
<!-- npu="310p" id2 -->
- Atlas inference products: Supported
<!-- end id2 -->
<!-- npu="910" id1 -->
- Atlas training products: Supported
<!-- end id1 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--op_name_map_res.md#id1 -->

## Feature Description

Path and file name of the mapping configuration file for extension operators (non-standard operators). Because the same extension operator may have different functions in different networks, you can specify the mapping from this extension operator to the extension operator that actually runs in a specific network.

In a Caffe network, layers that have the same type name but different computing functions, such as the DetectionOutput layer, must be specified as detection operator types such as FSRDetectionOutput and SSDDetectionOutput through operator mapping. Otherwise, the generated offline model fails to execute.

This parameter applies only to the Caffe framework.

## Related Parameters

This parameter is supported only when [--framework](--framework.md) is set to 0. When [--framework](--framework.md) is set to 1 and the network model is a MindSpore framework model, setting this parameter does not take effect, but the model conversion succeeds.

## Parameter Value

**Parameter Value:**
Path and file name of the extension operator mapping configuration file.

**Parameter Value Format:**
Path and file name: supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

## Recommended Configuration and Benefits

None.

## Example

The following is an example of the extension operator mapping configuration file (the file name is _opname\_map.cfg_):

```text
DetectionOutput:SSDDetectionOutput
```

Upload the configured _opname\_map.cfg_ to any directory on the server where the ATC tool is located, for example, $HOME/module. The following is an example of how to use it:

```bash
atc --op_name_map=$HOME/module/opname_map.cfg ...
```

## Dependencies and Constraints

None.
