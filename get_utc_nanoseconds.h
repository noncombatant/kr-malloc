// Copyright 2022 Chris Palmer, https://noncombatant.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#ifndef PORCELAIN_UTC_NANOSECONDS_H
#define PORCELAIN_UTC_NANOSECONDS_H

#include <stdint.h>

// Returns the current time since the epoch, UTC, in nanoseconds, or 0 on error.

int64_t GetUTCNanoseconds(void);

#endif
