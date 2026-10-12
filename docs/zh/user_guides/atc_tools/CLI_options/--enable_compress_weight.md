# --enable\_compress\_weight

## 产品支持情况

全量芯片支持

## 功能说明

是否开启权重压缩。

开启后，模型转换时对网络中支持压缩的算子权重进行压缩处理，减小生成的om离线模型文件体积，降低推理时的权重读取带宽。

## 关联参数

该参数设置为true时，不能同时通过[--compress\_weight\_conf](--compress_weight_conf.md)参数指定权重压缩配置文件，两者同时设置时模型转换报错。

## 参数取值

- true：开启权重压缩。
- false：（默认值）关闭权重压缩。

## 推荐配置及收益

无。

## 示例

```bash
atc --enable_compress_weight=true ...
```
