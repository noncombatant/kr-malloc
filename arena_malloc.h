#include <assert.h>
#include <stdatomic.h>
#include <stddef.h>

// `Arena` is a metadata structure that describes a (set of) allocation
// region(s). You can use 1 for the entire process, or 1 per thread, or 1 per
// object lifetime, or whatever you like.
//
// Each arena has its own free list, regions of memory to allocate from (see
// `Chunk`, below), and lock (to maintain consistency when called by multiple
// threads). Callers can reduce lock contention by giving each thread its own
// arena(s), or otherwise minimizing sharing arenas among threads.
typedef struct Arena Arena;

// Initializes the new `Arena`, setting its `minimum_chunk_units`, which is
// measured in `sizeof(Header)` _units_.
void arena_create(Arena* a, size_t minimum_chunk_units)
    __attribute__((nonnull));

// This is a good value to use for `Arena.minimum_chunk_units` for most
// applications. It is tuned to be appropriate for the platform.
extern const size_t default_minimum_chunk_units;

// Returns a pointer to a memory region containing at least `count * size`
// bytes. Checks the multiplication for overflow.
//
// Returns `NULL` and sets `errno` if there was an error.
void* arena_malloc(Arena* a, size_t count, size_t size)
    __attribute__((malloc, nonnull));

// Puts the memory region that `p` points to back onto the free list.
void arena_free(Arena* a, void* p) __attribute__((nonnull));

// Returns all memory in the `Arena` back to the platform. All allocations made
// inside the arena will be invalid after this function returns.
void arena_destroy(Arena* a) __attribute__((nonnull));

// Implementation details below this point.

// A `Chunk` is a unit of memory provided from outside the allocator (such as
// the OS via `mmap`). This list keeps track of them so that we can release them
// back to the OS.
typedef struct Chunk {
  struct Chunk* next;
  size_t byte_count;
} Chunk;

// A `Header` describes an entry in an `Arena`’s free list: a region of memory
// that can be divided up and returned to the caller.
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

// An `Arena` is metadata that describes a set of `Chunk`s and the `Header`s
// that make up its free list. A caller can create and use as many arenas as
// they like.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
struct Arena {
  // This implementation uses a simple spin lock. Under contention, it starts
  // affecting performance.
  atomic_flag lock;

  // The head of the chunk list.
  Chunk* chunk_list;

  // The head of the free list.
  Header free_list;

  // Where we last left off in a search of the free list.
  //
  // `NULL` is a sentinel value indicating that `free_list` has not yet been
  // initialized.
  Header* free_list_start;

  // We always request at least this amount from the operating system. The value
  // should be chosen (a) to reduce pressure on the page table; and (b) to
  // reduce the number of times we need to invoke the kernel.
  size_t minimum_chunk_units;
};
#pragma clang diagnostic pop
