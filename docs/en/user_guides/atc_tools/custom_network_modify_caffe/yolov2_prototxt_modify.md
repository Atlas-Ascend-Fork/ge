# Modifying the YOLOv2 Network Model prototxt

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:15:06.542Z pushedAt=2026-10-28T03:23:11.132Z nl=2 -->

> [!NOTE]
>
> None of the code samples in this section can be directly copied into a network model. You need to adjust the corresponding parameters based on the network model you are using. For example, the parameters in bottom and top must correspond one-to-one with the bottom and top in the specific network model, and the order of the parameters corresponding to bottom and top cannot be changed.

1. Modify the Region operator.

    Because the Yolo operator plus the DetectionOutput operator constitute the complete post-processing logic of the feature detection network, you need to replace the Region process operator with the Yolo operator before adding the YoloV2DetectionOutput operator.

    See *AI Framework Operator Support List* \> Supported Caffe Operator List. The Yolo operator has one input and three outputs. Based on this principle, the modified Yolo operator is as follows. The comparison is shown in [Figure 1](#fig1), where the left is the prototxt of the original operator and the right is the prototxt adapted to the AI processor.

    **Figure 1** Original operator and modified operator comparison 2<a id="fig1"></a>
    ![Illustration](../figures/original_vs_modified_op_2.png "Original operator and modified operator comparison 2")

    The constructed code sample is as follows:

    ```textproto
    layer {
     bottom: "layer31-conv"
     top: "yolo_coords"
     top: "yolo_obj"
     top: "yolo_classes"
     name: "yolo"
     type: "Yolo"
     yolo_param {
      boxes: 5
      coords: 4
      classes: 80
      yolo_version: "V2"
      softmax: true
      background: false
        }
    }
    ```

    For parameter description, see *AI Framework Operator Support List* \> Supported Caffe Operator List.

2. Add the YoloV2DetectionOutput operator to the last layer.

    For the YoloV2 network, refer to [Extended Operator List](extended_operator_list.md) and add the post-processing operator layer YoloV2DetectionOutput to the end of the original prototxt file. See *AI Framework Operator Support List* \> Supported Caffe Operator List. The YoloV2DetectionOutput operator has four inputs and two outputs. Based on the preceding principle, the constructed post-processing operator code sample is as follows:

    ```textproto
    layer {
    name: "detection_out2"
    type: "YoloV2DetectionOutput"
    bottom: "yolo_coords"
    bottom: "yolo_obj"
    bottom: "yolo_classes"
    bottom: "img_info"
    top: "box_out"
    top: "box_out_num"
    yolov2_detection_output_param {
      boxes: 5
      classes: 80
      relative: true
      obj_threshold: 0.5
      score_threshold: 0.5
      iou_threshold: 0.45
      pre_nms_topn: 512
      post_nms_topn: 1024
      biases: 0.572730
      biases: 0.677385
      biases: 1.874460
      biases: 2.062530
      biases: 3.338430
      biases: 5.474340
      biases: 7.882820
      biases: 3.527780
      biases: 9.770520
      biases: 9.168280
      }
    }
    ```

    For parameter description, see *AI Framework Operator Support List* \> Supported Caffe Operator List.

3. Add an input:

    Since the YoloV2DetectionOutput operator has img\_info inputs, this input is added when the model is input. The comparison is shown in [Figure 2](#fig2), where the left side is the prototxt of the original operator, and the right side is the prototxt adapted to the AI processor.

    **Figure 2**  Original operator and modified operator comparison 1<a id="fig2"></a>
    ![Illustration](../figures/original_vs_modified_op_1_2.png "Original operator and modified operator comparison 1-2")

    The code sample is as follows. The parameter is \[batch,4\], where 4 indicates the four dimensions netH, netW, scaleH, and scaleW. netH and netW are the H and W of the network model input, and scaleH and scaleW are the H and W of the original image.

    ```textproto
    input: "img_info"
    input_shape {
      dim: 1
      dim: 4
    }
    ```
