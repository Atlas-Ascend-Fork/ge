# --jit\_compile

<!-- md-trans-meta sourceCommit=0e375ee43f53b583431f339c4dc1d9fa79286a1d translatedAt=2026-11-16T14:09:46.428Z pushedAt=2026-11-17T10:57:59.340Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

During model compilation, this parameter determines whether to prioritize online compilation of operators or to prioritize the use of precompiled operator binary files.

## Related Parameters

None.

## Parameter Values

- 0: Preferentially search for the compiled operator binary files in the system. If they are found, the operators are not compiled again, delivering better compilation performance. If they are not found, the operators are compiled.
- 1 (default): Compile operators online. The system performs fusion and optimization based on the obtained graph information to compile operators with better runtime performance.
- 2: For networks with static shapes, compile operators online. For networks with dynamic shapes, preferentially search for the compiled operator binaries in the system. If the corresponding binaries are not found, compile the operators.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --jit_compile=1 --model=model.onnx ...
```

## Dependency Constraints

None.
