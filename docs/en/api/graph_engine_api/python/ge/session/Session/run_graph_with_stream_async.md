# run\_graph\_with\_stream\_async

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-15T04:18:03.704Z pushedAt=2026-10-16T10:53:07.725Z -->

## Description

Asynchronously runs a graph.

## Prototype

```python
run_graph_with_stream_async(graph_id: int, stream: int, inputs: List[Tensor]) -> List[Tensor]
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | Graph ID. |
| stream | Input | Stream on which the graph runs. |
| inputs | Input | List of input tensors, which are memory spaces on the device. |

## Return Value

Returns a list of output tensors, which are memory spaces on the device.

## Constraints

- If **graph_id** is not an integer, a TypeError is thrown.
- If **stream** is not an integer, a TypeError is thrown.
- If any element in **inputs** is not of the Tensor type, a TypeError is thrown.
- If the execution fails, a RuntimeError is thrown.
- Before calling this API, create a **Stream** through the **acl.rt.create\_stream\(\)** API provided by acl, and only the scenario where the **Stream** is the default **Context** is supported. Before obtaining the output execution result, ensure that the tasks on the **Stream** have been executed through the **acl.rt.synchronize\_stream** API. For details about the API, see "[Stream Management](https://gitcode.com/cann/runtime/blob/9.2.0/docs/zh/api_ref/README.md)".
