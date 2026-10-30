# Overview

<!-- md-trans-meta sourceCommit=9f2ea985eb7ee6374346cc894560d663152ee14c translatedAt=2026-10-15T04:17:21.166Z pushedAt=2026-10-16T10:53:07.720Z -->

The **Session** class is used to manage the execution session of a graph.

Example of session execution:

```python
from ge.session import Session
from ge.graph import Tensor
from ge.ge_global import GeApi

# Initialize GE.
config = {
    "ge.execDeviceId": "0",
    "ge.graphRunMode": "0"
}
GeApi.ge_initialize(config)


# Create a session.
session = Session()

# Add a graph.
session.add_graph(0, graph)

# Prepare the input.
input_tensor = Tensor(data=[1.0, 2.0, 3.0, 4.0], data_type=DataType.DT_FLOAT, format=Format.FORMAT_ND, shape=[2, 2])

# Run the graph.
outputs = session.run_graph(0, [input_tensor])

# Finalize GE.
GeApi.ge_finalize()
```

**Applicable products of the Session API are as follows:**

<!-- npu="950" id1 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3 products: Supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 products: Supported
<!-- end id3 -->
<!-- npu="310b" id4 -->
- Atlas 200I/500 A2 inference products: Not supported
<!-- end id4 -->
<!-- npu="310p" id5 -->
- Atlas inference products: Supported
<!-- end id5 -->
<!-- npu="910" id6 -->
- Atlas training products: Supported
<!-- end id6 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/python/ge/session/Session/overview_res.md#id1 -->
