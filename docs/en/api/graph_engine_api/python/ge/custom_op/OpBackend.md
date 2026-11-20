# OpBackend

<!-- md-trans-meta sourceCommit=ec59967ce75a72680ec94e5392304e5308677e5f translatedAt=2026-10-15T02:25:03.569Z pushedAt=2026-10-16T10:53:07.279Z -->

Enumeration of execution backends for Python custom operators. [register_kernel](register_kernel.md) selects the execution backend of **execute** through this enumeration.

| Enumeration Value | Description |
| --- | --- |
| DEVICE | Device backend. |
| HOST | Host CPU backend. |
