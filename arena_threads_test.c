#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>

#include "get_utc_nanoseconds.h"
#include "kr_malloc.h"

static const char HelpMessage[] =
    "Benchmarks the allocator, allowing the caller to set the number of\n"
    "allocations per thread, the number of threads, and the size of each\n"
    "allocation.\n"
    "\n"
    "You can make the allocation size random per allocation by setting it\n"
    "to \"r\", immediately followed by a seed value for the random number\n"
    "generator, e.g. \"r42\". When size randomization is in effect, the\n"
    "maximum allocation size is set by an internal constant (currently\n"
    "%zu).\n"
    "\n"
    "Usage: arena_threads_test iterations thread_count allocation_size"
    "\n";

static size_t iterations;
static size_t thread_count;
static size_t allocation_size;
static const size_t maximum_allocation_size = 0xFFFFUL;
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
    size_t s = allocation_size;
    if (s == 0) {
      s = (unsigned)rand() % maximum_allocation_size;
    }
    char* p = arena_malloc(&a, s, 1);
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

static noreturn void help() {
  fprintf(stderr, HelpMessage, maximum_allocation_size);
  exit(1);
}

int main(int count, char* arguments[]) {
  if (count != 4) {
    help();
  }
  iterations = strtoul(arguments[1], NULL, 0);
  thread_count = strtoul(arguments[2], NULL, 0);
  if (arguments[3][0] != 'r') {
    allocation_size = strtoul(arguments[3], NULL, 0);
  } else {
    if (strlen(arguments[3]) < 2) {
      help();
    }
    const unsigned seed = (unsigned)strtoul(&(arguments[3][1]), NULL, 0);
    srand(seed);
    printf("random seed: %u\n", seed);
    allocation_size = 0;
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
