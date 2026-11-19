# --weight

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-16T14:43:11.592Z pushedAt=2026-11-17T12:22:50.378Z nl=2 -->

## Product Support

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--weight_res.md#id1 -->

<!-- npu="950" id2 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id2 -->

<!-- npu="A3" id3 -->
- Atlas A3 products: Not supported
<!-- end id3 -->

<!-- npu="910b" id4 -->
- Atlas A2 products: Not supported
<!-- end id4 -->

<!-- npu="310b" id5 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id5 -->

<!-- npu="310p" id6 -->
- Atlas inference products: Supported
<!-- end id6 -->

<!-- npu="910" id7 -->
- Atlas training products: Supported
<!-- end id7 -->

<!-- npu="IPV350" id1 -->
- IPV350: not support
<!-- end id1 -->

## Function Description

Path and name of the weight file of the original network model. This parameter must be specified when the original network model is Caffe.

## Related Parameters

When the original model is in the Caffe framework, this parameter must be used in conjunction with [--model](--model.md).

## Parameter Values

**Parameter value:**
Path and file name of the weight file.

**Parameter value format:**
Path and file name: supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --mode=0 --model=$HOME/module/resnet50.prototxt --weight=$HOME/module/resnet50.caffemodel --framework=0 --soc_version=<soc_version>  --output=$HOME/module/out/caffe_resnet50 ...
```

## Dependency Constraints

None.
