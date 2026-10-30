# --jit\_compile

<!-- md-trans-meta sourceCommit=2461b5215e5b8745a550d67c26b5f9840c83434b translatedAt=2026-10-22T03:16:06.297Z pushedAt=2026-10-26T02:54:35.291Z -->

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

## Dependencies and Constraints

None.
