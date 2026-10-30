# Overview

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-26T13:19:16.337Z pushedAt=2026-10-28T07:42:19.811Z nl=2 -->

[Session](Session.md) (the original API, also known as V1) and [GESession](../GeSession/GESession.md) (V2) are both graph compilation and execution APIs used to implement graph loading, graph compilation, and graph execution. However, the two sets of APIs cannot be used together. It is recommended to use the V2 API, as the V1 API will be gradually deprecated. For migration guidance from Session to GeSession, see [Session to GeSession Migration Guide](../../../../../user_guides/graph_dev/appendix/session_to_gesession_migration.md).

- The original Session contains both GE and DataFlow APIs. The GE-related APIs are now split into the **GeSession** class in the **ge\_api\_v2.h** header file to achieve clearer module division.
- In addition, the original APIs had similar names but significantly different functions, which have been unified and optimized. The original compilation APIs include **CompileGraph** and **BuildGraph**, and the execution APIs include **ExecuteGraphWithStreamAsync** and **RunGraphWithStreamAsync**. These APIs have clear functional differences, but their names are easily confused, and there are restrictions on their combined use. For example, **BuildGraph** cannot be used with **ExecuteGraphWithStreamAsync**, and **CompileGraph** cannot be used with **RunGraph**/**RunGraphAsync**.

In V2, the APIs are simplified and unified:

- Only one compilation API is retained: **CompileGraph**.
- The stream-based asynchronous execution API is also unified into one: **RunGraphWithStreamAsync**.
- The **inputs** and **outputs** parameters in the execution API uniformly use the **gert::Tensor** type, which offers better performance.

This adjustment aims to improve API clarity and usage consistency, avoiding functional exceptions caused by API misuse.

**Session API product support is as follows:**

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/cpp/ge/Session/overview_res.md#id1 -->
