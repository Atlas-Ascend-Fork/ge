# fix(python): validate unsigned constant ranges before ctypes conversion

## Behavior

Unsigned constant values must fit their declared uint32 or uint64 type before ctypes conversion. Scalar and list entry points agree. The existing negative uint64 diagnostic is preserved. Signed and floating-point factories are unchanged.

## Validation and limits

Regression: `tests/ge/ut/ge/graph/pyge_tests/unsigned_constant_range_cpu_test.py`, 12 passed in 0.10s. Tests exercise production GraphBuilder, TensorHolder, Graph, enums and ctypes buffers; native libraries are replaced at the boundary. Real CANN/NPU execution and full C++ UT/ST were not verified.

## Scan compliance maintenance

The four integer constant APIs share value/shape preparation while preserving scalar dispatch, empty lists, type errors and unsigned range checks. Floating-point constant behavior is unchanged. Methods follow the category order: magic methods, properties, static methods, class methods, ordinary methods, and protected instance helpers. Relative order within each category is preserved.
