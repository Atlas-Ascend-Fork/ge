# --input\_fp16\_nodes

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T03:10:42.298Z pushedAt=2026-10-26T01:28:23.493Z -->

## Product Support

All chips supported

## Function Description

Specifies the name of the input node whose input data type is float16.

## Related Parameters

- If this parameter is configured, the [--insert\_op\_conf](--insert_op_conf.md) parameter cannot be used for the same input node at the same time.
- When [--framework](--framework.md) is set to 1 and the network model is a MindSpore framework model, setting this parameter does not take effect, but the model conversion succeeds.

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

## Dependencies and Constraints

None.
