# aclopSetModelDir

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:57:55.282Z pushedAt=2026-10-29T03:58:31.197Z nl=2 -->

## Product Support

<!-- npu="950" id36 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id36 -->
<!-- npu="A3" id37 -->
- Atlas A3 products: Supported
<!-- end id37 -->
<!-- npu="910b" id38 -->
- Atlas A2 products: Supported
<!-- end id38 -->
<!-- npu="310b" id39 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id39 -->
<!-- npu="310p" id40 -->
- Atlas inference products: Supported
<!-- end id40 -->
<!-- npu="910" id41 -->
- Atlas training products: Supported
<!-- end id41 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetModelDir_res.md#id1 -->

## Feature Description

Sets the directory for loading the model file, which is compiled from a single operator into a \*.om file.

## Prototype

```c
aclError aclopSetModelDir(const char *modelDir)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDir | Input | Pointer to the model file path.<br>You can set a multi-level directory here, but the system reads model files from at most three directory levels, starting from the last level of the multi-level directory.<br>For example, if modelDir is set to "dir0/dir1" and the directory hierarchy under dir1 is "dir2/dir3/dir4", the system supports reading model files only from the "dir1", "dir1/dir2", and "dir1/dir2/dir3" directories. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

The operator library version in the model loading environment must be consistent with that in the model compilation environment; otherwise, an error is reported when loading operators. You can check the operator library version through the version field in the \$\{INSTALL\_DIR\}/opp/version.info file. \$\{INSTALL\_DIR\} must be replaced with the file storage path after CANN software installation. For example, when installed as the root user, the default file storage path is /usr/local/Ascend/cann.

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
Before loading, evaluate whether the memory space is sufficient based on the size of the single-operator om file. Insufficient memory space causes application exceptions. The maximum number of operators being executed concurrently in a process varies by product model:
<!-- end id1 -->

<!-- npu="950,A3,910b" id2 -->
- For Ascend 950PR&950DT products, Atlas A3 products, and Atlas A2 products, the maximum number is 2000000.
<!-- end id2 -->
<!-- npu="910,310b" id3 -->
- For Atlas 200I/500 A2 inference products and Atlas training products, the maximum number is 40000000.
<!-- end id3 -->
<!-- npu="310p" id4 -->
- For Atlas inference products, the upper limit is 40000000 in the Ascend EP form and 2000000 in the Control CPU open form.
<!-- end id4 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetModelDir_res.md#id2 -->
