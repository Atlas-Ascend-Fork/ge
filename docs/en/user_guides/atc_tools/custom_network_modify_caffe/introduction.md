# Overview

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:10:32.546Z pushedAt=2026-10-27T14:41:44.681Z nl=2 -->

The modifications in this section apply only to Caffe network models.

<!-- npu="950" id1 -->
The Ascend 950PR&950DT products do not support the Caffe framework: the Caffe framework is no longer evolving in this product form, and its functionality is not guaranteed.
<!-- end id1 -->
<!-- npu="A3" id2 -->
The Atlas A3 products do not support the Caffe framework: the Caffe framework is no longer evolving in this product form, and its functionality is not guaranteed.
<!-- end id2 -->
<!-- npu="910b" id3 -->
The Atlas A2 products do not support the Caffe framework: the Caffe framework is no longer evolving in this product form, and its functionality is not guaranteed.
<!-- end id3 -->
<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/custom_network_modify_caffe/introduction_res.md#id1 -->

Network operators can be classified into the following categories:

- Standard operators: Caffe standard operators supported by the AI processor, such as Convolution.
- Extended operators: public operators supported by the AI processor that are not Caffe standard operators. They fall into two types:
  - One type consists of operators custom-extended based on the Caffe framework, such as ROIPooling in Faster RCNN and the normalization operator Normalize in SSD.
  - The other type consists of custom operators originating from other deep learning frameworks, such as Passthrough in YOLOv2.

Network models such as Faster RCNN and SSD contain some operator structures that are not defined in the original Caffe framework, such as ROIPooling, Normalize, PSROIPooling, and Upsample. To enable the AI processor to support these networks, the original Caffe framework network models need to be extended, which reduces the workload for developers to develop custom operators and post-processing code. If a developer's Caffe framework network model uses these extended operators, the definitions of the extended layers must be modified or added in the prototxt file before model conversion can be performed successfully.

This section provides the list of extended operators supported by the AI processor and describes how to modify the prototxt file based on the extended operators.
