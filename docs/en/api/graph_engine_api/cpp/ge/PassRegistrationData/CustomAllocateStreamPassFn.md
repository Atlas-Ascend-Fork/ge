# CustomAllocateStreamPassFn

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:05:56.035Z pushedAt=2026-10-28T07:42:19.765Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <register/register\_custom\_pass.h\>
- Library file: libregister.so

## Description

Registers the custom logical stream allocation Pass execution function.

For details about how to use the API, see Programming Guide \> Custom Pass Development \> Using a Custom Logical Stream Allocation Pass to Customize Concurrency.

## Prototype

```c++
PassRegistrationData &CustomAllocateStreamPassFn(const CustomAllocateStreamPassFunc &allocate_stream_pass_fn)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| allocate_stream_pass_fn | Input | Custom logical stream allocation function. For details, see [Callback Function CustomAllocateStreamPassFunc](#callback-function-customallocatestreampassfunc). |

## Return Value

Returns a reference to the current object.

## Constraints

None

## Callback Function CustomAllocateStreamPassFunc

You define and implement the CustomAllocateStreamPassFunc class function, that is, a custom logical stream allocation function.

```c++
Status CustomAllocateStreamPassFunc(const ConstGraphPtr &graph, StreamPassContext &stream_context)
```

**Table 1**  Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Graph for which logical streams are to be allocated. |
| stream_context | Input | Logical stream allocation context. You can use this context to apply for a new stream id and set the stream id of a node. |
| - | Output | - SUCCESS: Success.<br>  - Other values: Failure. |
