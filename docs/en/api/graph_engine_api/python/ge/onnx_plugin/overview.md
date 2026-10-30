# Overview

<!-- md-trans-meta sourceCommit=500eefa22ce54caa25b8e15e68d19940c2524ed6 translatedAt=2026-10-15T03:37:09.306Z pushedAt=2026-10-16T10:53:07.569Z -->

`ge.onnx_plugin` provides a Python interface for writing ONNX custom parsing plugins. A plugin developer declares the mapping from an ONNX origin operator to a GE target operator and binds a parse callback, and can then use Python to convert an ONNX node into a GE Operator. This corresponds to the `REGISTER_CUSTOM_OP` registration and the `ParseParamsFn` callback implementation in a C++ plugin.

Plugin files are placed under the path specified by the environment variable `ASCEND_CUSTOM_OPP_PATH`, and are automatically discovered and registered by the parser during the initialization phase. This environment variable supports multiple paths separated by `:`. Each path can be a single `.py` file or a directory. In directory mode, only the direct `.py` files and subpackages within it are loaded, and files or directories whose names start with `_` are skipped. The same origin type (`domain::opset::operator type`, for example `ai.onnx::11::Elu`) can be provided by only one plugin. When a C++ plugin conflicts with a Python plugin, the C++ plugin is retained. When Python plugins conflict with each other, parser initialization fails.

For a complete runnable plugin example, see [ONNX Plugin Example](../../../../../../../examples/onnx_plugin/README.md).

The module members are as follows:

- [`onnx_plugin`](onnx_plugin.md): Creates a plugin descriptor.
- [`OnnxPlugin`](OnnxPlugin/overview.md): Plugin descriptor, used to bind parse callbacks.
- [`OnnxNode`](OnnxNode/overview.md): Read-only value object of an ONNX node, passed in by GE as a callback argument.

The following example registers an Elu plugin, reads the `alpha` attribute of the ONNX node, and writes it to the target operator:

```python
from ge.graph import Operator
from ge.onnx_plugin import OnnxNode, onnx_plugin


elu = onnx_plugin(
    source="Elu",
    domain="ai.onnx",
    opsets=range(8, 19),
    target="Elu",
)


@elu.parse_node
def parse_elu(node: OnnxNode, target: Operator) -> None:
    target.set_attr("alpha", node.attrs.get("alpha", 1.0))
```
