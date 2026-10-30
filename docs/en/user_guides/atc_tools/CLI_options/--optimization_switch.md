# --optimization\_switch

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T03:31:54.143Z pushedAt=2026-10-26T13:18:55.341Z -->

## Product Support

Supported on all chips

## Function Description

Control switch for graph optimization passes during operator compilation.

## Related Parameters

The difference between this parameter and the [--fusion\_switch\_file](--fusion_switch_file.md) parameter in precision comparison is that --fusion\_switch\_file can only disable the rules of graph fusion and UB fusion and requires a separately configured JSON file, whereas this parameter applies to all rules and can specify fusion rules directly through the parameter without a separate JSON file. If both parameters are configured and the same fusion rule is configured, the configuration of the --optimization\_switch parameter takes precedence.

## Parameter Values

**Parameter value:**
"Passname1:on;Passname2:off", where the key is the Pass name and the value is on (enabled) or off (disabled).

**Parameter value constraints:**
Multiple key-value pairs can be concatenated. Case-insensitive pattern matching is not supported. Multiple configurations are separated by semicolons. For the configurable fusion rules, see [Fusion Rule List](../references/fusion_rules_list.md).

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --optimization_switch="Passname1:on;Passname2:off" --model=model.onnx ...
```

## Dependency Constraints

None.
