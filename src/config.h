#pragma once

// Platform detection macros for XER
// Sets one of XER_PLATFORM_WINDOWS, XER_PLATFORM_LINUX, XER_PLATFORM_MACOS, XER_PLATFORM_UBUNTU, XER_PLATFORM_OPENBSD, XER_PLATFORM_POSIX

#if defined(_WIN32) || defined(_WIN64)
    #define XER_PLATFORM_WINDOWS 1
#else
    #define XER_PLATFORM_WINDOWS 0
#endif

#if defined(__APPLE__) && defined(__MACH__)
    #define XER_PLATFORM_MACOS 1
#else
    #define XER_PLATFORM_MACOS 0
#endif

#if defined(__linux__)
    #define XER_PLATFORM_LINUX 1
#else
    #define XER_PLATFORM_LINUX 0
#endif

#if defined(__linux__) && defined(__ubuntu__)
    #define XER_PLATFORM_UBUNTU 1
#else
    #define XER_PLATFORM_UBUNTU 0
#endif

#if defined(__OpenBSD__)
    #define XER_PLATFORM_OPENBSD 1
#else
    #define XER_PLATFORM_OPENBSD 0
#endif

#if defined(_WIN32) || defined(_WIN64)
    #define XER_PLATFORM_POSIX 0
#else
    #define XER_PLATFORM_POSIX 1
#endif
