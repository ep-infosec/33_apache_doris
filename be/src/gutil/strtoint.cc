// Copyright 2008 Google Inc. All Rights Reserved.
//
// Architecture-neutral plug compatible replacements for strtol() friends.
// See strtoint.h for details on how to use this component.
//

#include "gutil/strtoint.h"

#include <errno.h>

#include <limits>

// Replacement strto[u]l functions that have identical overflow and underflow
// characteristics for both ILP-32 and LP-64 platforms, including errno
// preservation for error-free calls.
int32 strto32_adapter(const char* nptr, char** endptr, int base) {
    const int saved_errno = errno;
    errno = 0;
    const long result = strtol(nptr, endptr, base);
    if (errno == ERANGE && result == LONG_MIN) {
        return std::numeric_limits<int>::min();
    } else if (errno == ERANGE && result == LONG_MAX) {
        return std::numeric_limits<int>::max();
    } else if (errno == 0 && result < std::numeric_limits<int>::min()) {
        errno = ERANGE;
        return std::numeric_limits<int>::min();
    } else if (errno == 0 && result > std::numeric_limits<int>::max()) {
        errno = ERANGE;
        return std::numeric_limits<int>::max();
    }
    if (errno == 0) errno = saved_errno;
    return static_cast<int32>(result);
}

uint32 strtou32_adapter(const char* nptr, char** endptr, int base) {
    const int saved_errno = errno;
    errno = 0;
    const unsigned long result = strtoul(nptr, endptr, base);
    if (errno == ERANGE && result == ULONG_MAX) {
        return std::numeric_limits<unsigned int>::max();
    } else if (errno == 0 && result > std::numeric_limits<unsigned int>::max()) {
        errno = ERANGE;
        return std::numeric_limits<unsigned int>::max();
    }
    if (errno == 0) errno = saved_errno;
    return static_cast<uint32>(result);
}