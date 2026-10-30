# ONNX Plugin Python-ization Design Document

## 1. Overview

ONNX plugin Python-ization is an ONNX custom operator parsing plugin extension capability provided by the `ge` Python package: a plugin is a Python file that declares the mapping from an ONNX source operator to a GE target operator and completes parsing through callbacks. Plugins are loaded and executed during the parsing stage of ATC ONNX model compilation, without changing the way ATC is used.

The GE ONNX parser reads the protobuf content of the model file directly to complete graph parsing. Custom operators in the model (defined by the exporter, not built in GE) cannot be integrated by default; previously, parsing code had to be developed as a C++ plugin (corresponding to the C++ `ParseParamsFn`, `ParseParamsByOperatorFn`, and `ParseOpToGraphFn` registration methods, see the [ONNX graph integration](../../../user_guides/custom_op/custom_op_v2/custom_op_development_guide.md#64-onnx-graph-entry) chapter of the custom operator development guide). With Python-ization, integrating a custom operator only requires one `.py` file, and the plugin callbacks do two kinds of work:

1. **Parameter parsing**: transcribe attributes on the ONNX node to the GE operator, using the `parse_node` or `parse_operator` callback;
2. **Operator decomposition**: when GE has no existing operator, replace the original node with a subgraph built from existing operators, using the `decompose` callback, without writing device kernels.

This feature, together with [GE Pass Python-ization](ge_python_pass_design.md) and [custom operator Python-ization](ge_python_custom_op_design.md), constitutes the three plugin extension types of the `ge` Python package; loading and discovery share the same mechanism (the `ASCEND_CUSTOM_OPP_PATH` environment variable), see [GE-PY Python Module Class Relationship Document](ge_python.md). The C++ ONNX plugin mechanism remains available; one origin type (see Section 6.2) can be provided by only one plugin. This feature is supported on all chips.

## 2. Version Constraints

| Dependency | Version | Description |
| ---- | ---- | ---- |
| Python | 3.9-3.14 (cp39-cp314, officially supported) | Native artifacts are delivered per Python version (see Section 8.1); the build matrix covers cp39-cp314, and the sample has been verified end to end on Python 3.12 |
| onnx | 1.21.0 | Consistent with the tested scope of the [sample](../../../../../examples/onnx_plugin/README_en.md); the onnx package is used only in the PyTorch ONNX export flow (`torch.onnx.export`); the GE parser does not depend on this package |
| pybind11 | >=2.13.6,<3.0.0 | Used only by fallback codegen; required for on-site compilation of the bridge/native when no matching native artifact exists |

Python version support adopts the same "main wheel + per-version native sub-wheel + fallback codegen + automatic selection at installation" model as the Python pass, runtime, and custom-op components:

- **Build and delivery**: the `ge_py_onnx_plugin_bridge` native wheel is produced per Python version by the matrix build (`ge_python_onnx_plugin_native_wheel_matrix` in `api/python/ge/CMakeLists.txt`), with official support for cp39-cp314, and delivered with the CANN package; the `ge_py` main wheel carries only Python code and fallback codegen resources.
- **Installation selection**: the CANN installation script selects the native wheel matching the current `python3` interpreter version (`ge-compiler_custom_install.sh`); when no wheel matches, that native wheel is skipped and the fallback codegen resources carried by the main wheel take over.
- **Runtime matching**: the bridge library and native module record the `python_tag` (such as `cp312`), platform, and bridge ABI version (currently 2; artifacts are version-paired with the loader, and artifacts with a mismatched ABI are rejected explicitly) in the manifest (`manifest.json`); at runtime, the version is read from the Python symbols loaded in the current process and the `python_tag` is computed, and only artifacts matching the current interpreter tag are loaded. When no matching artifact is found, `ge.onnx_plugin.fallback_runtime.run_fallback_codegen()` is invoked first to build the artifact on site with the current Python interpreter; only when both prebuilt artifacts and fallback codegen are unavailable does compilation fail with `No compatible ONNX Python plugin bridge artifact found for runtime ...`; the error is surfaced to the ATC console via E19999 and contains the current interpreter runtime key, the list of available artifacts (python tag, platform, bridge ABI, and artifact path), and fallback troubleshooting and version alignment guidance. The implementation is in `base/common/python_runtime/python_artifact_utils.h` and `parser/parser/onnx/python_onnx_plugin_bridge/onnx_plugin_bridge_loader.cc`.

Therefore, the Python version executing the ATC compilation does not need to match the build-time Python version: when no matching artifact exists in the package, it is built on site automatically, and reinstalling the CANN package is not required. Fallback compilation requires `pybind11` (>=2.13.6,<3.0.0), a C++ compiler, the Python development headers, and libpython in the current Python environment; if these conditions are not met, switch the interpreter to one of the available versions listed in the error message, or install the matching `ge` Python package.

## 3. Overall Architecture

### 3.1 Components

| Component | Code Location | Responsibility |
| ---- | -------- | ---- |
| Python public package `ge.onnx_plugin` | `api/python/ge/ge/onnx_plugin/` | Descriptor definition and validation, process-level registry, plugin scanning and loading, callback dispatch, `OnnxNode` binding |
| C++ bridge layer | `parser/parser/onnx/python_onnx_plugin_bridge/` | Load bridge artifacts, initialize Python runtime, register Python-side descriptors into the parser, forward callbacks |
| ONNX parser | `parser/parser/onnx/onnx_parser.cc` | Match registered plugins by origin type node by node during parsing and trigger callbacks |

```mermaid
graph LR
    subgraph ATC compilation process
        P[ONNX parser] -->|"load artifacts/register descriptors/forward callbacks"| B[C++ bridge layer]
        B -->|"import modules/call callbacks"| G["ge.onnx_plugin (Python package)"]
        G -->|"scan and load"| F["Plugin .py files (user written)"]
    end
    E["ASCEND_CUSTOM_OPP_PATH"] -->|"points to plugin directory"| F
```

### 3.2 Compile-time Runtime Flow

1. The user sets `ASCEND_CUSTOM_OPP_PATH` to the plugin directory and executes ATC to compile the ONNX model;
2. ONNX parser initialization: only when a loadable Python plugin entry exists in the environment variable paths (a `.py` file, or a single-level non-underscore `.py` file / a sub-directory containing `__init__.py` under a directory), the prebuilt bridge library `libge_python_onnx_plugin_bridge.so` is matched by runtime key (`python_tag` + platform + bridge ABI) and loaded, and the Python runtime is prepared; when no prebuilt bridge matches or loading fails, `ge.onnx_plugin.fallback_runtime.run_fallback_codegen()` builds the artifact on site with the current Python interpreter and loads it, and only when both fail does it report `No compatible ONNX Python plugin bridge artifact found` (E19999). When the environment variable is not set, or points to a C++-only custom operator directory (containing only so/ini deliverables, without any Python plugin entry), no bridge code is loaded and the behavior is identical to not using this feature. `ASCEND_CUSTOM_OPP_PATH` is also the standard delivery variable for C++ custom operators, so environments delivering only C++ operators must not be blocked by this feature;
3. The bridge layer enters the Python interpreter, imports `ge.onnx_plugin._bridge`, and calls `load_and_get_onnx_plugin_descriptors()`: plugin files are scanned and imported one by one, and the `onnx_plugin()` calls in the plugin code write descriptors into the Python process-level registry;
4. The descriptor list is passed back to the C++ side, and each origin type generates one `domi::OpRegistrationData` registration (`FrameworkType` is ONNX, `OriginOpType` is `domain::opset::source`), hooked to the parameter parsing or decomposition entry according to callback types;
5. When parsing each ONNX node, a non-built-in operator constructs the origin type as `<domain>::<opset>::<op_type>` to look up registrations; on a hit, the corresponding Python callback is invoked through the bridge layer;
6. When a plugin callback raises a Python exception or a plugin file fails to load, compilation fails and exits; the default error (`E19999`) contains the Python-side error information (exception type, error statement, plugin file and line number).

## 4. Directory Structure

```text
api/python/ge/ge/onnx_plugin/        // Python public package
├── __init__.py                      // External entry: exports OnnxNode, OnnxPlugin, onnx_plugin
├── plugin.py                        // onnx_plugin() factory, OnnxPlugin descriptor, callback binding and validation
├── registry.py                      // Descriptor registry (thread safe), indexed by descriptor_key and origin type
├── bootstrap.py                     // Plugin scanning and loading entry (reads ASCEND_CUSTOM_OPP_PATH)
├── _bridge.py                       // C++ bridge call entry: load plugins, dispatch the three callbacks
├── _native.py                       // Loads the native module, provides OnnxNode
├── _artifact_utils.py               // Native artifact discovery and manifest validation (python_tag/platform/bridge ABI)
├── fallback_runtime.py              // Fallback entry: on-site compilation through the shared engine when no matching artifact exists
├── fallback_codegen/                // Build-time generated fallback compilation resources (shipped in the main wheel)
└── native_bindings/                 // pybind11 bindings of OnnxNode (read-only view over NodeProto)

parser/parser/onnx/python_onnx_plugin_bridge/    // C++ bridge layer
├── onnx_plugin_bridge_c_api.h       // Bridge C API: ABI version, registrar, callback table definitions
├── onnx_plugin_bridge.cc            // Bridge implementation: initialization, descriptor registration, callback forwarding
├── onnx_plugin_bridge_loader.cc     // Artifact loading: match by runtime key and dlopen the bridge library
└── onnx_plugin_bridge_registrar.cc  // Register descriptors into the ONNX parser operator registry
```

The bridge library and native module are produced at build time and organized under `python_tag-platform` directories in the run package at `ge/onnx_plugin/python_onnx_plugin_artifacts/`; artifacts generated by fallback on-site compilation are written to the same directory. `fallback_codegen/` is generated by `gen_fallback_resources.py` at build time and shipped in the main wheel.

## 5. Interface Description

For the complete interface description, see the onnx_plugin API reference (`docs/zh/api/graph_engine_api/python/ge/onnx_plugin/onnx_plugin.md`); this section only lists design-level key points.

### 5.1 onnx_plugin() Descriptor Factory

```python
onnx_plugin(*, source: str, domain: str, opsets: Collection[int], target: str) -> OnnxPlugin
```

| Parameter | Meaning |
| ---- | ---- |
| `source` | ONNX source operator type, such as `MyElu`. Non-empty string, must not contain `:` (the colon is the origin type field separator; its presence makes the origin type impossible to split back into `<domain>::<opset>::<source>`) |
| `domain` | The domain explicitly declared by the plugin author at registration, non-empty string, must not contain `:` (same reason as above). Different in meaning from the node domain field in the model file, see Section 6.3 |
| `opsets` | Collection of supported ONNX opset versions, elements are positive integers, deduplicated and sorted at registration |
| `target` | GE target operator type; the corresponding operator prototype must be installed and registered |

Parameter validation is executed immediately at call time: invalid types raise `TypeError`, invalid values (such as empty collections or non-positive integers) raise `ValueError`. `source`, `domain`, and `opsets` expand into a set of origin types (`domain::opset::source`, such as `example.domain::1::MyElu`).

### 5.2 Callback Binding Methods

The `OnnxPlugin` descriptor provides three binding methods (usable directly as decorators). One descriptor can bind multiple callbacks (such as parameter parsing combined with decomposition); binding the same callback repeatedly raises `ValueError`:

| Method | Callback Signature | Return Value Convention | Purpose |
| ---- | -------- | ---------- | ---- |
| `parse_node` | `(node: OnnxNode, target) -> None` | Must return `None` | Read ONNX node attributes by name and write them to the target operator; preferred |
| `parse_operator` | `(source, target) -> None` | Must return `None` | Parse attributes based on Operator; the only choice when a node carries composite attributes such as tensors or subgraphs |
| `decompose` | `(source) -> Graph` | Must return a `ge.graph.Graph` object | Replace the original node with a subgraph built from existing operators; the subgraph is built with the ES graph construction API (see [ES Python API](../../../user_guides/es_graph/api/es_python.md)) |

In callbacks, `target` is a writable operator and `source` is a read-only operator; a return value that violates the convention fails compilation with `TypeError`.

### 5.3 OnnxNode Object

The `node` received by the `parse_node` callback is a native-layer read-only wrapper over the ONNX `NodeProto`; all properties are read-only:

| Property | Meaning |
| ---- | ---- |
| `name` | Node name |
| `origin_type` | The node's `op_type` (without the domain and opset prefix) |
| `inputs` / `outputs` | Tuples of the node's input and output data names |
| `attrs` | Attribute dictionary (read-only mapping), supports only `int`, `float`, `str`, and homogeneous lists |

When the node carries composite attributes such as tensors (for example the `value` of Constant) or subgraphs (for example `then_branch`/`else_branch` of If), reading `attrs` fails as a whole; such nodes must use `parse_operator` instead.

## 6. Registration and Discovery

### 6.1 Plugin Discovery and Loading

- The discovery entry is the `ASCEND_CUSTOM_OPP_PATH` environment variable; multiple paths are joined by the path separator (colon on Linux), and each entry can be a directory or a single `.py` file;
- Directories are scanned one level: top-level `.py` files (files starting with an underscore are skipped) are imported one by one, and subdirectories containing `__init__.py` are imported as packages. Non-Python files and `.py` files in deeper levels are not loaded. When a plugin file shares a directory with its dependency libraries or data files, the dependencies are not loaded automatically and must be imported by the plugin code itself;
- Plugins are imported and executed at ATC initialization; all top-level code in the file runs. If any plugin file fails to load (syntax error, import exception, etc.), the whole compilation is interrupted; locating the specific file requires debug logs;
- The same file is loaded only once in the same process (deduplicated by file path).

### 6.2 Registration Rules and Conflict Handling

- The `descriptor_key` of a descriptor (composed of module name, callback name, callback kind, and source/domain/opsets) and every origin type must be unique; duplicate registration raises `ValueError`;
- Handling of origin type conflicts: conflicts among multiple Python plugins fail parser initialization; conflicts between a Python plugin and a C++ plugin keep the C++ plugin and reject the Python plugin;
- Therefore, under the same `source` and `domain`, the `opsets` of different plugins must not overlap — overlap produces identical origin types, rejected at the registration stage.

### 6.3 Node Matching and Callback Dispatch

The origin type used for matching is constructed by the parser from the model file: `<node domain>::<model opset_import version>::<node op_type>`. The node's `domain` field can be empty; per the ONNX standard, empty means the standard domain `ai.onnx`. The domain and opset here come from the model file, while the `domain` registered by the plugin is a mapping key explicitly declared by the author; the two match only when their values are identical.

When reading `opset_import`, the parser normalizes the empty domain to `ai.onnx`. If one normalized domain is declared with multiple different versions (the typical case: the export side configures `ai.onnx` in `custom_opsets` with a version different from `opset_version`), the parser prints a WARNING naming the conflicting versions and the effective one; the matching behavior is unchanged (the later declaration wins).

When an origin type misses all registrations, the parser appends an E19999 diagnosis besides the original errors (E13010 in the pre-check stage and E16002 in the parsing stage): if the domain has a version conflict, the diagnosis references the conflict and probes registrations under the other declared versions — for example, if the model resolves to `ai.onnx::2::Relu` while `ai.onnx::13::Relu` is registered under another declared version, the diagnosis points to the export-side version override; without a conflict, a generic hint is given (check that the plugin's declared domain/opsets cover the origin and that the plugin file is discoverable via `ASCEND_CUSTOM_OPP_PATH`). The diagnosis only adds information and never changes parsing behavior.

After a node hits a registration, the dispatch rules are:

- `parse_node` and `parse_operator` belong to the same parameter parsing stage; bind only one of them. When both are bound to the same descriptor, the parser calls only the callback bound by `parse_operator`, and the callback bound by `parse_node` is ignored without an error;
- When only `decompose` is bound, the framework automatically adds a default parameter parsing (only marking the original type on the operator, without attribute transcription), and the original node's inputs and outputs are passed to the decomposed subgraph;
- When the target operator's input and output counts are fixed, no port registration is needed; otherwise ports must be registered one by one inside the callback (`register_input`, `register_optional_input`, etc.), and the registration order must match the ONNX operator's input order; see Section 4.1 of the sample README.

## 7. Usage Example

For the complete runnable sample (including a `parse_node` + `decompose` plugin, a `parse_operator` plugin, and a one-click script), see the [ONNX Python plugin sample](../../../../../examples/onnx_plugin/README_en.md). The following is a minimal plugin example (excerpted from the sample `my_elu_plugin.py`):

```python
import json

from ge.onnx_plugin import onnx_plugin

my_elu = onnx_plugin(
    source="MyElu",          # custom operator type in the ONNX model
    domain="example.domain", # domain declared at registration
    opsets=(1,),             # supported opset versions
    target="Elu",            # existing GE target operator
)

@my_elu.parse_operator
def parse_my_elu(source, target) -> None:
    """Parse alpha from the JSON attribute string of the source operator and write it to Elu."""
    attrs = json.loads(source.get_attr("attribute"))
    alpha = 1.0
    for attr in attrs.get("attribute", []):
        if attr.get("name") == "alpha":
            alpha = float(attr.get("f", 1.0))
    target.set_attr("alpha", alpha)
```

At compilation, set the plugin file directory to `ASCEND_CUSTOM_OPP_PATH`:

```bash
export ASCEND_CUSTOM_OPP_PATH="$(pwd)/plugin:${ASCEND_CUSTOM_OPP_PATH:-}"
atc --model=model.onnx --framework=5 --output=output/model --soc_version=<soc_version>
```

## 8. Compatibility Check

### 8.1 Python Version Release and Fallback Compatibility Strategy

The version release of the ONNX plugin is consistent with the Python pass, runtime, and custom-op components, adopting the "main wheel + per-version native sub-wheel + runtime fallback + automatic selection at installation + multi-version build pipeline" model.

The main wheel carries only pure Python code and fallback codegen resources; each Python minor version generates its own `ge_py_onnx_plugin_bridge` native wheel. Loading follows the order of "matching prebuilt artifact first, then on-site compilation with the current Python runtime on a miss", and an error is reported only when both prebuilt artifacts and fallback are unavailable.

#### 8.1.1 Python Version Matrix

The officially supported Python tags are:

- `cp39`
- `cp310`
- `cp311`
- `cp312`
- `cp313`
- `cp314`

The `ge_python_onnx_plugin_native_wheel_matrix` target invokes `python_pass_native_build/build_python_native_matrix.py`, automatically detects `python3.9` to `python3.14` available in PATH/Conda, and rebuilds each version by reusing the compile/link metadata of the parent build. A version whose development headers or libpython are incomplete is allowed to be skipped while other available versions continue to be generated; the build result records the Python tags actually generated and skipped. `ge_python_onnx_plugin_native_wheel` is a single-version development target that produces only the artifact set and native wheel for the current `HI_PYTHON`.

#### 8.1.2 Wheel Split and Component Ownership

The release artifacts include:

```text
ge_py-<version>-py3-none-any.whl
ge_py_onnx_plugin_bridge-<version>-cpXY-<platform>.whl
```

Among them:

- The `ge_py` main wheel carries only the Python API, registry, bootstrap, loader, the shared `ge/_internal` layer, and fallback codegen resources; it no longer embeds `python_onnx_plugin_artifacts`.
- `ge_py_onnx_plugin_bridge` carries only `libge_python_onnx_plugin_bridge.so` and `_ge_onnx_plugin_native.so`, with the package name `ge-py-onnx-plugin-bridge`.
- The bridge and native module belong to the same artifact set and must use the same Python tag, platform, and `bridge_abi`; the current bridge ABI is 2, and artifacts with a mismatched ABI are rejected explicitly by manifest validation.
- The installation script selects the native wheel matching the current `python3` interpreter; when no wheel matches, that wheel is skipped and the fallback resources in the main wheel take over, without requiring a CANN reinstall.

#### 8.1.3 Artifact Directories

Precompiled and fallback-compiled artifacts are both written to:

```text
ge/onnx_plugin/python_onnx_plugin_artifacts/<python_tag>-<platform>/
├── manifest.json
├── libge_python_onnx_plugin_bridge.so
└── _ge_onnx_plugin_native.so
```

The fallback resources in the main wheel are located in `ge/onnx_plugin/fallback_codegen/`, generated by `gen_fallback_resources.py` at build time, and contain the bridge/native sources, headers, proto closure, and `build_config.json`. The manifest records `python_tag`, `python_version`, `platform`, `bridge_abi`, `build_python`, and `artifacts`; before loading, the JSON structure, artifact file existence, `python_tag`, platform, and ABI are validated.

#### 8.1.4 Fallback Responsibilities and Invocation Order

1. Python side: `ge.onnx_plugin._native` calls `ensure_native_module(SPEC)` in `ge/_internal/native_loader.py`; the loading order is `sys.modules` cache -> matching prebuilt artifact -> on-site compilation -> aggregated diagnostics.
2. C++ side: `EnsureLoaded()` in `onnx_plugin_bridge_loader.cc` first calls `TryLoadPrebuiltBridge()` and then `TryLoadFallbackBridge()` on failure; fallback invokes `run_fallback_codegen()` through the module name `ge.onnx_plugin.fallback_runtime`. It runs in-process when Python symbols are already loaded; otherwise, it invokes the current Python through a subprocess.
3. The shared compilation engine is located in `ge/_internal/fallback_runtime.py` and is responsible for parsing `build_config.json`, materializing fallback resources, invoking `CXX`/`c++`, and atomically publishing the artifacts and manifest.
4. Fallback requires the current Python environment to satisfy: `pybind11` (>=2.13.6,<3.0.0) is installed; the Python development headers and libpython are available; a C++ compiler is available; and the CANN `include`/`lib64`/`pkg_inc` directories can be resolved (after sourcing `set_env.sh` or within the run package directory structure). If any condition is not met, compilation fails and the specific reasons are aggregated in the diagnostics.
5. Fallback generates only its own artifact set and does not modify other component directories; on failure, it cleans up the temporary working directory and retains existing prebuilt artifacts.

#### 8.1.5 Runtime Selection and Compatibility Validation

- Both the Python side and the C++ side use the actual Python runtime key of the current process as the selection basis, and do not substitute the outer launcher Python version for the current process version.
- The ONNX plugin selects artifacts by Python tag, platform, and `bridge_abi`; artifacts with an incomplete manifest, missing files, or a mismatched ABI are not considered candidates.
- The bridge is loaded only when a loadable Python plugin entry exists in `ASCEND_CUSTOM_OPP_PATH`; C++-only custom operator directories are not affected.
- When no matching prebuilt artifact exists, fallback is attempted first; only when fallback is also unavailable does E19999 report the runtime key, available artifacts, loader/PYTHONPATH, and the suggested wheel name, and prompt checking `pybind11` (>=2.13.6,<3.0.0), the Python development headers, and the C++ compiler.
- Another CPython minor version is not brought up in the same process; after the bridge is loaded, the current process Python version is validated again.

#### 8.1.6 Version Compatibility Test Requirements

At least the following must be verified under every available Python tag:

- Offline installation and pip auto-selection of the main wheel and the `ge_py_onnx_plugin_bridge` native wheel;
- `import ge.onnx_plugin` and loading of the `OnnxNode` native type;
- ATC compilation, ACL execution, and result comparison with the sample plugins;
- On-site fallback compilation and loading after clearing prebuilt artifacts;
- Error diagnostics for mismatched Python tag, platform, or `bridge_abi`, and for unavailable fallback;
- C++-only custom operator directories (without Python plugin entries) do not trigger bridge loading.
