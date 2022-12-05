#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "get_utc_nanoseconds.h"
#include "kr_malloc.h"

#define iterations 1000

static Arena a;

static void* allocate_lots(void* _) {
  (void)_;
  const size_t iterations_size = iterations * sizeof(char*);
  char** ps = malloc(iterations_size);
  // Touch every page to ensure the benchmark doesn’t get noisier than it
  // already is due to lazy commitment/page faults.
  for (size_t i = 0; i < iterations_size; i += 4096) {
    ps[i] = NULL;
  }

  const int64_t start = GetUTCNanoseconds();

  for (size_t i = 1; i < iterations; i++) {
    char* p = arena_malloc(&a, i, 1);
    ps[i] = p;
    if (p == NULL) {
      printf("%s\n", strerror(errno));
      exit(errno);
    }
  }

  const int64_t after_allocations = GetUTCNanoseconds();

  for (size_t i = 1; i < iterations; i++) {
    arena_free(&a, ps[i]);
  }

  const int64_t end = GetUTCNanoseconds();
  printf("total: %" PRId64 " ns = mallocs %" PRId64 " ns + frees %" PRId64
         " ns\n",
         end - start, after_allocations - start, end - after_allocations);
  return NULL;
}

int main() {
  arena_create(&a, default_minimum_chunk_units);

  pthread_t threads[5];
  for (size_t i = 0; i < 5; i++) {
    int e = pthread_create(&threads[i], NULL, allocate_lots, NULL);
    if (e) {
      err(errno, "Could not create thread\n");
    }
  }
  for (size_t i = 0; i < 5; i++) {
    int e = pthread_join(threads[i], NULL);
    if (e) {
      err(errno, "Could not join thread\n");
    }
  }

  arena_destroy(&a);
}
