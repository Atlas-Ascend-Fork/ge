# TransferConfig

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T04:03:30.836Z pushedAt=2026-11-17T11:33:39.731Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 inference products: Supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 training products: Not supported
<!-- end id3 -->

## Function

Constructs TransferConfig.

## Prototype

```python
__init__(dst_cluster_id: int, dst_addrs: List[int], src_layer_range: Optional[range] = None, src_batch_index: int = 0)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| dst_cluster_id | int | cluster_id of the instance where the destination cache resides. |
| dst_addrs | List[int] | Memory address of each tensor in the destination cache. If the destination cache is in a non-PA scenario and the batch_index to be transferred to is not 0, dst_addrs must be offset to the actual address. |
| src_layer_range | Optional[range] | Range of local layers to be transferred. step supports only 1. The default value is None, indicating that all layers are transferred. |
| src_batch_index | int | Batch index of the local cache. It can be set when the source cache is in a non-PA scenario. |

## Examples

```python
from llm_datadist import TransferConfig
TransferConfig(1, dst_addrs, range(0, 3), 1)
```

## Return Value

Normally, an instance of TransferConfig is returned.

TypeError or ValueError may be thrown for invalid parameters.

src\_layer\_range If the value is invalid, [LLMException](LLMException.md) is thrown.

## Constraints

The number of addresses in the destination address list must be twice the number of layers to be transferred.
