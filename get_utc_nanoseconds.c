// Copyright 2022 Chris Palmer, https://noncombatant.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#include "get_utc_nanoseconds.h"

#if defined(_POSIX_VERSION) || defined(__MACH__)
#include "get_utc_nanoseconds_posix.c"
#else
#error Not implemented yet
#endif
