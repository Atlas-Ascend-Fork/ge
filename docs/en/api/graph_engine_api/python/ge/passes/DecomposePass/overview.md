# Overview

<!-- md-trans-meta sourceCommit=57e742592fb0b093fa4f5d82c393c97b374a8bfd translatedAt=2026-10-15T03:52:07.807Z pushedAt=2026-10-16T10:53:07.607Z -->

Defines the operator decomposition pass, which inherits from **FusionBasePass**. The execution engine calls the **meet\_requirements\(\)** and **replacement\(\)** methods, rather than the **run()** method, on matched nodes. Both the **meet\_requirements\(\)** and **replacement\(\)** methods can declare `context: PassContext` at the end of their parameters.

**Constraints**:

- **Do not override the **run()** method**: If the **run()** method is defined in a subclass, a TypeError is raised at class definition time.
- The **replacement()** method must be implemented, while **meet_requirements()** is optional (returns **True** by default).
- A subclass can define the class attribute **op_types** (**Optional[List[str]]**) to specify the list of operator types to be decomposed. This attribute is automatically set when the **register_decompose_pass** decorator is used.
