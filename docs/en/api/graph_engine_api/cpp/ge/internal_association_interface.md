# Internal Associated APIs

<!-- md-trans-meta sourceCommit=5b8f845da85af623dc094addaa97e74b5eb3f467 translatedAt=2026-10-16T11:21:49.845Z pushedAt=2026-10-28T07:40:22.165Z nl=2 -->

The following APIs in **ge\_api\_error\_codes.h** are internal associated APIs and do not need to be called directly by developers:

- **ErrorNoRegisterar** class: An error code registration class used to register specific error codes and their descriptions.

    It contains the following member functions:

  - Two constructors:

    ErrorNoRegisterar\(uint32\_t err, const std::string &desc\) noexcept

    ErrorNoRegisterar\(const uint32\_t err, const char \*const desc\) noexcept

    Where **err** is the error code and **desc** is the error code description.

  - One default destructor:

    \~ErrorNoRegisterar\(\) = default

- **StatusFactory** class: A singleton status factory class that manages registered error codes.

    It contains the following member functions:

  - AscendString GetErrDescV2\(const uint32\_t err\)

    Obtains the error code description based on the error value (ABI-compatible interface, recommended for priority use).

  - static StatusFactory \*Instance\(\)

    Returns the **StatusFactory** class object as a singleton, which is globally unique.

  - void RegisterErrorNo\(const uint32\_t err, const std::string &desc\)

    void RegisterErrorNo\(const uint32\_t err, const char \*const desc\)

    Used for error code registration.

  - std::string GetErrDesc\(const uint32\_t err\)

    Obtains the error code description based on the error code value.
