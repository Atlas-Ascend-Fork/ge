# aclmdlExecConfigAttr

<!-- md-trans-meta sourceCommit=6910bdd915e3528be24078d698c2568ed03b113b translatedAt=2026-10-27T15:05:31.894Z pushedAt=2026-10-28T13:15:54.105Z nl=2 -->

```c
typedef enum {
    ACL_MDL_STREAM_SYNC_TIMEOUT = 0,
    ACL_MDL_EVENT_SYNC_TIMEOUT,
    ACL_MDL_WORK_ADDR_PTR,
    ACL_MDL_WORK_SIZET,
    ACL_MDL_MPAIMID_SIZET,      /** param reserved */
    ACL_MDL_AICQOS_SIZET,       /** param reserved */
    ACL_MDL_AICOST_SIZET,       /** param reserved */
    ACL_MDL_MEC_TIMETHR_SIZET   /** param reserved */
} aclmdlExecConfigAttr;
```

**Table 1**  Enum member description

| Enum Member | Description |
| --- | --- |
| ACL_MDL_STREAM_SYNC_TIMEOUT | Controls the timeout of Stream tasks during model inference. The attribute value is of the INT32 type.<br>The value description is as follows:<br><br>  - -1: Indicates waiting forever, which is the default.<br>  - >0: Configures a specific timeout in milliseconds. |
| ACL_MDL_EVENT_SYNC_TIMEOUT | Controls the timeout of Event tasks during model inference. The attribute value is of the INT32 type.<br>The value description is as follows:<br><br>  - -1: Indicates waiting forever, which is the default.<br>  - >0: Configures a specific timeout in milliseconds. |
| ACL_MDL_WORK_ADDR_PTR | Pointer to the working memory required by the model (temporary data stored on the Device during model execution). The working memory is managed by the user. This is generally used in scenarios where a model is loaded once and executed concurrently multiple times.<br>If both ACL_MDL_WORK_ADDR_PTR and ACL_RT_STREAM_WORK_ADDR_PTR in aclrtStreamConfigAttr (which indicates the working memory of the model on the Stream) are configured, ACL_MDL_WORK_ADDR_PTR takes precedence. |
| ACL_MDL_WORK_SIZET | Size of the working memory required by the model, in bytes. This is generally used in scenarios where a model is loaded once and executed concurrently multiple times. |
| ACL_MDL_MPAIMID_SIZET | Reserved configuration. |
| ACL_MDL_AICQOS_SIZET | Reserved configuration. |
| ACL_MDL_AICOST_SIZET | Reserved configuration. |
| ACL_MDL_MEC_TIMETHR_SIZET | Reserved configuration. |

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
Note that currently only ACL\_MDL\_STREAM\_SYNC\_TIMEOUT and ACL\_MDL\_EVENT\_SYNC\_TIMEOUT are supported.
<!-- end id1 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlExecConfigAttr_res.md#id1 -->
