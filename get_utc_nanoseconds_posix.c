// Copyright 2022 by [Chris Palmer](https://noncombatant.org)
// SPDX-License-Identifier: Apache-2.0

#include <time.h>

int64_t GetUTCNanoseconds(void) {
  struct timespec time;
  if (clock_gettime(CLOCK_REALTIME, &time)) {
    return 0;
  }
  return (time.tv_sec * 1000000000LL) + time.tv_nsec;
}
