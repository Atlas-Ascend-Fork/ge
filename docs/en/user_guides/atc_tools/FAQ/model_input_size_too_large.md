# Quantization Model Fails Because the Model Input Size Is Too Large and the AI Core Fails to Execute the Task

<!-- md-trans-meta sourceCommit=455d9ca7b1ddac7f3eb8a6a22c869e2b528dd59c translatedAt=2026-10-27T14:21:46.247Z pushedAt=2026-10-28T08:42:03.125Z nl=2 -->

## Problem Description

The following is an example of the model conversion command. The model input size is related to the shape specified by the input\_shape parameter:

```bash
atc --model=xxxxxx.pb --framework=3 --output=xxxxxx --soc_version=xxxxxx --input_shape="input:64,224,224,3" --input_format=NHWC --compression_optimize_conf=config/quant.cfg
```

During model conversion, the following error is reported:

```console
[ERROR] AMCT(757013,atc.bin):2023-03-14-14:15:54[model_process.cpp:299]execute model failed, modelId is 1, errorCode is 507011
[ERROR] AMCT(757013,atc.bin):2023-03-14-14:15:54[sample_process.cpp:320]execute inference failed
[ERROR] AMCT(757013,atc.bin):2023-03-14-14:15:55[sample_process.cpp:275]ACL model infer failed.
[ERROR] AMCT(757013,atc.bin):2023-03-14-14:15:55[quantize_api.cpp:242]sample process failed
[ERROR] AMCT(757013,atc.bin):2023-03-14-14:15:55[quantize_api.cpp:378]Do Calibration failed.
[ERROR] AMCT(757013,atc.bin):2023-03-14-14:15:56[inner_graph_calibration.cpp:77]Failed to execute InnerQuantizeGraph.
ATC run failed, Please check the detail log, Try 'atc --help' for more information
EZ9999: Internal Error!
EZ9999  Aicore kernel execute failed, device_id=0, stream_id=11, report_stream_id=2, task_id=209, flip_num=0, fault kernel_name=17786444594805609729-1_0_1_vgg_16/conv1/conv1_2/Conv2D_histo, program id=206, hash=1846532111878224358.[FUNC:GetError][FILE:stream.cc][LINE:1131]
        TraceBack (most recent call last):
        Model synchronize execute failed, model_id=1![FUNC:GetStreamToSyncExecute][FILE:model.cc][LINE:630]
        rtModelExecute execute failed, reason=[the model stream execute failed][FUNC:FuncErrorReason][FILE:error_message_manage.cc][LINE:49]
        [Exec][Model]Execute model failed, ge result[507011], modelId[1][FUNC:ReportCallError][FILE:log_inner.cpp][LINE:161]
        [Exec][Model]modelId[1] execute failed, result[507011][FUNC:ReportInnerError][FILE:log_inner.cpp][LINE:145]
        An unknown error occurred. Please check the log.
```

## Possible Causes

The AI Core failed to execute the task. This may be because the Batch size value at the input\_shape parameter is too large, causing the operator on the AI Core to fail to execute.

## Solution

You can reduce the Batch size value at the input\_shape parameter, for example: --input\_shape="**input:8,224,224,3**". After adjusting the parameter value, convert the model again.
