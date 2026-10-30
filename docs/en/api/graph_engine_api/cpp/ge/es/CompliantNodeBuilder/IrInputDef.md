# IrInputDef

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:39:17.439Z pushedAt=2026-10-28T07:40:22.358Z nl=2 -->

This struct is used by legacy APIs and contains a **std::string** field. For new code, use the ABI-safe [IrInputDefV2](./IrInputDefV2/IrInputDefV2.md).

IR input definition struct, defined as follows:

```c++
struct IrInputDef {
  std::string name;
  IrInputType ir_input_type;
  std::string symbol_id;
}
```

The parameters are described as follows:

| Member | Type | Description |
| --- | --- | --- |
| name | std::string | Input name. |
| ir_input_type | IrInputType | Input type. |
| symbol_id | std::string | Reserved field. |
