# DUMP\_GE\_GRAPH

<!-- md-trans-meta sourceCommit=2461b5215e5b8745a550d67c26b5f9840c83434b translatedAt=2026-10-20T12:44:39.364Z pushedAt=2026-10-28T11:27:34.998Z nl=2 -->

## Feature Description

Prints the graph description information of each stage in the entire process to a file. This environment variable controls the amount of content in the dumped graph.

Supported values include:

- 1: A full dump that includes edge relationships and data information.
- 2: A basic dump that does not contain data such as weights.
- 3: A simplified dump that shows only node relationships.

After this environment variable is set, the following files are generated in the current path:

- ge\_onnx\*.pbtxt: An ONNX-based model description structure that can be opened with visualization software such as Netron.
- ge\_proto\*.txt: A text file stored in protobuf format. This file can be converted to a JSON format file to help users locate issues. This file generally appears in pairs with ge\_onnx\*.pbtxt, but ge\_proto\*.txt contains more string-type attribute information than the ge\_onnx\*.pbtxt file. Therefore, ge\_proto\*.txt is displayed more completely, and users can choose either file to open.

    Because the ge\_proto\*.txt file structure has been optimized for file size compared with ge\_onnx\*.pbtxt, setting the DUMP\_GE\_GRAPH environment variable to 2 or 3 has the same effect on the ge\_proto\*.txt file, both displaying a basic dump that does not contain data such as weights.

- ge\_readable\*.txt: (Optional) A highly readable text file similar to the Dynamo fx graph style. This type of file is generated only when the DUMP\_GRAPH\_FORMAT environment variable is set and its value includes "readable".

## Configuration Example

```bash
export DUMP_GE_GRAPH=1
```

## Constraints

None

## Applicable Products

Supported on all chips
