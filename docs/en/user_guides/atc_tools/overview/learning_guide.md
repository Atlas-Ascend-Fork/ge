# Learning Guide

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:26:09.938Z pushedAt=2026-10-28T09:30:32.170Z nl=2 -->

This document describes how to use ATC (Ascend Tensor Compiler) to convert models and obtain offline models that adapt to AI processors. After reading this document, you will be able to achieve the following goals:

- Understand how to convert original network models of different frameworks into offline models for AI processors.
- Convert models into offline models that meet different customization requirements based on the parameters described in this document.

Personnel who are familiar with basic Linux commands and have a certain understanding of machine learning and deep learning can better understand this document.

## For Beginners

| [ATC Overview](atc_overview.md) | [Quick Start](quick_start.md) | [Basic Features](../basic_feature/basic_features.md) |
| --- | --- | --- |
| Describes the functional architecture, running process, and key concepts of ATC. | Using the network models of all frameworks supported by ATC as examples, this section briefly describes how to perform model conversion for basic features. | Describes the basic features of ATC, such as converting a model into a JSON file to view parameter information, supporting dynamic batch size and dynamic resolution for offline models, and combining various ATC parameters to convert models into offline models that meet requirements. |

## For Experts

| [Enabling AIPP](../AIPP/enable_aipp.md) | [Single Operator Model Conversion](../sinlgeop_model_convert/sinlgeop_model_convert.md) | [Special Topics](../custom_network_modify_caffe/special_topics.md) |
| --- | --- | --- |
| Describes what AIPP is, how to enable AIPP during model conversion, how to construct an AIPP configuration file based on the configuration file template, and how to output image data that meets requirements based on the color space conversion function, and provides configuration examples in typical scenarios. | Describes what a single operator description file is, how to construct a single operator description file, and how to convert the file into an offline model adapted to the AI processor for verifying single operator functions. | Describes how to customize and modify models such as Caffe and TensorFlow models in scenarios where the ATC tool cannot be directly used, so that the ATC tool can be used smoothly. |
