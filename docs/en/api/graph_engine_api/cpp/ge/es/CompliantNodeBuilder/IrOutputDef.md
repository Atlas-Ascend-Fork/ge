# IrOutputDef

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:40:05.178Z pushedAt=2026-10-28T07:40:22.361Z nl=2 -->

Description: This struct is used for the legacy APIs and contains a **std::string** field. For new code, use the ABI-safe [IrOutputDefV2](./IrOutputDefV2/IrOutputDefV2.md) instead.

The IR output definition struct is defined as follows:

```c++
struct IrOutputDef {
  std::string name;
  IrOutputType ir_output_type;
  std::string symbol_id;
}
```

Parameters

| Member | Type | Description |
| --- | --- | --- |
| name | std::string | Output name. |
| ir_output_type | IrOutputType | Output type. |
| symbol_id | std::string | Reserved field. |
