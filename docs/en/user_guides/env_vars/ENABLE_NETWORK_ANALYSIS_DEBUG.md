# ENABLE\_NETWORK\_ANALYSIS\_DEBUG

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-13T09:58:32.387Z pushedAt=2026-11-16T06:11:08.644Z nl=2 -->

## Feature Description

In TensorFlow training scenarios, when computation graph compilation fails, the training process is terminated by default and the remaining graphs are not delivered to the Device. If developers want the training process to continue and allow the TF Adapter to keep delivering computation graphs to the Device when graph compilation fails, they can set this environment variable.

> [!NOTE]Note
>
> In the current version, the value of this environment variable is not restricted. As long as this environment variable exists in the environment, the training process will not be terminated when graph compilation fails.

## Configuration Example

```bash
export ENABLE_NETWORK_ANALYSIS_DEBUG=1
```

## Constraints

Currently, this environment variable supports only the TensorFlow training scenario.

## Applicable Products

<!-- npu="950" id1 -->
Ascend 950PR&950DT products: Supported
<!-- end id1 -->

<!-- npu="A3" id2 -->
Atlas A3 products: Supported
<!-- end id2 -->

<!-- npu="910b" id3 -->
Atlas A2 products: Supported
<!-- end id3 -->

<!-- npu="910" id4 -->
Atlas training products: Supported
<!-- end id4 -->

<!-- npu="310b" id5 -->
Atlas 200I/500 A2 inference products: Not supported
<!-- end id5 -->

<!-- npu="310p" id6 -->
Atlas inference products: Not supported
<!-- end id6 -->
