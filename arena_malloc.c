#include <sys/mman.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>

#include "arena_malloc.h"

size_t default_minimum_chunk_size = ((size_t)1 << 21) / sizeof(Header);

void arena_open(Arena* a, size_t minimum_chunk_size) {
  a->free_list.next = NULL;
  a->free_list.size = 0;
  a->free_list_start = NULL;
  a->minimum_chunk_size = minimum_chunk_size;
}

// Returns a pointer to a memory region containing at least `count`
// `Header`-sized objects.
//
// Returns `NULL` and sets `errno` if there was an error.
static Header* get_more_memory(Arena* a, size_t count) {
  if (count < a->minimum_chunk_size) {
    count = a->minimum_chunk_size;
  }
  Header* h = mmap(NULL, count * sizeof(Header), PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  if (h == MAP_FAILED) {
    return NULL;
  }
  h->size = count;
  arena_free(a, h + 1);
  return a->free_list_start;
}

void* arena_malloc(Arena* a, size_t count, size_t size) {
  const size_t byte_count = count * size;
  if (byte_count < count || byte_count < size) {
    errno = EINVAL;
    return NULL;
  }

  // Round up to an integer number of `Header`-sized units, and add 1 to account
  // for the actual `Header` metadata that describes the region.
  const size_t unit_count =
      (byte_count + sizeof(Header) - 1) / sizeof(Header) + 1;

  Header* p;
  Header* previous;

  // Determine whether `free_list` has been initialized. Note that if it has not
  // been, the `for` loop below this one will fall through to the call to
  // `get_more_memory` on the 1st iteration.
  if ((previous = a->free_list_start) == NULL) {
    a->free_list.next = a->free_list_start = previous = &(a->free_list);
    a->free_list.size = 0;
  }

  // Iterate down `free_list`, looking for regions large enough or requesting a
  // new region from the platform.
  for (p = previous->next; true; previous = p, p = p->next) {
    if (p->size >= unit_count) {
      if (p->size == unit_count) {
        // If this region is exactly the size we need, we're done.
        previous->next = p->next;
      } else {
        // If this region is larger than we need, return the tail end of it to
        // the caller, and adjust the size of the region `Header`.
        p->size -= unit_count;
        p += p->size;
        p->size = unit_count;
      }
      a->free_list_start = previous;
      return (void*)(p + 1);
    }

    // If we have wrapped around to the beginning of `free_list` (its end always
    // points to its beginning), we need to get more memory.
    if (p == a->free_list_start) {
      if ((p = get_more_memory(a, unit_count)) == NULL) {
        return NULL;
      }
    }
  }
}

void arena_free(Arena* a, void* p) {
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
  if (h + h->size == current->next) {
    h->size += current->next->size;
    h->next = current->next->next;
  } else {
    h->next = current->next;
  }

  // If `h` is at the end of the segment, join it to the beginning. These 2
  // joins ensure that we coalesce segments into larger segments.
  if (current + current->size == h) {
    current->size += h->size;
    current->next = h->next;
  } else {
    current->next = h;
  }

  a->free_list_start = current;
}

void arena_close(Arena* a) {
  // TODO
  (void)a;
}
