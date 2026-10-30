# --raw\_ge\_options

<!-- md-trans-meta sourceCommit=2461b5215e5b8745a550d67c26b5f9840c83434b translatedAt=2026-10-22T03:40:31.808Z pushedAt=2026-10-27T03:35:29.182Z -->

## Supported Products

Supported on all chips

## Function Description

Specifies the path of the JSON file that contains GE compile options. The ATC tool parses the "compile options" field in the JSON file and converts it into the specific parameters required for GE compilation.

Applicable scenario: When a user has performed online or offline inference using the [options parameter](../../../api/graph_engine_api/cpp/ge/options_params/overall_constraints.md) and wants to apply these configurations to the offline inference process of the ATC tool, the user can specify this JSON file to reuse the configurations, ensuring consistent inference behavior on both ends.

## Related Parameters

If the JSON file specified by --raw\_ge\_options contains options that are not supported by the ATC tool, it must be used together with the [--raw\_ge\_options\_ignore\_unsupported](--raw_ge_options_ignore_unsupported.md) parameter. When this parameter is set to true, the ATC tool ignores the unsupported options, prints a warning, and continues to process the remaining options. When it is set to false, the ATC tool reports an error and terminates upon encountering an unsupported option.

## Parameter Value

**Parameter Value:**
Path and file name of the JSON file. The following is an example of the JSON file:

```json
{
  "compile options": {
    "global": {
      "ge.jit_compile": "1"
    },
    "session": {
      "ge.jit_compile": "1"
    },
    "graph": {
      "ge.jit_compile": "2"
    }
  },
  "execute options": {
    "graph": {
      "ge.exec.staticMemoryPolicy": "2"
    }
  }
}
```

The ATC tool parses only the "compile options" in the file and always ignores the "execute options". It flattens and merges the global, session, and graph objects in the file, with the merge priority as follows: graph \> session \> global.

For the same parameter, the effective priority is as follows: parameters explicitly set on the ATC command line \> --raw\_ge\_options parameters in the JSON file \> ATC parameter default values.

For example, for the ge.jit\_compile parameter in the preceding example (for the parameter description, see [options parameter description](../../../api/graph_engine_api/cpp/ge/options_params/overall_constraints.md)), the final value in the JSON file is 2. If [--jit\_compile](--jit_compile.md)=0 is also specified on the ATC command line, the value that finally takes effect is 0.

**Parameter Value Constraints:**
Each options key in the JSON file must be non-empty.

## Recommended Configuration and Benefits

None.

## Example

Upload the JSON file to the path on the server where the ATC tool is located, and run the following command:

```bash
atc --raw_ge_options=$HOME/module/option.json --model=model.onnx ...
```

## Dependencies

None.
