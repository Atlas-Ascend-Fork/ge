# fix(python): reject outputs owned by another graph builder

## Behavior

Graph outputs must belong to the receiving GraphBuilder. Reject a foreign TensorHolder before EsSetGraphOutput, which receives only the tensor handle. build_and_reset(outputs=...) reuses this validation. Same-builder output registration is unchanged.

## Validation and limits

Regression: `tests/ge/ut/ge/graph/pyge_tests/graph_output_owner_cpu_test.py`, 4 passed in 0.07s. Tests exercise production GraphBuilder, TensorHolder, Graph, enums and ctypes buffers; native libraries are replaced at the boundary. Real CANN/NPU execution and full C++ UT/ST were not verified.

## Native argument encapsulation

TensorHolder supplies a borrowed pointer through the read-only ctypes `_as_parameter_` protocol. The six tensor/node attribute setters pass the holder directly for conversion to the existing pointer type, without reading another class's `_handle` or transferring ownership. Public method signatures and C APIs are unchanged. CPU regressions use typed pointers and CFUNCTYPE callbacks to verify addresses, UTF-8 names/values, and success/error returns. They do not verify the actual GE native library or NPU execution.

GraphBuilder and TensorHolder cooperate through `apply_scope_infos_to_node` to apply current attributes and control dependencies to new nodes. The four arithmetic results no longer call a protected method of another class. Processing order, returned holder, and owner references are preserved.

The original internal scope-method name remains as a delegating compatibility entry for existing operator generators and generated Python wrappers, without applying attributes or control edges twice.
