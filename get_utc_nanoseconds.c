// Copyright 2022 by [Chris Palmer](https://noncombatant.org)
// SPDX-License-Identifier: Apache-2.0

#include "get_utc_nanoseconds.h"

#if defined(_POSIX_VERSION) || defined(__MACH__)
#include "get_utc_nanoseconds_posix.c"
#else
#error Not implemented yet
#endif
