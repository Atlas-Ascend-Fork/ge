# ONNX Graph Integration

<!-- md-trans-meta sourceCommit=1c0a36284515311df07ca620682d500844a67127 translatedAt=2026-10-21T09:40:03.754Z pushedAt=2026-10-28T03:43:00.477Z nl=2 -->

The ONNX frontend differs from PyTorch: the GE ONNX Parser directly reports an error for op types it does not recognize (PARAM\_INVALID). Therefore, an additional **ONNX parsing plugin** is required to map ONNX node attributes to GE operator attributes. A brief diagram of the deliverables on both sides is shown below:

![](../figures/onnx_into_graph.png)

1. **Stage 1: Operator Definition**

    In the operator definition stage, metadata such as the inputs, outputs, and attributes of the operator is declared for use in subsequent stages.

    **Implementation by frontend:**

    | Frontend Framework | Definition Method | Deliverable | Description |
    | --- | --- | --- | --- |
    | ONNX | NodeProto | ONNX model embedding | The model only declares the op semantics; the REG_OP header file on the GE side still needs to be written manually.<br> Note: The NodeProto embedded in the ONNX model is only the "usage declaration" of the op in the model. The GE side still requires op_graph/include/*.h (REG_OP) for the Parser to use when creating nodes. The two serve different purposes and cannot replace each other. |

2. **Stage 2: Frontend Integration (Parsing Plugin)**

    Map the frontend operator type to the GE operator type and handle attribute conversion.

    **Integration method by frontend:**

    | Frontend Framework | Access Method | Deliverables Involved | Key Point |
    | --- | --- | --- | --- |
    | ONNX | Parser parsing plugin | libonnx_parser_plugin.so | ParseParamsFn must be implemented manually |

    **ONNX operator type identifier format**: "domain::version::OpType" (for example, "ai.onnx::11::Conv" and "com.example::1::MyOp"). The Parser searches the OpRegistry for a mapping in this format. If no mapping is found, parsing fails.

    **The ONNX parsing plugin must:**

    1. Use REGISTER\_CUSTOM\_OP to register the mapping from ONNX op type to GE op type.
    2. Implement ParseParamsFn to extract attributes from the ONNX NodeProto and set them to the GE Operator.

    **Parsing plugin code:**

    ```c++
    // framework/onnx/add_custom_parser.cpp
    #include "register/register.h"
    #include "proto/onnx/ge_onnx.pb.h"

    // Attribute parsing function: map ONNX NodeProto attributes to GE Operator attributes
    // op_src: ONNX node Proto message body
    // op_dest: GE operator object, used to set attributes
    static domi::Status ParseMyOp(const google::protobuf::Message *op_src, ge::Operator &op_dest) {
      // Convert the Message to the ONNX NodeProto type
      auto *node = dynamic_cast<const ge::onnx::NodeProto *>(op_src);
      // Traverse all attributes of the ONNX node.
      for (const auto &attr : node->attribute()) {
        // Set the alpha attribute in ONNX to the GE operator.
        if (attr.name() == "alpha") {
          op_dest.SetAttr("alpha", attr.f());
        }
      }
      return domi::SUCCESS;
    }

    // Register the ONNX parsing plugin: map the ONNX op type to the GE op type.
    // When the ONNX Parser encounters an unrecognized operator, it uses this registration information to find the parsing plugin.
    REGISTER_CUSTOM_OP("MyCustomOp")
        .FrameworkType(domi::ONNX)                                    // Declare this as an ONNX framework plugin.
        .OriginOpType({"ai.onnx::11::MyOp", "ai.onnx::13::MyOp"})     // ONNX operator type (multiple versions supported).
        .ParseParamsFn(ParseMyOp);                                    // Attribute parsing callback function.

    ```

    **REGISTER\_CUSTOM\_OP Key methods:**

    | Method | Purpose |
    | --- | --- |
    | .FrameworkType(domi::ONNX) | Declares this as an ONNX framework plugin. |
    | .OriginOpType("domain::ver::Type") | Specifies the ONNX op type (supports a list, covering multiple versions). |
    | .ParseParamsFn(fn) | Attribute parsing function (Message* → Operator&). |
    | .ParseParamsByOperatorFn(fn) | Attribute parsing on the automatic mapping path (Operator → Operator). |
    | .ParseOpToGraphFn(fn) | Expands a single operator into a subgraph (optional). |
    | .ParseSubgraphPostFn(fn) | Post-processing for operators containing subgraphs (optional). |

    The ONNX scenario requires two .so files:

    | Deliverable | Responsibility | Placement path |
    | --- | --- | --- |
    | ONNX parsing plugin .so | REGISTER_CUSTOM_OP + ParseParamsFn, responsible for ONNX → GE attribute mapping | Plugin directory under ASCEND_CUSTOM_OPP_PATH |
    | GE custom operator .so | REG_OP + REG_AUTO_MAPPING_OP + execution logic | op_graph/lib/\<os>/\<arch>/ |

    **Deliverable organization example:**

    ```tree
    custom_opp/
    ├── op_graph/
    │   ├── include/add_custom.h    # REG_OP
    │   └── lib/linux/x86_64/libcust_opapi.so  # GE deliverable
    ├── framework/
    │   └── onnx/
    │       └── libonnx_parser_plugin.so       # Parsing plugin (standalone .so)
    ```

    For the unified deliverable specification, see [Deliverables and OPP Specifications](deliverables_and_opp.md).

    **Notes:**

    - The ONNX parsing plugin .so must be loaded through ASCEND\_CUSTOM\_OPP\_PATH before ATC compilation; otherwise, the Parser cannot recognize the custom op type.
    - When the domain in OriginOpType is empty, "ai.onnx" is used by default. A custom domain must be explicitly specified.
    - If an ONNX operator can be mapped to a combination of existing GE operators, it can be expanded into a subgraph through ParseOpToGraphFn, without the need to write a GE-side custom operator .so.

3. **Stage 3: Graph construction and integration**

    The graph construction and integration stage converts the frontend computation graph into a GE computation graph.

    **Graph construction paths by frontend:**

    | Frontend Framework | Graph Construction Path | Conversion Mechanism |
    | --- | --- | --- |
    | ONNX | ONNX Model → GE Graph | Parser |

    This stage is automatically completed by the framework, and no additional deliverables are required from developers. After graph construction is complete, the GE graph already contains the custom operator nodes.

4. **Stage 4: Graph Compilation**

    The callback interfaces for graph compilation are triggered during GE compilation and are used for shape inference, kernel compilation, and serialization. **Callback interfaces and trigger timing:**

    | Callback Interface | Trigger Time | Applicable Scenarios |
    | --- | --- | --- |
    | InferShape/InferDataType | Shape/DataType inference at compile time | All scenarios (optional) |
    | Compile | RTC kernel compilation at compile time | Online compilation/offline OM scenario |
    | Serialize | Writing to OM during ATC compilation | Offline OM scenario |

    For signatures, context APIs, and typical flows, see [Callback Interface](callback_interface.md).

5. **Stage 5: Graph Execution**

    This stage is the core of the custom operator. GE calls back the Execute interface during execution.

    **Callback interfaces and trigger timing:**

    | Callback Interface | Trigger Time | Applicable Scenarios |
    | --- | --- | --- |
    | Execute | Executes the kernel at runtime | All scenarios (must be implemented) |
    | Deserialize | Restores the binary when OM is loaded | Offline OM scenario |
    | UpdateHostArgs | Refreshes when the I/O address changes | Static graph offloading scenario |

    For detailed examples of the callback interfaces, see [Callback Interface](callback_interface.md).

**Summary: ONNX graph construction stages and deliverables involved**

| Stage | Deliverables Involved | Description |
| --- | --- | --- |
| Stage 1: Operator definition | ONNX model embedding | Provided by NodeProto |
| Stage 2: Frontend integration | framework/onnx/libonnx_parser_plugin.so | Parsing plugin |
| Stage 3: Graph construction and integration | None | Automatically converted by the Parser |
| Stage 4: Graph compilation | op_graph/lib/<os\>/<arch\>/*.so | Callback implemented in the GE deliverable |
| Stage 5: Graph execution | op_graph/lib/<os\>/<arch\>/*.so | Callback implemented in the GE deliverable |

**Additional requirements:**

- op\_graph/include/\*.h (REG\_OP) is used by the Parser when creating nodes.
- The parsing plugin and the GE deliverable must be loaded simultaneously.
