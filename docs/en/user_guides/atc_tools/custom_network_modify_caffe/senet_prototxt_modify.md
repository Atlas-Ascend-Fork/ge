# Modifying the SENet Network Model prototxt File

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:13:09.532Z pushedAt=2026-10-28T02:47:07.890Z nl=2 -->

> [!NOTE]
>
> None of the code samples in this section can be directly copied into a network model. You need to adjust the corresponding parameters based on the network model you are using. For example, the parameters in bottom and top must correspond one-to-one with the bottom and top in the specific network model, and the order of the parameters corresponding to bottom and top cannot be changed.

The Axpy operator in this network model needs to be replaced with three operators: Reshape, Scale, and Eltwise. The modified sample is as follows:

![Illustration](../figures/extended_op_senet.png)

The modified code sample is as follows:

```textproto
layer {
  name: "conv3_1_axpy_reshape"
  type: "Reshape"
  bottom: "conv3_1_1x1_up"
  top: "conv3_1_axpy_reshape"
  reshape_param {
    shape {
      dim: 0
      dim: -1
    }
  }
}
layer {
  name: "conv3_1_axpy_scale"
  type: "Scale"
  bottom: "conv3_1_1x1_increase"
  bottom: "conv3_1_axpy_reshape"
  top: "conv3_1_axpy_scale"
  scale_param {
    axis: 0
    bias_term: false
  }
}
layer {
  name: "conv3_1_axpy_eltwise"
  type: "Eltwise"
  bottom: "conv3_1_axpy_scale"
  bottom: "conv3_1_1x1_proj"
  top: "conv3_1"
}
```

For the parameter explanations of the Reshape, Scale, and Eltwise operators, see *AI Framework Operator Support List*\>Supported Caffe Operator List.
