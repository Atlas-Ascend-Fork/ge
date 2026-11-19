# Failure to Load a Single Operator Due to Operator Library Package Version Mismatch

<!-- md-trans-meta sourceCommit=093cb17925c99586593960695038a0ea0d003851 translatedAt=2026-11-16T15:01:53.951Z pushedAt=2026-11-17T12:37:27.017Z nl=2 -->

## Symptoms

Loading a single operator fails with an error, and the log displays information similar to the following:

```console
E19999: Internal error!
E19999 The opp version of the model does not match the current opp run package, Model is [6.4.T11.0.B300], opp run package is [7.0.T3.0.B107], try to convert the om again!
```

## Possible Causes

In a dynamic shape operator scenario or an Ascend virtualization instance scenario, the installed version of the operator library package (package name: Ascend-cann-toolkit_\_\{version\}_\_linux-_\{arch\}_.run) in the environment where single operator model data is loaded is **inconsistent with the installed version of the operator library package** in the environment where the om model file is compiled, causing an error when operators are loaded.

## Solution

In the dynamic shape operator scenario or the Ascend virtualization instance scenario, the installed version of the operator library package in the environment where single operator model data is loaded must be **consistent with the installed version of the operator library package** in the environment where the om model file is compiled. After this error occurs, check the installed versions and choose to replace the opp package version in the operator loading environment or replace the opp package version in the environment where the operator om file is compiled. If you choose the latter, the model must be converted again.
