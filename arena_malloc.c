// Copyright 2022 by [Chris Palmer](https://noncombatant.org)
// SPDX-License-Identifier: Apache-2.0

#include <sys/mman.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arena_malloc.h"

#define add(a, b, result) __builtin_add_overflow(a, b, result)
#define mul(a, b, result) __builtin_mul_overflow(a, b, result)

// Set this to true to run `check_free` in `arena_free`. `check_free`’s
// algorithm is very slow, making it unsuitable for production.
static const bool do_check_free = false;

// Set this to true to `memset` regions with `overwrite_on_free_value` before
// freeing them. The cost of the `memset` will come to dominate the time taken
// to free as the allocation size grows, so this may be unsuitable for
// production.
static const bool overwrite_on_free = false;
static const char overwrite_on_free_value = 0x0c;

// For more information about locks and tuning them, see
// https://rigtorp.se/spinlock/. Here, we optimize for simple implementation.

static void lock(atomic_flag* f) {
  do {
  } while (atomic_flag_test_and_set_explicit(f, memory_order_acquire));
}

static void unlock(atomic_flag* f) {
  atomic_flag_clear_explicit(f, memory_order_release);
}

const size_t default_minimum_chunk_units = ((size_t)1 << 21) / sizeof(Header);
static size_t page_size = 0;

static void arena_create_internal(Arena* a, size_t minimum_chunk_units) {
  atomic_flag_clear(&(a->lock));
  a->chunk_list = NULL;
  a->free_list.next = NULL;
  a->free_list.unit_count = 0;
  a->free_list_start = NULL;
  a->minimum_chunk_units = minimum_chunk_units;
}

void arena_create(Arena* a, size_t minimum_chunk_units) {
  if (page_size == 0) {
    page_size = (size_t)sysconf(_SC_PAGESIZE);
  }
  const size_t m = page_size / sizeof(Header);
  minimum_chunk_units = minimum_chunk_units >= m ? minimum_chunk_units : m;
  arena_create_internal(a, minimum_chunk_units);
}

// Prepends the new `Chunk`, of `byte_count` bytes, to the `a->chunk_list`.
static void prepend_chunk(Arena* a, Chunk* chunk, size_t byte_count) {
  assert(page_size != 0);
  assert(byte_count % page_size == 0);
  if (a->chunk_list == NULL) {
    a->chunk_list = chunk;
    a->chunk_list->next = NULL;
    a->chunk_list->byte_count = byte_count;
  } else {
    Chunk* c = a->chunk_list;
    a->chunk_list = chunk;
    a->chunk_list->next = c;
    a->chunk_list->byte_count = byte_count;
  }
}

// Puts the memory region `p` points to back onto the free list. This function
// is both part of the implementation of `arena_free` and of `get_more_memory`.
static void free_internal(Arena* a, void* p) {
  // The `Header` is always immediately before the region we returned to the
  // caller of `malloc`.
  Header* h = (Header*)p - 1;

  // Iterate over `free_list` looking for the segment containing `h`.
  Header* current;
  for (current = a->free_list_start; !(h > current && h < current->next);
       current = current->next) {
    if (current >= current->next && (h > current || h < current->next)) {
      // Freed block at start of end of arena.
      break;
    }
  }

  // If `h` is at the beginning of the segment, join it to the end.
  if (h + h->unit_count == current->next) {
    h->unit_count += current->next->unit_count;
    h->next = current->next->next;
  } else {
    h->next = current->next;
  }

  // If `h` is at the end of the segment, join it to the beginning. These 2
  // joins ensure that we coalesce segments into larger segments.
  if (current + current->unit_count == h) {
    current->unit_count += h->unit_count;
    current->next = h->next;
  } else {
    current->next = h;
  }

  a->free_list_start = current;
}

// Returns a pointer to the 1st `Header` in the `Chunk`.
static Header* get_1st_header(Chunk* chunk) {
  // Advance past the 1st page, which we use solely for `a->chunk_list`. Yes, we
  // waste 1 page just to hold the `Chunk` structure. The other alternative is
  // to maintain a dedicated memory mapping for it. That could be more
  // space-efficient, at the cost of additional code and (possibly?) reduced
  // data locality.
  char* char_chunk = ((char*)chunk) + page_size;

  // Regarding this pointer trickery, clang warns us:
  //
  //   cast from 'char *' to 'Header *' (aka 'union header *') increases
  //   required alignment from 1 to 8 [-Wcast-align]
  //
  // so we assert that we are still aligned, as documentation:
  assert((uintptr_t)chunk % 8 == 0);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"
  return (Header*)char_chunk;
#pragma clang diagnostic pop
}

// Returns a pointer to a memory region containing at least `count`
// `Header`-sized objects.
//
// Returns `NULL` and sets `errno` if there was an error.
static Header* get_more_memory(Arena* a, size_t unit_count) {
  unit_count =
      unit_count < a->minimum_chunk_units ? a->minimum_chunk_units : unit_count;
  size_t byte_count;
  if (mul(unit_count, sizeof(Header), &byte_count) ||
      add(byte_count, page_size, &byte_count)) {
    errno = EINVAL;
    return NULL;
  }

  Chunk* chunk = mmap(NULL, byte_count, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  if (chunk == MAP_FAILED) {
    return NULL;
  }
  prepend_chunk(a, chunk, byte_count);

  Header* h = get_1st_header(chunk);
  h->unit_count = unit_count;
  free_internal(a, h + 1);
  return a->free_list_start;
}

// Computes the count of `Header`-sized units necessary to store `count * size`
// bytes, + 1 for the actual `Header` metadata that describes the region.
static size_t get_unit_count(size_t count, size_t size) {
  size_t byte_count;
  if (mul(count, size, &byte_count) || byte_count == 0) {
    return 0;
  }
  size_t unit_count;
  if (add(byte_count, sizeof(Header) - 1, &unit_count)) {
    return 0;
  }
  return unit_count / sizeof(Header) + 1;
}

void* arena_malloc(Arena* a, size_t count, size_t size) {
  const size_t unit_count = get_unit_count(count, size);
  if (unit_count == 0) {
    errno = EINVAL;
    return NULL;
  }

  Header* p;
  Header* previous;
  lock(&(a->lock));

  // Determine whether `a->free_list` has been initialized. Note that if it has
  // not been, the `for` loop below this one will fall through to the call to
  // `get_more_memory` on the 1st iteration.
  if ((previous = a->free_list_start) == NULL) {
    a->free_list.next = a->free_list_start = previous = &(a->free_list);
    a->free_list.unit_count = 0;
  }

  // Iterate down `a->free_list`, looking for regions large enough or requesting
  // a new region from the platform.
  for (p = previous->next; true; previous = p, p = p->next) {
    if (p->unit_count >= unit_count) {
      if (p->unit_count == unit_count) {
        // If this region is exactly the size we need, we're done.
        previous->next = p->next;
      } else {
        // If this region is larger than we need, return the tail end of it to
        // the caller, and adjust the size of the region `Header`.
        p->unit_count -= unit_count;
        p += p->unit_count;
        p->unit_count = unit_count;
      }
      a->free_list_start = previous;
      unlock(&(a->lock));
      return p + 1;
    }

    // If we have wrapped around to the beginning of `free_list` (its end always
    // points to its beginning), we need to get more memory.
    if (p == a->free_list_start) {
      if ((p = get_more_memory(a, unit_count)) == NULL) {
        unlock(&(a->lock));
        return NULL;
      }
    }
  }
}

// Now that we have the `Chunk` information in the `Arena`, we can test to see
// whether `p` is actually in any chunk we have allocated. That is still not a
// perfect test that `p` is exactly a pointer previously returned by
// `arena_malloc`, but it’s better than what we started with.
//
// `abort`s if `p` is not inside a known `Chunk`.
static void check_free(Arena* a, void* p) {
  const uintptr_t pu = (uintptr_t)p;
  uintptr_t usable_start = 0, usable_end = 0;
  for (Chunk* c = a->chunk_list; c != NULL; c = c->next) {
    const uintptr_t cu = (uintptr_t)c;
    assert(cu % page_size == 0);
    usable_start = cu + page_size;
    usable_end = cu + c->byte_count - sizeof(Header);
    if (pu >= usable_start && pu <= usable_end) {
      return;
    }
  }
  abort();
}

void arena_free(Arena* a, void* p) {
  lock(&(a->lock));
  if (do_check_free) {
    check_free(a, p);
  }
  if (overwrite_on_free) {
    const Header* h = (Header*)p - 1;
    memset(p, overwrite_on_free_value, (h->unit_count - 1) * sizeof(Header));
  }
  free_internal(a, p);
  unlock(&(a->lock));
}

void arena_destroy(Arena* a) {
  lock(&(a->lock));
  for (Chunk* c = a->chunk_list; c != NULL;) {
    Chunk* next = c->next;
    if (munmap(c, c->byte_count)) {
      abort();
    }
    c = next;
  }
  a->chunk_list = NULL;
  a->free_list_start = NULL;
  memset(&(a->free_list), 0, sizeof(a->free_list));
  unlock(&(a->lock));
}
