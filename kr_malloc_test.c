#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "get_utc_nanoseconds.h"
#include "kr_malloc.h"

#define iterations 1000

// Touch every page to ensure the benchmark doesn’t get noisier than it already
// is due to lazy commitment/page faults.
static void touch_pages(void* p, size_t byte_count) {
  char* pp = p;
  for (size_t i = 0; i < byte_count; i += 4096) {
    pp[i] = 0;
  }
}

int main() {
  const size_t iterations_size = iterations * sizeof(char*);
  char** ps = malloc(iterations_size);
  touch_pages(ps, iterations_size);

#if defined(ARENA)
  Arena a;
  arena_create(&a, default_minimum_chunk_units);
#endif

  const int64_t start = GetUTCNanoseconds();

  for (size_t i = 1; i < iterations; i++) {
#if defined(ORIGINAL_FLAVOR)
    char* p = kr_malloc(i * 1);
#elif defined(ARENA)
    char* p = arena_malloc(&a, i, 1);
#else
    char* p = kr_malloc(i, 1);
#endif
    ps[i] = p;
    if (p == NULL) {
      printf("%s\n", strerror(errno));
      return errno;
    }
  }

  const int64_t after_allocations = GetUTCNanoseconds();

  for (size_t i = 1; i < iterations; i++) {
#if defined(ARENA)
    arena_free(&a, ps[i]);
#else
    kr_free(ps[i]);
#endif
  }

  const int64_t end = GetUTCNanoseconds();
  printf("ns per malloc: %" PRId64 ", ns per free: %" PRId64 "\n",
         (after_allocations - start) / iterations,
         (end - after_allocations) / iterations);

#if defined(ARENA)
  arena_destroy(&a);
#endif
}
