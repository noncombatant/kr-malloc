// Copyright 2022 by [Chris Palmer](https://noncombatant.org)
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

// Prints a representation of the arena to the given `FILE`.
//
// Returns the number of characters printed, or a negative value if an error
// occurs.
int arena_print(FILE* f, Arena* a) __attribute__((nonnull));

int arena_print(FILE* f, Arena* a) {
  lock(&(a->lock));

  int r = fprintf(f, "Arena %p (minimum chunk units %zu):\n", (void*)a,
                  a->minimum_chunk_units);
  if (r < 0) {
    goto end;
  }

  for (Chunk* c = a->chunk_list; c != NULL; c = c->next) {
    const int x = fprintf(f, "Chunk %p: next: %p, size: %zu\n", (void*)c,
                          (void*)c->next, c->byte_count);
    if (x < 0 || add(r, x, &r)) {
      goto end;
    }
  }

  for (Header* h = &(a->free_list); h != NULL; h = h->next) {
    const int x = fprintf(f, "Header %p: next: %p, unit_count: %zu\n", (void*)h,
                          (void*)h->next, h->unit_count);
    if (x < 0 || add(r, x, &r)) {
      goto end;
    }
    if (h->next == &(a->free_list)) {
      break;
    }
  }

end:
  unlock(&(a->lock));
  return r;
}
