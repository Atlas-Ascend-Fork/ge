# Overview

<!-- md-trans-meta sourceCommit=57e742592fb0b093fa4f5d82c393c97b374a8bfd translatedAt=2026-10-15T04:00:40.793Z pushedAt=2026-10-16T10:53:07.643Z -->

A pattern-matching-based fusion pass that inherits from **FusionBasePass**. The execution engine invokes the **patterns\(\)**, **meet\_requirements\(\)**, and **replacement\(\)** methods instead of the **run\(\)** method. Both the **meet\_requirements\(\)** and **replacement\(\)** methods can declare `context: PassContext` at the end of their parameters.

Constraints:

- **Do not override the run\(\) method**: if the **run\(\)** method is defined in a subclass, a **TypeError** is raised at class definition time.
- You must implement the **patterns\(\)** method or at least one **@pattern** method, and implement the **replacement\(\)** method; the **meet\_requirements\(\)** method is optional (it returns **True** by default).
- The **@pattern** method cannot be used together with the **patterns\(\)** method.
- The **patterns\(self, inputs\)** syntax is not supported. For expression patterns, use the **@pattern** method.
