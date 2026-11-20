# Introduction

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-06T11:22:59.494Z pushedAt=2026-11-13T11:32:26.849Z nl=2 -->

This section describes two methods of modifying a graph through Pass. The differences between the two Pass implementations are as follows:

**Table 1** Comparison of the two Pass implementation methods

| Graph Modification Method | Pass Implemented Based on Pattern Mapping (Recommended) | Pass Implemented Based on the Graph Modification API |
| --- | --- | --- |
| Core idea | Define a matching template through Pattern. The framework automatically completes matching and searching, and developers only need to define the replacement structure. | Directly write graph editing functions, manually traverse the graph structure, search for nodes, create new nodes, establish edge connections, and delete old nodes. |
| Key features | Only three steps are required: match → decide → replace<br><br>  - Patterns: define the template topology to be matched<br>  - MeetRequirements: condition filtering (optional)<br>  - Replacement: define the replacement structure | - Node search, edge processing, and node addition/deletion must be implemented manually<br>  - Large amount of code, requiring handling of all graph modification details |
| Applicable scenarios | This development method can be used when no constraints are triggered | - Simple, one-time modification scenarios<br>  - Directly call image editing APIs to adjust the graph structure in real time<br>  - Scenarios requiring full control over the graph structure |
| Constraints/Prohibited items | - Control edges: the framework does not support control dependency matching, only data edge matching<br>  - Subgraph: nested subgraph structures are not supported. Pattern must be a flat DAG (Directed Acyclic Graph)<br>  - Dynamic input/output nodes: an uncertain number of inputs/outputs causes matching ambiguity and prevents precise positioning | - |
