# Overview

<!-- md-trans-meta sourceCommit=6d5a35469af253d7c5b6ff894beab0546bf42c74 translatedAt=2026-10-15T03:59:24.541Z pushedAt=2026-10-16T10:53:07.638Z -->

A pattern graph wrapper used to match subgraphs in a target graph. It is typically constructed through **create\_pattern**.

**Constraints**

- **\_\_init\_\_\(graph\)** moves the **graph** out of the Python wrapper and invalidates it. Do not continue using the **Graph** variable after construction is complete.
- **release\(\)** is an internal method of the GE bridge layer. Python Pass developers do not need to call it directly. After **patterns\(\)** returns the **Pattern**, the framework automatically calls **release\(\)** to hand the native Pattern\* over to the C++ pipeline.
- **capture\_tensor** accepts a **TensorHolder**/**Node** with an index, or a **NodeIo**-style object (with **node**/**index** attributes), in which case the **index** parameter is left as **None**.
