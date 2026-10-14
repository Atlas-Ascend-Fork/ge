# --save\_original\_model

## 产品支持情况

全量芯片支持

## 功能说明

控制模型转换时是否同时输出原始模型文件。

设置为true时，除生成[--output](--output.md)参数指定的om离线模型文件外，还会额外生成一个保存图优化前网络的om模型文件，文件名为“--output参数值\_original.om”，保存在与--output参数相同的输出路径下，可用于模型调试与问题定位。

## 关联参数

需要与[--output](--output.md)参数配合使用，原始模型文件的名称与保存路径基于--output参数值生成。

## 参数取值

- true：输出原始模型。
- false：（默认值）不输出原始模型。

## 推荐配置及收益

无。

## 示例

```bash
atc --save_original_model=true ...
```
