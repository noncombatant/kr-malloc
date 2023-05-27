// Copyright 2022 by [Chris Palmer](https://noncombatant.org)
// SPDX-License-Identifier: Apache-2.0

#ifndef PORCELAIN_UTC_NANOSECONDS_H
#define PORCELAIN_UTC_NANOSECONDS_H

#include <stdint.h>

// Returns the current time since the epoch, UTC, in nanoseconds, or 0 on error.

int64_t GetUTCNanoseconds(void);

#endif
