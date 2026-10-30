# Header Files and Library Files

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:10:16.479Z pushedAt=2026-10-29T06:33:18.771Z nl=2 -->

## API Categories

API names use acl as the prefix, following the naming style: acl+_API category abbreviation_+\*, where \* represents the operation verb and object, both using initial capitalization. For ease of description, all APIs in this document are collectively referred to as acl APIs.

**Table 1**  Key API categories

| API name prefix | Description |
| --- | --- |
| aclmdl | APIs for model inference |
| aclop | APIs for single-operator model execution |
| aclblas | BLAS APIs |

## Header Files and Library Files Required for Calling APIs

After installing the firmware, driver, and CANN software package, you can reference the header files and library files of the ACL APIs when compiling and running applications.

You need to include the required files based on the ACL APIs actually used. The purpose of each header file is listed in the following table.

The header files of the ACL APIs are located in the “$\{INSTALL\_DIR\}/include/” directory, and the library files are located in the “$\{INSTALL\_DIR\}/lib64/” directory. $\{INSTALL\_DIR\} must be replaced with the file storage path after the CANN software is installed. For example, when installed as the root user, the default file storage path is /usr/local/Ascend/cann.

**Table 2** Header file list

| Header file defining the API | Purpose | Corresponding library file |
| --- | --- | --- |
| acl/acl_mdl.h | Defines the model management APIs. | libacl_mdl.so<br>Note: For compatibility with earlier versions, libascendcl.so is supported in earlier versions, but this method will be deprecated in later versions. You are advised to use libacl_mdl.so to avoid compatibility issues in later versions. |
| acl/acl_op.h<br>acl/acl_op_compiler.h | Defines the single-operator call APIs (including only the single-operator model execution APIs). | libacl_op_executor.so<br>libacl_op_compiler.so<br>Note: For compatibility with earlier versions, libascendcl.so is supported in earlier versions, but this method will be deprecated in later versions. You are advised to use libacl_op_executor.so and libacl_op_compiler.so to avoid compatibility issues in later versions. |
| acl/ops/acl_cblas.h | Defines the CBLAS APIs. | libacl_cblas.so |

**Note:**
When compiling ACL API programs, link the corresponding library files based on the included header files. If redundant .so files (for example, libascendcl.a) are referenced, version functions may become abnormal or compatibility issues may occur during later version upgrades.
