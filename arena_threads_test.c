#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "get_utc_nanoseconds.h"
#include "kr_malloc.h"

static size_t iterations = 1000;
static size_t thread_count = 5;
static size_t allocation_size = 64;

static Arena a;

// Touch every page to ensure the benchmark doesn’t get noisier than it already
// is due to lazy commitment/page faults.
static void touch_pages(void* p, size_t byte_count) {
  char* pp = p;
  for (size_t i = 0; i < byte_count; i += 4096) {
    pp[i] = 0;
  }
}

static void* allocate_lots(void* _) {
  (void)_;
  const size_t iterations_size = iterations * sizeof(char*);
  char** ps = malloc(iterations_size);
  touch_pages(ps, iterations_size);

  const int64_t start = GetUTCNanoseconds();

  for (size_t i = 1; i < iterations; i++) {
    char* p = arena_malloc(&a, allocation_size, 1);
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
  printf("ns per malloc: %'" PRId64 ", ns per free: %'" PRId64 "\n",
         (uint64_t)(after_allocations - start) / iterations * thread_count,
         (uint64_t)(end - after_allocations) / iterations * thread_count);
  return NULL;
}

int main(int count, char* arguments[]) {
  if (count > 1) {
    iterations = strtoul(arguments[1], NULL, 0);
    if (count > 2) {
      thread_count = strtoul(arguments[2], NULL, 0);
      if (count > 3) {
        allocation_size = strtoul(arguments[3], NULL, 0);
      }
    }
  }

  arena_create(&a, default_minimum_chunk_units);

  pthread_t* threads = calloc(thread_count, sizeof(pthread_t));
  for (size_t i = 0; i < thread_count; i++) {
    int e = pthread_create(&threads[i], NULL, allocate_lots, NULL);
    if (e) {
      err(errno, "Could not create thread\n");
    }
  }
  for (size_t i = 0; i < thread_count; i++) {
    int e = pthread_join(threads[i], NULL);
    if (e) {
      err(errno, "Could not join thread\n");
    }
  }

  arena_destroy(&a);
}
