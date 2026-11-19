# CustomPassFn

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:06:18.667Z pushedAt=2026-10-28T07:42:19.766Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <register/register\_custom\_pass.h\>
- Library file: libregister.so

## Description

Registers the execution function of a custom Pass.

For details about how to use this API, see Programming Guide > Custom Pass Development > Using a Custom Pass to Modify a Graph.

## Prototype

```c++
PassRegistrationData &CustomPassFn(const CustomPassFunc &custom_pass_fn)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| custom_pass_fn | Input | Execution function of the custom Pass. |

## Return Value

Returns a reference to the object itself.

## Constraints

None

## Callback Function CustomPassFunc

You define and implement the CustomPassFunc class function, that is, a custom graph modification function.

```c++
Status CustomPassFunc(GraphPtr &graph, CustomPassContext &custom_context)
```

**Table 1**  Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Graph to be modified. |
| custom_context | Input | Debugging object through which debugging information is registered with the framework. |
| - | Output | - SUCCESS: Success.<br>  - Other values: Failure. |
