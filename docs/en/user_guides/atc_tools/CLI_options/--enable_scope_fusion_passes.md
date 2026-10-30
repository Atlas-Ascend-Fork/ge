# --enable\_scope\_fusion\_passes

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T03:02:04.646Z pushedAt=2026-10-23T13:53:52.281Z -->

## Product Support

Supported on all chips

## Function Description

Specifies the list of Scope fusion rules to be applied during compilation.

Both built-in and user-defined Scope fusion rules fall into the following two categories:

- General fusion rules: Scope fusion rules common to all networks. They take effect by default and cannot be modified by users.
- Non-General fusion rules: Scope fusion rules applicable to specific networks. They do not take effect by default. Users can specify the list of fusion rules to be applied through [--enable\_scope\_fusion\_passes](--enable_scope_fusion_passes.md).

For the currently supported fusion rules, see *TensorFlow Parser Scope Fusion Rule Reference*.

## Related Parameters

None.

## Parameter Values

**Parameter value:**
Name of the registered fusion rule.

**Parameter value format:**
Multiple rule lists are allowed, separated by commas, for example, ScopePass1,ScopePass2,...

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --enable_scope_fusion_passes=ScopePass1,ScopePass2 ...
```

## Constraints

This parameter applies only to TensorFlow network models. To view the log information related to fusion rules during model conversion, set [--log](--log.md) to at least the warning level.
