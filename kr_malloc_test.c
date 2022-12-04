#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "get_utc_nanoseconds.h"
#include "kr_malloc.h"

#define iterations 1000

#if defined(ARENA)
static void test_allocation_time(Arena* a) {
#else
static void test_allocation_time() {
#endif
  const size_t iterations_size = iterations * sizeof(char*);
  char** ps = malloc(iterations_size);
  // Touch every page to ensure the benchmark doesn’t get noisier than it
  // already is due to lazy commitment/page faults.
  for (size_t i = 0; i < iterations_size; i += 4096) {
    ps[i] = NULL;
  }

  const int64_t start = GetUTCNanoseconds();

  for (size_t i = 1; i < iterations; i++) {
#if defined(ORIGINAL_FLAVOR)
    char* p = kr_malloc(i * 1);
#elif defined(ARENA)
    char* p = arena_malloc(a, i, 1);
#else
    char* p = kr_malloc(i, 1);
#endif
    ps[i] = p;
    if (p == NULL) {
      printf("%s\n", strerror(errno));
      exit(errno);
    }
  }

  const int64_t after_allocations = GetUTCNanoseconds();

  for (size_t i = 1; i < iterations; i++) {
#if defined(ARENA)
    arena_free(a, ps[i]);
#else
    kr_free(ps[i]);
#endif
  }

  const int64_t end = GetUTCNanoseconds();
  printf("total: %" PRId64 " ns = mallocs %" PRId64 " ns + frees %" PRId64
         " ns\n",
         end - start, after_allocations - start, end - after_allocations);
}

int main() {
#if defined(ARENA)
  Arena a;
  arena_create(&a, default_minimum_chunk_units);
  test_allocation_time(&a);
  arena_destroy(&a);

  // Make sure passing `NULL` works, too.
  test_allocation_time(NULL);
#else
  test_allocation_time();
#endif
}
