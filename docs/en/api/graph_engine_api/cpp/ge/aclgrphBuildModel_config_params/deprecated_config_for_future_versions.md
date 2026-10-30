# Deprecated Configurations for Future Versions

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:47:38.712Z pushedAt=2026-10-28T07:40:22.209Z nl=2 -->

## INPUT\_SHAPE\_RANGE

**This parameter is deprecated. Do not use it.**
To specify the shape range of model input data, use the **INPUT\_SHAPE** parameter.

Specifies the shape range of model input data. This feature cannot be used together with **DYNAMIC\_BATCH\_SIZE**, **DYNAMIC\_IMAGE\_SIZE**, or **DYNAMIC\_DIMS**.

- Supports configuration by name: "input\_name1:\[n1,c1,h1,w1\];input\_name2:\[n2,c2,h2,w2\]", for example: "input\_name1:\[8\~20,3,5,-1\];input\_name2:\[5,3\~9,10,-1\]". The specified node must be enclosed in double quotation marks, and multiple nodes must be separated by semicolons. **input\_name** must be the node name in the network model before conversion, and the shape range value must be placed in \[\]. If you know the name of the data node, configuring **INPUT\_SHAPE\_RANGE** by name is recommended.
- Supports configuration by index: "\[n1,c1,h1,w1\],\[n2,c2,h2,w2\]", for example: "\[8\~20,3,5,-1\],\[5,3\~9,10,-1\]". The node name can be omitted; by default, the first pair of brackets corresponds to the first input node, and multiple nodes are separated by commas. When configuring **INPUT\_SHAPE\_RANGE** by index, the data node must have the **index** attribute set to indicate which input it is, with the index starting from 0.
- For dynamic dimensions, a shape range is represented by a tilde “\~”, a fixed dimension is represented by a fixed number, and an unbounded range is represented by -1.
- For scalar inputs, the shape range must also be filled in, expressed as: \[\].
- For multi-input scenarios, for example, when there are three inputs and only the second and third inputs have shape ranges while the first input is a fixed input, the shape of the fixed input must still be filled in.

## SHAPE\_GENERALIZED\_BUILD\_MODE

The Shape compilation method during graph compilation. **This parameter is deprecated in later versions. Do not use it for newly developed features.**

- **shape\_generalized**: Fuzzy compilation means that, for operators that support dynamic Shape, the system generalizes the variable dimensions internally before compilation.

    This parameter applies to the scenario where you want to compile once and perform inference multiple times, in which case you can use the fuzzy compilation feature.

- **shape\_precise**: Exact compilation means that the system compiles directly according to the dimension information specified by the user without any escaping internally during compilation.
