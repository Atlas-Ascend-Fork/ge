# --help or --h

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-16T13:59:06.082Z pushedAt=2026-11-17T08:49:53.629Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

Displays help information.

## Related Parameters

None.

## Parameter Values

None.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --help
```

Part of the returned information is as follows:

```console
ATC start working now, please wait for a moment.
usage: atc <args>
generate offline model example:
atc --model=./alexnet.prototxt --weight=./alexnet.caffemodel --framework=0 --output=./domi --soc_version=<soc_version>
generate offline model for single op example:
atc --singleop=./op_list.json --output=./op_model --soc_version=<soc_version>

===== Basic Functionality =====
[General]
  --h/help            Show this help message
... ...
```

## Dependency Constraints

None.
