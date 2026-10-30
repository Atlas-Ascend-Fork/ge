# --buffer\_optimize

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T02:46:25.445Z pushedAt=2026-10-23T02:05:15.232Z -->

## Product Support

Supported on all chips

## Feature Description

Switch for data cache optimization.

## Related Parameters

None.

## Parameter Values

**Parameter values:**

- l1\_optimize: enables l1 optimization. In the current version, this parameter is invalid and is equivalent to off\_optimize.
- l2\_optimize: (default) enables l2 optimization.
- off\_optimize: disables data cache optimization.

Here, l1 refers to the L1 Buffer, which is a general-purpose internal memory. It is a relatively large data staging area in the AI Core that can temporarily store data that needs to be reused in the AI Core, thereby reducing the number of reads and writes from the bus. l2 refers to the L2 Buffer, which represents external memory. The AI Core needs to load data from external memory into internal memory before it can perform the corresponding computation.

## Recommended Configuration and Benefits

It is recommended that you enable the data cache optimization feature. Enabling data cache optimization improves computing efficiency and performance. However, some operators may have scenarios that are not considered in their implementation, which may affect precision. Therefore, if a precision issue occurs, you can try disabling data cache optimization. If the precision meets the requirement after the data cache optimization feature is disabled, identify the problematic operator and report it to technical support for further analysis and resolution. After the operator issue is resolved, it is recommended that you keep the data cache optimization feature enabled.

## Example

```bash
atc --buffer_optimize=l2_optimize ...
```

## Dependency Constraints

None.
