# Value Dependent Shape Custom Operator (Value Dependent Operator) Sample

## Sample Overview

- Graph construction entry: `GE`
- Operator programming language: `Ascend C`
- Compilation method: `.asc` and `.cpp` compiled in same target
- Model sinking capability: `Not applicable`
- Core pipeline: `Ascend C kernel and host-side custom op compiled in same library -> GE deliverables -> In-process graph construction -> Session::RunGraph -> RT2 dynamic execution`
- Difference from other samples: This sample focuses on the standard implementation of value dependent operators: declare the data-dependent input via `IMPL_OP`'s `InputsDataDependency`, and determine the concatenation axis by reading the value of the `concat_dim` input at the `InferShape` stage, then derive the output shape.

This sample demonstrates how to develop a value dependent custom operator and run it in a GE graph. Taking the ConcatV2 operator as an example: the deliverable derives the output shape by reading input values in `InferShape` and executes via `EagerExecuteOp`, and the sample builds a graph in-process and directly calls `Session::RunGraph`.

The semantics of `ConcatV2Custom` align with the ops-math production operator ConcatV2 (TensorFlow compatible): the dynamic input `x` consists of at least 2 tensors of the same dtype, the input `concat_dim` is an `int32/int64` scalar, and the output `y` is the concatenation of all inputs along the axis specified by `concat_dim`. Since the concatenation axis (and therefore which dim of the output shape is summed) depends on the **value** of the `concat_dim` input rather than its shape, this operator is a value dependent operator.

## Applicable Scenarios

- Want to learn how to develop a custom value dependent operator (output shape depends on input value) and run it in a GE graph.
- Want to understand the execution flow of value dependent operators: `InputsDataDependency` declaration, compile-time / runtime `InferShape` value reading, and `Execute` with the pre-filled shape.

## Prerequisites

### CANN

- CANN environment has been correctly installed and configured, e.g., executed `source ${ASCEND_HOME_PATH}/set_env.sh`.
- Current environment has `ACL`, `GE`, `Graph`, `Ascend C` related headers and libraries.
- Follow [installation guide](../../../docs/en/quick_install.md) to complete toolkit and ops package installation.

### Framework and Plugins

- This sample does not depend on PyTorch, TensorFlow or TorchAir.

### Environment Variables

- `ASCEND_HOME_PATH`
- `ASCEND_CUSTOM_OPP_PATH` will be automatically appended as current sample's `output/` in `run.sh`

### Additional Dependencies

- `cmake`
- `g++`

## Quick Run

Execute in `examples/custom_op/value_dependent_shape_custom` directory:

### Recommended Method

```bash
source ${ASCEND_HOME_PATH}/set_env.sh
bash run.sh
```

`run.sh` will automatically complete configure, build, install and append `output/` to `ASCEND_CUSTOM_OPP_PATH`. If successful, terminal will print (the `[InferShape]` lines are direct evidence of the value-dependent derivation chain):

```text
=== concat_dim as Data input (axis value read at execution time) ===
[InferShape] concat_dim value unavailable, degrade to unknown dims     <- Compile time (value unavailable, rank-preserving degradation)
[InferShape] got concat axis from input value: 1 (normalized to 1)    <- Runtime: execution-graph InferShape node re-infers from the framework-materialized host copy
[Execute] use pre-filled output shape: [2, 3, 2]                      <- Execute takes the pre-filled shape, no axis value read
output shape: [2, 3, 2]
output values: 1 2 3 4 9 10 5 6 7 8 11 12

```

Mechanism:

- **Data input**: at compile time the graph input value is not yet available (`GetData` returns a null pointer); consistent with the production operator, `InferShape` degrades to `[unknown, unknown, unknown]` preserving the rank. At runtime (RunGraph), the framework chains an InferShape node inside the execution graph before the operator executes: the data-dependent input has been materialized as a host copy by lowering (D2H copy nodes inside the execution graph), the node reads the axis value `1`, re-infers the exact shape `[2, 3, 2]`, and `ExecuteCustomOpWithInferShape` pre-fills it into the output tensor; `Execute` validates the pre-filled shape and allocates the output with exact shape — **no operator-side D2H / stream synchronization throughout**.

### Step-by-step Method

```bash
source ${ASCEND_HOME_PATH}/set_env.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
cmake --install build
export ASCEND_CUSTOM_OPP_PATH="$(pwd)/output:$ASCEND_CUSTOM_OPP_PATH"

cd build
./value_dependent_shape_custom_session_run
cd ..
```

`export ASCEND_CUSTOM_OPP_PATH="$(pwd)/output:$ASCEND_CUSTOM_OPP_PATH"` adds the custom operator package root to the environment variable; GE then loads the deliverable by the rule `output/op_graph/lib/<os>/<arch>/libconcat_v2_custom_op.so`.

## Directory Structure and Key Files

```text
value_dependent_shape_custom
├── CMakeLists.txt
├── README.md
├── run.sh
├── ge
│   ├── custom_op.cpp                    // EagerExecuteOp execution + IMPL_OP registration (InferShape + InputsDataDependency)
│   ├── concat_v2_custom.h               // ConcatV2Custom proto definition
│   └── concat_v2_custom_kernel.asc      // Ascend C kernel source
└── session_run
    └── main.cc                          // In-process graph construction and direct Session::RunGraph
```

Key files:

- `ge/custom_op.cpp`
  Core flow of the custom operator: the `InferShape` logic implements value-dependent shape derivation (aligned with the production operator ops-math ConcatV2's `InferShape4ConcatV2`: reads the value of `concat_dim` via IR index `GetRequiredInputTensor(1)`, normalizes negative axis, accumulates the axis dim over dynamic input instances, degrades rank-preservingly when the value is unavailable), registered via `IMPL_OP(ConcatV2Custom).InputsDataDependency({1})` with the data dependency declaration; `Execute` reads no input value — it takes the framework pre-filled output shape (runtime InferShape re-inference result), validates that it satisfies the concat constraints against the input shapes (non-axis dims equal, axis dim = sum of inputs), then allocates output with exact shape via `MallocOutputTensor` and launches the kernel. The output dtype follows the first `x` input via `InferDataType`.
- `ge/concat_v2_custom_kernel.asc`
  Device kernel and host-side launch wrapper; concatenates 2 inputs along the axis via row-block byte copy (minimal demo implementation, no performance optimization).
- `session_run/main.cc`
  Builds a minimal graph and executes it via `Session::AddGraph + Session::RunGraph` (`ge.graphRunMode=1` dynamic graph mode, aligned with the production operator ConcatV2 GEIR test): `concat_dim` is a host-placed Data input (compile-time degrade to unknown, runtime re-inference to exact).
- `run.sh`
  Chains configure, build, install and run.

## Core Pipeline

1. `session_run/main.cc` builds a minimal graph and executes it via `Session::AddGraph + Session::RunGraph` (`ge.graphRunMode=1`, unknown-shape graphs go through the RT2 dynamic execution pipeline): `Data(x0 [2,2,2]) + Data(x1 [2,1,2]) + Data(concat_dim, host-placed) -> ConcatV2Custom`, compile-time output shape contains unknown dims.
2. The `InferShape` in `ge/custom_op.cpp` is registered via `IMPL_OP`, declaring `concat_dim` (IR input 1; `x` is the dynamic input occupying IR 0) as a data-dependent input. Both branches print `[InferShape]` logs to observe the chain:
   - Value unavailable (compile time, Data input not const): `GetData` returns a null pointer; consistent with the production operator, degrades rank-preservingly to `[unknown, unknown, unknown]`;
   - Value available (runtime reads the framework-materialized host copy value 1): output = first input shape `[2,2,2]` with axis-1 dim replaced by `2 + 1 = 3`, i.e., `[2,3,2]` (negative axis values are normalized by rank first).
3. **Framework-side mechanism (value dependent operators need not implement it themselves)**: lowering automatically materializes a host copy for data-dependent inputs (D2H copy nodes inserted into the execution graph, see `CreateDataDependentResult` in `runtime/v2/lowering/placement/placed_lowering_result.cc`); operators with `ShapeInferOp` registered are chained with a runtime InferShape node in the execution graph, and `ExecuteCustomOpWithInferShape` pre-fills the re-inferred shape into the output tensor before Execute (see `custom_node_converter.cc` / `custom_op_kernel.cc` under `runtime/v2/engine/custom/`).
4. `CMakeLists.txt` compiles `ge/custom_op.cpp` and `ge/concat_v2_custom_kernel.asc` together into `libconcat_v2_custom_op.so`.
5. In the `Execute` callback, `ge/custom_op.cpp` **reads no input value**: it takes the pre-filled output shape, validates that it satisfies the concat constraints against the input shapes (the output differs from the first input at exactly the axis dim, axis dim = sum of inputs, all other dims equal), derives the copy layout accordingly, calls `ctx->MallocOutputTensor(...)` with the exact shape (a value dependent operator does not need to allocate by upper bound with a reserved shape buffer like a data dependent operator), and finally invokes the launch wrapper exported from `.asc`.
6. After execution, `session_run/main.cc` reads the output tensor and prints the actual shape and values.


## Build Artifacts

- `output/op_graph/lib/linux/x86_64/libconcat_v2_custom_op.so`
  Custom operator deliverable used by GE on Linux x86_64; on aarch64 it is `output/op_graph/lib/linux/aarch64/libconcat_v2_custom_op.so`.
- `output/op_graph/include/concat_v2_custom.h`
  Operator proto header directly usable by graph construction.
- `build/value_dependent_shape_custom_session_run`
  Program that directly calls `Session::RunGraph`.

## Result Verification

On success:

- `output/op_graph/lib/<os>/<arch>/libconcat_v2_custom_op.so` is generated.
- `output/op_graph/include/concat_v2_custom.h` is generated.
- Output `output shape: [2, 3, 2]` and `output values: 1 2 3 4 9 10 5 6 7 8 11 12` (`x0` is `[2,2,2]` holding 1~8, `x1` is `[2,1,2]` holding 9~12, concatenated along axis 1).
- **Direct evidence that the value-dependent derivation chain works**: compile time prints `[InferShape] concat_dim value unavailable, degrade to unknown dims`; runtime prints `[InferShape] got concat axis from input value: 1 (normalized to 1)` from the execution-graph InferShape node; `Execute` prints `[Execute] use pre-filled output shape: [2, 3, 2]` (output shape comes from the framework pre-fill, no operator-side D2H).
- Optional: re-run with `export DUMP_GE_GRAPH=2` and confirm in the dumped graph (e.g., `ge_onnx_*_AfterInfershape.pbtxt`) that the compile-time output shape is `[-1, -1, -1]` (exact after runtime re-inference).

If it fails, check first:

- Whether `ASCEND_HOME_PATH` is set and the CANN environment is properly `source`d.
- Whether `ASCEND_CUSTOM_OPP_PATH` contains this sample's `output/`.
- Whether `output/op_graph/lib/<os>/<arch>/libconcat_v2_custom_op.so` and `output/op_graph/include/concat_v2_custom.h` are generated.
- Whether the current environment has a usable NPU and a usable Ascend C compilation environment.
- If `Execute` reports shapes not fully known: runtime InferShape did not re-infer (pre-filled shape stays unknown). Confirm `ge.graphRunMode=1` takes effect and the GE version provides the `ExecuteCustomOpWithInferShape` mechanism (operators must register `ShapeInferOp`).

## Notes / Limitations

- The sample validates results with `x0=[2,2,2]`, `x1=[2,1,2]` (float) and `concat_dim` (value `1`); the actual output is `[2, 3, 2]`.
- The demo kernel is fixed to 2 inputs and requires fully-known input shapes, consistent non-axis dims and contiguous ND layout (`InferShape` itself supports any instance count and unknown dims).
- The sample requires a usable NPU and Ascend C compilation environment; runtime re-inference and pre-fill rely on the RT2 dynamic execution pipeline (`ge.graphRunMode=1`), and `Execute` fails explicitly if the environment does not support it.
- The `.asc` compilation option is currently fixed to `--npu-arch=dav-2201`; adjust `CMakeLists.txt` for a different target chip.

## Appendix

### Operator Specification

| Item | Content |
| --- | --- |
| Operator type | `ConcatV2Custom` |
| Inputs | `x` (dynamic input, at least 2 instances), `concat_dim` |
| Output | `y` |
| Input `x` shape | Same rank across instances, consistent non-axis dims |
| Input `concat_dim` shape | Scalar (`[]` or `[1]`), its value is the concatenation axis (negative axis supported) |
| Output shape | First input shape with the axis dim replaced by the sum of that dim across all inputs |
| Input `x` dtype | `DT_FLOAT` / `DT_FLOAT16` / `DT_INT32` |
| Input `concat_dim` dtype | `DT_INT32` / `DT_INT64` |
| Output dtype | Same as input `x` (follows the first `x` input via `InferDataType`) |
| Format | `ND` |
| Kernel name | `concat_v2_custom` |
| Attribute | `N` (instance count of `x`, fixed to 2 in the demo graph) |

### Correspondence with the Production Operator ConcatV2

| Production operator (ops-math `conversion/concat_v2`) | This sample |
| --- | --- |
| `REG_OP(ConcatV2)`: `DYNAMIC_INPUT(x)` + `INPUT(concat_dim)` + `OUTPUT(y)` + `ATTR(N)` | Same proto structure (dtype set trimmed to the subset needed by the demo) |
| `IMPL_OP_INFERSHAPE(ConcatV2).InferShape(InferShape4ConcatV2).InputsDataDependency({1})` | `IMPL_OP(ConcatV2Custom).InputsDataDependency({1})` + `ShapeInferOp::InferShape` class capability registration (Eager custom operator form) |
| `GetRequiredInputTensor(1)` reads the axis value by IR index; rank-preserving degradation when unavailable | Same |
| Negative axis normalization; per-instance axis-dim accumulation over dynamic inputs | Same |
| AICPU kernel reads the axis value at execution time (host side reads no value) | This sample's host side likewise reads no value: the kernel copy layout is derived from the pre-filled shape and input shapes |

### Value Dependent Derivation Convention

The output shape derivation of this sample has two stages, and each branch prints `[InferShape]` logs for observation:

- `InferShape` stage (the same registered function is invoked at both compile time and runtime, ensuring consistent derivation):
  - Value unavailable at compile time (`GetData` returns a null pointer), printing `[InferShape] concat_dim value unavailable, degrade to unknown dims` and degrading to `[unknown, unknown, unknown]` preserving the rank;
  - At runtime the execution-graph InferShape node reads the framework-materialized host copy value 1 and re-infers, printing `[InferShape] got concat axis from input value: 1 (normalized to 1)`;
- `Execute` stage: reads no input value. `ExecuteCustomOpWithInferShape` has pre-filled the re-inferred shape into the output tensor; `Execute` validates that the pre-filled shape satisfies the concat constraints against the input shapes (the output differs from the first input at exactly the axis dim, axis dim = sum of inputs, all other dims equal), derives the copy layout accordingly, and allocates output memory via `MallocOutputTensor` with the exact shape.

Development essentials (common to value dependent operators):

- The value-dependent input must be declared via `InputsDataDependency` before it can be fetched in `InferShape`; fetching undeclared inputs is undefined behavior. `GetData` returning a null pointer for non-const inputs at compile time is expected — degrade rank-preservingly instead of failing.
- With dynamic inputs, fetch values and dtype by IR index (`GetRequiredInputTensor` / `GetRequiredInputDesc`), never by flat index (the placeholder tensor's `GetDataType()` is unreliable); build graphs with `create_dynamic_input_x(n, false)` to keep the flat input order consistent with the IR declaration order, otherwise indexes misalign.
- Declaring the value input's graph input as host-placed (`SetPlacement(kPlacementHost)`) is recommended to avoid H2D/D2H round trips.

For the current sample input, `x0` is `[2,2,2]` (values 1~8), `x1` is `[2,1,2]` (values 9~12), `concat_dim` value is `1`, the actual output shape is `[2, 3, 2]`, and the output data is `1 2 3 4 9 10 5 6 7 8 11 12`.
