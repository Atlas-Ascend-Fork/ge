# Deliverables and OPP Specifications

<!-- md-trans-meta sourceCommit=1c0a36284515311df07ca620682d500844a67127 translatedAt=2026-10-21T09:35:20.692Z pushedAt=2026-10-28T03:30:34.496Z nl=2 -->

## Standard Directory Tree (Single Source of Truth)

Custom operators are loaded by GE as .so deliverables, and the root directory of the OPP package is specified through the ASCEND\_CUSTOM\_OPP\_PATH environment variable. The following is the unified directory structure specification for the entire document:

```tree
<custom_opp_root>/
├── op_graph/                          # GE-side deliverables (core)
│   ├── include/                       # REG_OP proto header files (for the graph construction side/Parser)
│   │   └── my_custom_op.h
│   └── lib/
│       └── <os>/<arch>/               # e.g., linux/x86_64, linux/aarch64
│           └── libcust_opapi.so       # GE deliverables (unified naming)
├── framework/                         # Framework-side deliverables (as needed)
│   ├── tensorflow/
│   │   ├── libcustom_ops.so           # TF-side operator declaration (TF scenario)
│   │   └── npu_supported_ops.json     # Adapter configuration (TF scenario)
│   └── onnx/
│       └── libonnx_parser_plugin.so   # ONNX parsing plugin (ONNX scenario)
└── python/                            # PyTorch-side deliverables (optional, not involved in OPP loading)
    ├── add_custom.py                  # TORCH_LIBRARY definition
    └── add_custom_converter.py        # converter registration
```

## Directory Item Description

| Directory | Content | When Required |
| --- | --- | --- |
| op_graph/include/ | REG_OP proto header file | GE native/PyTorch/ONNX scenarios (for the graph construction side or Parser to create nodes) |
| op_graph/lib/<os\>/<arch\>/ | GE deliverable .so (callback implementation) | All scenarios |
| framework/tensorflow/ | TF-side declaration + Adapter configuration | TensorFlow scenarios only |
| framework/onnx/ | ONNX parsing plugin .so | ONNX scenarios only |
| python/ | PyTorch schema + converter | PyTorch scenarios only; this directory does not participate in OPP loading and is imported on demand by the Python side |

**Kernel binary location**: The precompiled kernel binary is not required to be placed in the OPP package. It can be compiled together with the .so, or loaded from a file during the Execute phase (ensure that the path conforms to the OPP package structure convention and **does not depend on the temporary working directory**; for details, see [Debugging and Common Issues](debug_and_common_issues.md)).

## Naming Conventions

| Item | Convention | Description |
| --- | --- | --- |
| GE deliverable | libcust_opapi.so | Unified naming for easy identification and script processing |
| Parsing plugin | libonnx_parser_plugin.so | ONNX scenario |
| TF-side operator | libcustom_ops.so | TensorFlow scenario |
| REG_OP header file | op_graph/include/\<op>.h | Same name as the op type for easy retrieval |
| \<os>/\<arch> | For example, linux/x86_64 | Fill in according to the target platform; all are required |

## Environment Variables and Loading Mechanism

```bash
export ASCEND_CUSTOM_OPP_PATH="<custom_opp_root>:$ASCEND_CUSTOM_OPP_PATH"
```

- Supports appending the system built-in path: **write the custom path first, then append the original value** (:$ASCEND\_CUSTOM\_OPP\_PATH).
- GE searches for the deliverable .so under $ASCEND\_CUSTOM\_OPP\_PATH/op\_graph/lib/<os\>/<arch\>/.
- The ONNX parsing plugin and the GE deliverable must be **loaded simultaneously**, and the parsing plugin must be loaded before ATC compilation.
- The `python/` directory of PyTorch is not loaded through this variable; it is installed as a Python package and referenced via `/`.

## Deployment Steps

Take the build artifacts under `build/` as an example:

```bash
# 1. Organize the OPP structure.
mkdir -p output/op_graph/lib/linux/x86_64
cp build/libcust_opapi.so output/op_graph/lib/linux/x86_64/

# 2. If CMake install rules are used, this can replace the previous step.
cmake --install build --prefix "$(pwd)/output"

# 3. Configure the environment variables.
export ASCEND_CUSTOM_OPP_PATH="$(pwd)/output:$ASCEND_CUSTOM_OPP_PATH"

# 4. Verify that GE can find the deliverables.
ls "$ASCEND_CUSTOM_OPP_PATH"/op_graph/lib/linux/x86_64/
```

For a complete end-to-end example, see [Quick Start](quick_start.md).
