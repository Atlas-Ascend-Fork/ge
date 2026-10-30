# MemoryType

<!-- md-trans-meta sourceCommit=0e5c73f3b4489260ae9f6461e1178f02c751dbca translatedAt=2026-10-16T11:22:32.225Z pushedAt=2026-10-28T07:40:22.163Z nl=2 -->

Memory type enum. The header file is located in **include/ge/ge\_api\_types.h** under the file storage path after CANN software installation.

```c++
enum class MemoryType : std::int64_t {
  /*
   * call aclrtMalloc with aclrtMemMallocPolicy::ACL_MEM_MALLOC_HUGE_FIRST,
   * ACL_MEM_MALLOC_HUGE_ONLY, ACL_MEM_MALLOC_NORMAL_ONLY
   */
  MEMORY_TYPE_DEFAULT,
  /*
   * call aclrtMalloc with aclrtMemMallocPolicy::ACL_MEM_MALLOC_HUGE_FIRST_P2P,
   * ACL_MEM_MALLOC_HUGE_ONLY_P2P, ACL_MEM_MALLOC_NORMAL_ONLY_P2P
   */
  MEMORY_TYPE_P2P
};
```

The enum items are described as follows:

- **MEMORY\_TYPE\_DEFAULT**: Default Device memory type. It can be obtained by calling the **aclrtMalloc** API. When applying for memory, pass one of the three enumeration values in **aclrtMemMallocPolicy**: **ACL\_MEM\_MALLOC\_HUGE\_FIRST**, **ACL\_MEM\_MALLOC\_HUGE\_ONLY**, **ACL\_MEM\_MALLOC\_NORMAL\_ONLY**
- **MEMORY\_TYPE\_P2P**: Memory type used only in device-to-device memory copy scenarios. It can be obtained by calling the **aclrtMalloc** API. When applying for memory, pass one of the three enum values in **aclrtMemMallocPolicy**: **ACL\_MEM\_MALLOC\_HUGE\_FIRST\_P2P**, **ACL\_MEM\_MALLOC\_HUGE\_ONLY\_P2P**, **ACL\_MEM\_MALLOC\_NORMAL\_ONLY\_P2P**

For details about the **aclrtMalloc** and **aclrtMemMallocPolicy** APIs, see *Runtime API*.
