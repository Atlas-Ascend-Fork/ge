# Modifying the SSD Network Model prototxt

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:14:35.952Z pushedAt=2026-10-28T03:04:25.569Z nl=2 -->

> [!NOTE]
>
> None of the code samples in this chapter can be directly copied into a network model. You need to adjust the corresponding parameters based on the network model you are using. For example, the parameters in bottom and top must correspond one-to-one with the bottom and top in the specific network model, and the order of the parameters corresponding to bottom and top cannot be changed.

For an SSD network, refer to [Extended Operator List](extended_operator_list.md) and add the post-processing operator layer SSDDetectionOutput at the end of the original prototxt file.

See the caffe.proto file (the file path is $\{INSTALL\_DIR\}/include/proto). First, add the declaration of the custom layer parameters in the LayerParameter message (the following custom layer has already been declared in caffe.proto, so you do not need to add it again):

```c++
message LayerParameter {
...
  optional SSDDetectionOutputParameter ssddetectionoutput_param = 232;
...
}
```

See the caffe.proto file. The Type and attributes of this operator are defined as follows:

```c++
message SSDDetectionOutputParameter {
    optional int32 num_classes= 1 [default = 2];
    optional bool share_location = 2 [default = true];
    optional int32 background_label_id = 3 [default = 0];
    optional float iou_threshold = 4 [default = 0.45];
    optional int32 top_k = 5 [default = 400];
    optional float eta = 6 [default = 1.0];
    optional bool variance_encoded_in_target = 7 [default = false];
    optional int32 code_type = 8 [default = 2];
    optional int32 keep_top_k = 9 [default = 200];
    optional float confidence_threshold = 10 [default = 0.01];
}
```

See *AI Framework Operator Support List* \> Supported Caffe Operator List. The SSDDetectionOutput operator has 3 inputs and 2 outputs. Based on the preceding principles, the constructed code sample is as follows:

```textproto
layer {
  name: "detection_out"
  type: "SSDDetectionOutput"
  bottom: "bbox_delta"
  bottom: "score"
  bottom: "anchors"
  top: "out_boxnum"
  top: "y"
  ssddetectionoutput_param {
    num_classes: 2
    share_location: true
    background_label_id: 0
    iou_threshold: 0.45
    top_k: 400
    eta: 1.0
    variance_encoded_in_target: false
    code_type: 2
    keep_top_k: 200
    confidence_threshold: 0.01
  }
}
```

- In the bottom input, bbox\_delta corresponds to mbox\_loc in the original Caffe network, score corresponds to mbox\_conf\_flatten in the original Caffe network, and anchors corresponds to mbox\_priorbox in the original Caffe network. The value of num\_classes must be consistent with the value in the original network model.
- For the top output in the multi-batch scenario:
  - The output shape of out\_boxnum is \(batchnum,8\), where the first value of each batchnum is the actual number of boxes.
  - The output shape of y is \(batchnum,len,8\), where len is the value of keep\_top\_k after alignment to 128 (for example, if batch is 2 and keep\_top\_k is 200, the final output shape is \(2,256,8\)), and the first 256\*8 data items are the result of the first batch.

For parameter explanation, see *AI Framework Operator Support List* \> supported Caffe operator list.
