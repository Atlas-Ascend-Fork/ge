# Overall Constraints

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-26T12:49:30.035Z pushedAt=2026-10-28T07:42:19.751Z nl=2 -->

**The product support status of the parameters in this chapter is summarized as follows: if a parameter also specifies the specific products it supports, the actual support indicated in the parameter prevails.**

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/cpp/ge/options_params/overall_constraints_res.md#id1 -->

**This chapter lists the configuration parameters passed to the GEInitialize, Session constructor, and AddGraph API, which take effect at the global, session, and graph levels, respectively.**

> [!NOTE]
>
>[Basic Functions](./basic_functions.md)\~[Deprecated Configurations for Future Versions](./deprecated_config_for_future_versions.md) lists only the configuration parameters supported in the current version. If a parameter is not listed in the table, it is reserved or applies to AI processors of other versions, and you can ignore it.
