# System Error "nca load model from file failed, ret = 254."

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:22:41.674Z pushedAt=2026-10-28T08:42:11.727Z nl=2 -->

## Symptoms

During quantization, if the following logs appear, the group information may not have been obtained.

```console
[INFO][ncx_interface.cpp:35] ncx lib has been init
[INFO][ncx_interface.cpp:35] ncx lib has been init
[ERROR][ncx_interface.cpp:208] nca load model from file failed, ret = 254. please check detail info from device log.
```

Based on the preceding ERROR information, check “/home/mdc/var/log/debug/TOOLS\_\*.log” for further location. If the following information appears, it can be determined that the error is caused by the NCS failing to create a group because it did not obtain the group information.

```console
[INFO][ncx_interface.cpp:35] ncx lib has been init
[INFO][ncx_interface.cpp:35] ncx lib has been init
[ERROR][ncx_interface.cpp:208] nca load model from file failed, ret = 254. please check detail info from device log.
[ERROR] AMCT(1372344,acl_test_tool):2025-09-12-08:26:47[sample_process.cpp:378]NCX load model failed.
[ERROR] AMCT(1372344,acl_test_tool):2025-09-12-08:26:47[sample_process.cpp:442]Ncx execute failed.
[ERROR] AMCT(1372344,acl_test_tool):2025-09-12-08:26:47[sample_process.cpp:297]Ncx model infer failed.
[ERROR] AMCT(1372344,acl_test_tool):2025-09-12-08:26:47[quantize_api.cpp:268]sample process failed
[ERROR] AMCT(1372344,acl_test_tool):2025-09-12-08:26:47[quantize_api.cpp:314]Calibration Inference failed!
[ERROR] AMCT(1372344,acl_test_tool):2025-09-12-08:26:47[quantize_api.cpp:439]Do Calibration failed.
[ERROR][ncx_interface.cpp:168] nca close session: amct ncx lib has not been init
[ERROR][ncx_interface.cpp:135] nca finalize: amct ncx lib has not been init
[ERROR] AMCT(1372344,acl_test_tool):2025-09-12-08:26:47[sample_process.cpp:518]NCX finalize failed.
[ERROR] AMCT(1372344,acl_test_tool):2025-09-12-08:26:47[sample_process.cpp:460]NCX destroy resource failed.
[ERROR] AMCT(1372344,acl_test_tool):2025-09-12-08:26:47[quantize_api.cpp:184]sample process destroy resource failed!
[ERROR] AMCT(1372344,acl_test_tool):2025-09-12-08:26:47[inner_graph_calibration.cpp:73]Failed to finalize CalibrationInitializer.
```

## Solution

Ensure that a capability group has been created. To create a group:

Call the dcmi\_create\_capability\_group API provided by DCMI to create a group.
