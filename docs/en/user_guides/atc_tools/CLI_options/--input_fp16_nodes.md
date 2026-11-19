# --input\_fp16\_nodes

<!-- md-trans-meta sourceCommit=becb235792507fcdd0382168301e8d0fb82e3246 translatedAt=2026-11-16T14:02:56.078Z pushedAt=2026-11-17T09:06:14.448Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

Specifies the name of the input node whose input data type is float16.

## Related Parameters

- If this parameter is configured, the [--insert\_op\_conf](--insert_op_conf.md) parameter cannot be used for the same input node at the same time.
- When [--framework](--framework.md) is set to 1 and the network model is a MindSpore framework model, setting this parameter is invalid, but the model conversion still succeeds.
<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--input_fp16_nodes_res.md#id1 -->

## Parameter Values

**Parameter value:**
Name of the input node whose data type is float16.

**Parameter value constraints:**
If multiple nodes are specified, they must be enclosed in double quotation marks and separated by **semicolons**.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --input_fp16_nodes="node_name1;node_name2" ...
```

## Dependency Constraints

None.
