#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "get_utc_nanoseconds.h"
#include "kr_malloc.h"

#define iterations 1000

int main() {
  void* ps[iterations];
  const int64_t start = GetUTCNanoseconds();

  for (size_t i = 1; i < iterations; i++) {
#ifdef ORIGINAL_FLAVOR
    void* p = ps[i] = kr_malloc(i * 1);
#else
    void* p = ps[i] = kr_malloc(i, 1);
#endif
    if (p == NULL) {
      printf("%s\n", strerror(errno));
      return errno;
    }
  }

  const int64_t after_allocations = GetUTCNanoseconds();

  for (size_t i = 1; i < iterations; i++) {
    kr_free(ps[i]);
  }

  const int64_t end = GetUTCNanoseconds();
  printf("total: %" PRId64 " ns = mallocs %" PRId64 " ns + frees %" PRId64
         " ns\n",
         end - start, after_allocations - start, end - after_allocations);
}
