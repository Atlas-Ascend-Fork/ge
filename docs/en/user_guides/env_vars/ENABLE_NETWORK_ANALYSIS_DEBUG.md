# ENABLE\_NETWORK\_ANALYSIS\_DEBUG

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-20T12:50:34.565Z pushedAt=2026-10-28T11:30:50.521Z nl=2 -->

## Feature Description

In TensorFlow training scenarios, when computation graph compilation fails, the training process is terminated by default and the remaining graphs are not delivered to the Device. If developers want the training process to continue and allow the TF Adapter to keep delivering computation graphs to the Device when graph compilation fails, they can set this environment variable.

> [!NOTE]
>
> In the current version, the value of this environment variable is not restricted. As long as this environment variable exists in the environment, the training process will not be terminated when graph compilation fails.

## Configuration Example

```bash
export ENABLE_NETWORK_ANALYSIS_DEBUG=1
```

## Usage Constraints

Currently, this environment variable supports only the TensorFlow training scenario.

## Applicable Products

<!-- npu="950" id1 -->
Ascend 950PR and 950DT products: Supported
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
