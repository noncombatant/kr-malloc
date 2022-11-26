// Copyright 2022 Chris Palmer, https://noncombatant.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#include <time.h>

int64_t GetUTCNanoseconds(void) {
  struct timespec time;
  if (clock_gettime(CLOCK_REALTIME, &time)) {
    return 0;
  }
  return (time.tv_sec * 1000000000LL) + time.tv_nsec;
}
