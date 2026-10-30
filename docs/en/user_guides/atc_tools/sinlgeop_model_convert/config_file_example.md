# Sample Configuration File

<!-- md-trans-meta sourceCommit=73cc22fa193c049e7877714b651cc02a774ef134 translatedAt=2026-10-27T14:35:10.408Z pushedAt=2026-10-28T10:36:20.153Z nl=2 -->

## Configuring a Single Operator Description File

The single operator description file varies with different inputs or Format scenarios. This section provides configuration examples for each scenario.

The single operators in this section are defined based on Ascend IR, and the description file is in JSON format. For details about the parameters in the JSON description file, see [Table 1](singleop_desc_intro.md#table1). For details about the Ascend IR definition of single operators, see *[Operator Library](https://hiascend.com/document/redirect/CannCommunityOplist)*\> "Ascend IR Operator Specification".

- **When Format is ND:**

    In this example, the converted offline model of the single operator is add.om.

    ```json
    [
        {
          "op": "Add",
          "name": "add",
          "input_desc": [
            {
              "format": "ND",
              "shape": [3,3],
              "type": "int32"
            },
            {
              "format": "ND",
              "shape": [3,3],
              "type": "int32"
            }
          ],
          "output_desc": [
            {
              "format": "ND",
              "shape": [3,3],
              "type": "int32"
            }
          ]
        }
    ]
    ```

- **When Format is NCHW:**

    In this example, the converted offline model of the single operator is conv2d.om.

    ```json
    [
     {
       "op": "Conv2D",
       "name": "conv2d",
       "input_desc": [
         {
           "format": "NCHW",
           "shape": [1, 3, 16, 16],
           "type": "float16"
         },
         {
           "format": "NCHW",
           "shape": [3, 3, 3, 3],
           "type": "float16"
         }
       ],
       "output_desc": [
         {
           "format": "NCHW",
           "shape": [1, 3, 16, 16],
           "type": "float16"
         }
       ],
       "attr": [
           {
             "name": "strides",
             "type": "list_int",
             "value": [1, 1, 1, 1]
           },
           {
             "name": "pads",
             "type": "list_int",
             "value": [1, 1, 1, 1]
           },
           {
             "name": "dilations",
             "type": "list_int",
             "value": [1, 1, 1, 1]
           }
       ]
     }
     ]
    ```

- **When the Format used during Tensor computation differs from the original Format**

    During ATC model conversion, **origin\_format** and **origin\_shape** are converted into the **format** and **shape** required by the offline model.

    In this example, the converted offline model of the single operator is add.om.

    ```json
    [
      {
        "op": "Add",
        "name": "add",
        "input_desc": [
          {
            "format": "NC1HWC0",
            "origin_format": "NCHW",
            "shape": [8, 1, 16, 4, 16],
            "origin_shape": [8, 16, 16, 4],
            "type": "float16"
          },
          {
            "format": "NC1HWC0",
            "origin_format": "NCHW",
            "shape": [8, 1, 16, 4, 16],
            "origin_shape": [8, 16, 16, 4],
            "type": "float16"
          }
        ],
        "output_desc": [
          {
            "format": "NC1HWC0",
            "origin_format": "NCHW",
            "shape": [8, 1, 16, 4, 16],
            "origin_shape": [8, 16, 16, 4],
            "type": "float16"
          }
        ]
      }
    ]
    ```

- **Input specified as a constant**

    In this scenario, inputs that can be set as constants are supported. Two parameters, **is\_const** and **const\_value**, are added to indicate whether the input is a constant and the constant value, respectively. **const\_value** currently supports only one-dimensional list configuration, and the number of configured elements is determined by the value of shape. For example, in the following sample, shape is 2, so the number of elements in the list of **const\_value** is 2. The value type in **const\_value** is determined by type. If type is float16, the values in **const\_value** are automatically converted to float16 format during single operator compilation.

    The offline model converted from the single operator in this example is resizeBilinearV2.om.

    ```json
    [
      {
        "op": "ResizeBilinearV2",
        "name": "resizeBilinearV2",
        "input_desc": [
          {
            "format": "NHWC",
            "name": "x",
            "shape": [
              4,
              16,
              16,
              16
            ],
            "type": "float16"
          },
          {
            "format": "NHWC",
            "is_const": true,
            "const_value": [49, 49],
            "name": "size",
            "shape": [
              2
            ],
            "type": "int32"
          }
        ],
        "output_desc": [
          {
            "format": "NHWC",
            "name": "y",
            "shape": [
              4,
              48,
              48,
              16
            ],
            "type": "float"
          }
        ],
        "attr": [
          {
            "name": "align_corners",
            "type": "bool",
            "value": false
          },
          {
            "name": "half_pixel_centers",
            "type": "bool",
            "value": false
          }
        ]
      }
    ]
    ```

- **Optional input:**

    When an optional input exists but has no input data, the format of the optional input must be set to RESERVED and its type must be set to UNDEFINED. If the optional input has input data, set its format and type based on the input data.

    The offline model converted from the single operator in this example is matMulV2.om.

    ```json
    [
      {
        "op": "MatMulV2",
        "name": "matMulV2",
        "input_desc": [
          {
            "format": "ND",
            "shape": [16, 16],
            "type": "float"
          },
          {
            "format": "ND",
            "shape": [16, 16],
            "type": "float"
          },
          {
            "format": "RESERVED",
            "shape": [],
            "type": "UNDEFINED"
          },
          {
            "format": "RESERVED",
            "shape": [],
            "type": "UNDEFINED"
          }
        ],
        "attr": [
        {
            "name": "transpose_x1",
            "type": "bool",
            "value": false
        },
        {
            "name": "transpose_x2",
            "type": "bool",
            "value": false
        }
        ],
        "output_desc": [
          {
            "format": "ND",
            "shape": [16, 16],
            "type": "float"
          }
     ]
      }
    ]

    ```

- **Uncertain number of inputs (dynamic input scenario):**

    In this scenario, the number of inputs of the single operator is uncertain. The AddN single operator is used as an example here. The offline model converted from the single operator in this example is addN.om.

  - The constructed single operator JSON file uses the dynamic input dynamic\_input parameter instead of the Tensor name parameter.

    In this scenario, the value of dynamic\_input of the operator must be the same as the value of the input name defined for this operator in the operator information library. The number of inputs to be set is determined by the value of **N** in the attribute parameters of the AddN single operator description file. You can modify the number of inputs, but it must match the value of N in the attributes. (This description applies only to the AddN operator. For other dynamic input operators, follow the constraints of the specific operator.)

      ```json
      [
          {
              "op": "AddN",
              "name": "addN",
              "input_desc": [
                  {
                      "dynamic_input": "x",
                      "format": "NCHW",
                      "shape": [1,3,166,166],
                      "type": "float32"
                  },
                  {
                      "dynamic_input": "x",
                      "format": "NCHW",
                      "shape": [1,3,166,166],
                      "type": "int32"
                  },
                  {
                      "dynamic_input": "x",
                      "format": "NCHW",
                      "shape": [1,3,166,166],
                      "type": "float32"
                  }
              ],
              "output_desc": [
                  {
                      "format": "NCHW",
                      "shape": [1,3,166,166],
                      "type": "float32"
                  }
              ],
              "attr": [
                  {
                      "name": "N",
                      "type": "int",
                      "value": 3
                  }
              ]
          }
      ]
      ```

  - The constructed single operator JSON file uses the Tensor name parameter instead of the dynamic input dynamic\_input parameter.

    In this scenario, the value of the operator's name must be the same as the input name of the operator in the operator prototype definition. x0, x1, x2, and so on are automatically generated based on the number of inputs. The number of Tensor names to be set is determined by the value of **N** in the attribute parameters of the AddN single operator description file. You can modify the number of Tensor names, but it must match the value of N in the attributes. For example, if N is set to 3, set the name values to x0, x1, and x2. (This note applies only to the AddN operator. For constraints on other dynamic input operators, refer to the specific operator.)

      ```json
        [
            {
                "op": "AddN",
                "name": "addN",
                "input_desc": [
                    {
                        "name":"x0",
                        "format": "NCHW",
                        "shape": [1,3,166,166],
                        "type": "float32"
                    },
                    {
                        "name":"x1",
                        "format": "NCHW",
                        "shape": [1,3,166,166],
                        "type": "int32"
                    },
                    {
                        "name":"x2",
                        "format": "NCHW",
                        "shape": [1,3,166,166],
                        "type": "float32",

                    }
                ],
                "output_desc": [
                    {
                        "format": "NCHW",
                        "shape": [1,3,166,166],
                        "type": "float32"
                    }
                ],
                "attr": [
                    {
                        "name": "N",
                        "type": "int",
                        "value": 3
                    }
                ]
            }
        ]
        ```

## Configuring Multiple Operator Description Files

The description file supports defining multiple operator JSON file configurations. Each configuration includes the operator type, operator input and output information, and attribute information depending on the operator.

If the JSON file configures multiple operators, multiple om offline model files corresponding to the operators are generated after model conversion. The following configuration file is only an example. Modify it based on the actual situation.

```json
[
  {
    "op": "MatMul",
    "name": "matMul01",
    "input_desc": [
      {
        "format": "ND",
        "shape": [
          16,
          16
        ],
        "type": "float16"
      },
    ...
    ],
    "output_desc": [
      {
        "format": "ND",
        "shape": [
          16,
          16
        ],
        "type": "float16"
      }
    ],
    "attr": [
      {
        "name": "alpha",
        "type": "float",
        "value": 1.0
      },
    ...
    ]
  },
  {
    "op": "MatMul",
    "name": "matMul02",
    "input_desc": [
      {
        "format": "ND",
        "shape": [
          256,
          256
        ],
        "type": "float16"
      },
    ...
    ],
    "output_desc": [
      {
        "format": "ND",
        "shape": [
          256,
          256
        ],
        "type": "float16"
      }
    ],
    "attr": [
      {
        "name": "alpha",
        "type": "float",
        "value": 1.0
      },
    ...
    ]
  }
]
```

## Configuring a Single Operator Description File for Dynamic Shape

In dynamic shape scenarios, the content of a single operator description file varies depending on the scenario. This section provides configuration examples for different scenarios.

- When the shape is not specified during model compilation, the specific output shape can be derived based on the input static shape during model execution:

    ```json
    [
      {
        "op": "Add",
        "name": "add",
        "input_desc": [
          {
            "format": "ND",
            "shape": [-1,16],
            "shape_range": [[0, 32]],
            "type": "int64"
          },
          {
            "format": "ND",
            "shape": [-1,16],
            "shape_range": [[0, 32]],
            "type": "int64"
          }
        ],
        "output_desc": [
          {
            "format": "ND",
            "shape": [-1,16],
            "shape_range": [[0,32]],
            "type": "int64"
          }
        ]
      }
    ]
    ```

- When the shape is not specified during model compilation, the specific output shape can be derived based on the input static shape and constants during model execution:

    ```json
    [
      {
        "op": "TopK",
        "name": "topK",
        "input_desc": [
          {
            "format": "ND",
            "shape": [-1],
           "shape_range": [[1,-1]],
            "type": "int32"
          },
          {
            "format": "ND",
            "shape": [],      # A constant is passed in during inference
            "type": "int32"
          }
        ],
        "output_desc": [
          {
            "format": "ND",
            "shape": [-1],
            "shape_range": [[1,-1]],
            "type": "int32"
          },
       {
            "format": "ND",
            "shape": [-1],
            "shape_range": [[1,-1]],
            "type": "int32"
          }],
          "attr": [
          {
            "name": "sorted",
            "type": "bool",
            "value": true
          }
        ]
      }
    ]
    ```

- When the shape is not specified during model compilation, the exact output shape of the operator cannot be obtained based on the input static shape during model execution, but the range of the output shape can be obtained.

    In this scenario, in the output parameter output\_desc, set the value of the dimension whose shape is a dynamic dimension in the operator output TensorDesc to "-1", and provide the value range for the "-1" dimension in shape\_range:

    ```json
    [
      {
        "op": "Where",
        "name": "where",
        "input_desc": [
          {
            "format": "ND",
            "shape": [-1],
            "shape_range": [[1,-1]],
            "type": "int32"
          }
        ],
        "output_desc": [
          {
            "format": "ND",
            "shape": [-1, 1],
            "shape_range": [[1,-1]],
            "type": "int64"
          }
        ]
      }
    ]
    ```
