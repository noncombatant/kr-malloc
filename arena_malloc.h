#include <assert.h>
#include <stddef.h>

// `Arena` is a metadata structure that describes a (set of) allocation
// region(s). You can use 1 for the entire process, or 1 per thread, or 1 per
// object lifetime, or whatever you like.
typedef struct Arena Arena;

// Initializes the new `Arena`, setting its `minimum_chunk_units`, which is
// measured in `sizeof(Header)` _units_.
void arena_create(Arena* a, size_t minimum_chunk_units);

// This is a good value to use for `Arena.minimum_chunk_units` for most
// applications. It is tuned to be appropriate for the platform.
extern size_t default_minimum_chunk_units;

// Returns a pointer to a memory region containing at least `count * size`
// bytes. Checks the multiplication for overflow.
//
// If `a` is `NULL`, allocates memory in the default global arena.
//
// Returns `NULL` and sets `errno` if there was an error.
void* arena_malloc(Arena* a, size_t count, size_t size);

// Puts the memory region that `p` points to back onto the free list.
void arena_free(Arena* a, void* p);

// Returns all memory in the `Arena` back to the platform. All allocations made
// inside the arena will be invalid after this function returns.
void arena_destroy(Arena* a);

// Implementation details below this point.

typedef struct Header {
  struct Header* next;
  size_t unit_count;
  // Add padding if the `static_assert` below fires.
  // char _[count];
} Header;

// `long double` is (probably) the largest machine type. (If it’s not on your
// machine, change this `typedef`.) To ensure that allocations are properly
// aligned for all machine types, we assert that `Header` (the minimum unit of
// allocation) has the right size. This may require padding.
typedef long double Alignment;
static_assert(sizeof(Header) == sizeof(Alignment), "Add padding to `Header`");

typedef struct Chunk {
  struct Chunk* next;
  size_t byte_count;
} Chunk;

struct Arena {
  Chunk* chunk_list;

  Header free_list;

  // `NULL` is a sentinel value indicating that `free_list` has not yet been
  // initialized.
  Header* free_list_start;

  // We always request at least this amount from the operating system. The value
  // should be chosen (a) to reduce pressure on the page table; and (b) to
  // reduce
  // the number of times we need to invoke the kernel.
  size_t minimum_chunk_units;
};
