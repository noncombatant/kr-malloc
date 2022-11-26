# Commentary On K&R `malloc`

It’s good to learn as much as possible from the past, and thankfully we have
learned an incredible amount — about ethics, safety, reliability, type systems,
formal methods, compiler design and implementation, operating systems... — since
K&R were working.

This code is a good example of why people like C: it’s very small yet very
powerful, it makes a complex problem tractable and legible, its constant factor
for time cost is very low, and it stands as an exemplar of the often-strong
argument that (much of the time) simple implementations are sufficient or even
ideal:

> [Rob Pike’s 5 Rules of
> Programming](https://users.ece.utexas.edu/~adnan/pike.html)
>
>   1. You can’t tell where a program is going to spend its time. Bottlenecks
>   occur in surprising places, so don’t try to second guess and put in a speed
>   hack until you’ve proven that’s where the bottleneck is.
>   2. Measure. Don’t tune for speed until you’ve measured, and even then don’t
>   unless one part of the code overwhelms the rest.
>   3. Fancy algorithms are slow when n is small, and n is usually small. Fancy
>   algorithms have big constants. Until you know that n is frequently going to
>   be big, don’t get fancy. (Even if n does get big, use Rule 2 first.)
>   4. Fancy algorithms are buggier than simple ones, and they’re much harder to
>   implement. Use simple algorithms as well as simple data structures.
>   5. Data dominates. If you’ve chosen the right data structures and organized
>   things well, the algorithms will almost always be self-evident. Data
>   structures, not algorithms, are central to programming.
>
> Pike’s rules 1 and 2 restate Tony Hoare’s famous maxim “Premature optimization
> is the root of all evil.” Ken Thompson rephrased Pike’s rules 3 and 4 as “When
> in doubt, use brute force.”. Rules 3 and 4 are instances of the design
> philosophy KISS. Rule 5 was previously stated by Fred Brooks in The Mythical
> Man-Month. Rule 5 is often shortened to “write stupid code that uses smart
> objects”.

However, it also demonstrates some of the classic problems of C: the terseness
that comes from using `ed` on [a physically-taxing Teletype
console](https://en.wikipedia.org/wiki/Teletype_Model_33), a safety record only
a [MotoGP rider](https://www.motogp.com/) could love, use of globals even when
passing objects as parameters would make the program more general and useful,
and weak typing.

Ultimately, though, this `malloc` is a beautiful teaching tool because it shows
us the doors of possibility. It doesn’t open them, leaving us to explore and
discover.

## Alignment

```
typedef long Align;   /* for alignment to long boundary */

union header {        /* block header: */
    struct {
        union header *ptr;  /* next block if on free list */
        unsigned size;      /* size of this block */
    } s;
    Align x;          /* force alignment of blocks */
};
```

Despite what I just said about weak typing, the `Header` type is a great use of
C’s type system to achieve a combination of portability and generality.

However, `size` should be `size_t`, not `unsigned` (i.e. `unsigned int`). On
many systems, `size_t` is wider than `unsigned int`. On such systems, this
allocator would be incapable of creating space for a copy of a very long string
(`strlen` returns `size_t`) or a very large `mmap`ed region (which is also
`size_t` bytes long).

`long` is a decent stab at determining the largest size/alignment of the
primitive objects, although `long double` did exist at the time K&R were
writing. (`long long` did not, though.) So we can’t be certain that `long` was
**guaranteed** to have been the right choice for all machines at the time. (See
[C Portability Lessons from Weird
Machines](https://begriffs.com/posts/2018-11-15-c-portability.html) for
example.) On some machines, a persnickety `#ifdef` may have been necessary for a
correct program.

Comments of complete sentences explaining the why, rather than the what, would
be more useful. Meaningful names and type names are the best way to explain the
what; the remaining reason for comments is then to explain why.

For example, the comment for `s.size` is redundant (like so much Javadoc!), yet
still doesn’t manage to tell you that it’s the size **in `Header`-sized units,
not in bytes**. The comment for `s.ptr` could be obviated by calling the field
`s.next`. Its type name already tells us it’s a pointer, freeing up the field
name to tell us more. A comment that its value will be `NULL` if there is no
next header in the list might help, too. (Modern `optional` types would obviate
that comment as well.)

## Design: Arenas

```
typedef union header Header;

static Header base;       /* empty list to get started */
static Header *freep = NULL;     /* start of free list */

#define NALLOC   1024    /* minimum #units to request */
```

Design issue: `base`, `freep`, and `NALLOC` could all be members of an `Arena`
object, [allowing callers to maintain multiple memory allocation
areas](https://en.wikipedia.org/wiki/Region-based_memory_management) and to tune
the performance trade-offs. `NALLOC` can have a large effect on overall
allocator efficiency, and having a single global arena requires a global lock in
a multi-threaded environment — bad for time-efficiency. Modern allocators, such
as the Windows allocator, TCMalloc, and Partition Alloc make arenas an explicit
part of the allocator API for these reasons. Additionally, enabling callers to
release entire arenas at once can significantly ease manual memory management
— a classic C difficulty that leads to inefficiency, bugs, and safety problems.

Again, this code is intended to be a simple example, meeting the pre-existing
`malloc`/`free` API. But generations of programmers have learned from this and
other examples in the book, and so the problems persist.

By using the term _arena_, K&R allude to the possibility of having more than 1
(p. 187): “After setting the `size` field, `morecore` inserts the additional
memory into the arena by calling `free`.” Exercise 8.8 (p. 189) also asks us to
imagine supplementing an arena with caller-provided storage. It opens up the
possibility of having an arena populated by static storage and another provided
dynamically by the operating system. It’s too bad the proposed `bfree` function,
or something like it, never made it into the standard C library. It might have
saved some developers from having to roll their own.

## Getting More Memory From The Operating System

```
/* morecore:   ask system for more memory */
static Header *morecore(unsigned nu)
{
    char *cp, *sbrk(int);
    Header *up;

    if (nu < NALLOC)
        nu = NALLOC;
    cp = sbrk(nu * sizeof(Header));
    if (cp == (char *) -1)  /* no space at all */
        return NULL;
    up = (Header *) cp;
    up->s.size = nu;
    kr_free((void *)(up+1));
    return freep;
}
```

This `morecore` requests memory from the kernel by invoking `sbrk`, which
increases the size of the program’s [heap
segment](https://en.wikipedia.org/wiki/Data_segment) in memory. The end of the
heap segment was called the _program break_. (See also an [old-timey copy of the
Unix manual](https://man.cat-v.org/unix-6th/2/break).)

Modern allocators use `mmap`, not `sbrk`, to request new regions of memory from
the operating system, and most modern kernels provide mapped pages to programs
lazily, in the page fault handler.

K&R complain that `sbrk` uses -1 as its sentinel value, but a bigger problem is
that its argument is an `int`. `INT_MAX` is smaller than `UINT_MAX`, so this
allocator cannot allocate as much memory as its interface claims. This could be
a problem; consider [a 32-bit system with a 3/1
split](https://lwn.net/Articles/75174/) or a 32-bit machine with 16-bit `int`s,
in which it should be possible to create an object larger than `INT_MAX`.

To represent the span of the full address space requires a `size_t`; to allow
`sbrk` to reduce the program break as well as increase it, the type could have
been `ptrdiff_t`, or perhaps Unix could have had a 2nd system call (also taking
a `size_t`) for reducing the program break.

(K&R does not mention either `ssize_t` or `[u]intptr_t`; perhaps C89 or POSIX
defined them. Perhaps `sbrk` was invented even before `long` or `ptrdiff_t`.
Modern Linux provides the declaration `void* sbrk(intptr_t increment)`, and its
manual page says “Various systems use various types for the argument of
`sbrk()`. Common are `int`, `ssize_t`, `ptrdiff_t`, `intptr_t`.”)

It is possible, but not convenient, to implement `morecore` in a way that
satisfies its `unsigned` interface or the correct `size_t` interface. For
example, it could call `sbrk` multiple times to get a large region.

In any case, even in C/C++ code written recently, we see the apparent assumption
that everything is an `int` — which is no more true than that [“all the world’s
a VAX”](https://www.lysator.liu.se/c/ten-commandments.html). The int-evitable
consequences, such as (to choose at random)
[CVE-2022-29824](https://gitlab.gnome.org/GNOME/libxml2/-/commit/6c283d83eccd940bcde15634ac8c7f100e3caefd),
are sad. The int-fection began with [C’s predecessor,
B](https://en.wikipedia.org/wiki/B_(programming_language)), where even ASCII
characters were an afterthought.

## `malloc`: The Actual Allocator

```
/* malloc:  general-purpose storage allocator */
void *kr_malloc(unsigned nbytes)
{
    Header *p, *prevp;
    //Header *morecore(unsigned);
    unsigned nunits;

    nunits = (nbytes+sizeof(Header)-1)/sizeof(Header) + 1;
    if ((prevp = freep) == NULL) {  /* no free list yet */
        base.s.ptr = freep = prevp = &base;
        base.s.size = 0;
    }
    for (p = prevp->s.ptr; ;  prevp = p, p = p->s.ptr) {
        if (p->s.size >= nunits) {     /* big enough */
            if (p->s.size == nunits)      /* exactly */
                prevp->s.ptr = p->s.ptr;
            else {              /* allocate tail end */
                p->s.size -= nunits;
                p += p->s.size;
                p->s.size = nunits;
            }
            freep = prevp;
            return (void *)(p+1);
        }
        if (p == freep)  /* wrapped around free list */
            if ((p = morecore(nunits)) == NULL)
                return NULL;   /* none left */
    }
}
```

This is an admirably simple walk through a singly-linked list, doing the minimum
work and incurring the lowest possible costs in each case.

A key problem with this design is that heap metadata (the `Header` structure) is
_in line_ with memory returned to the caller. This creates a problem: spatial
unsafety (such as a buffer overrun or a type-confusion) can enable attackers
exploiting buggy programs to read and/or write `Header` structures. Attackers
find this a very valuable capability. Ideally — although this is easier said
than done — heap metadata would be stored _out of line_ from caller-usable
regions.

Here again we see the `unsigned` argument type, which should be `size_t`. (Page
252 shows `malloc` and `calloc` taking `size_t`. I suspect this example code was
written for the 1st edition of the book.)

Worse, we know that many (or even most) call sites will need to do a
multiplication along the lines of `count * sizeof(Whatever)`, but experience
shows that most authors do not remember to use `size_t` for the types of the
factors and terms, nor to check that the arithmetic did not overflow. The
`calloc` interface, `void* calloc(size_t count, size_t size)`, enables callers
to defer checking the multiplication to the callee, which is a substantial
improvement.

Even better would be for C to standardize non-standard compiler intrinsics like
GCC’s and Clang’s `__builtin_mul_overflow`. (It’s nice to want things!)

Interestingly, this implementation always asks the operating system for exactly
as much as it needs (`p = morecore(nunits)`), rather than asking for more and
paying for the cost of the additional space by invoking the system less often
(saving time). Together with the very small value for `NALLOC`, we can surmise
that storage space was very expensive at the time.

A modern value for `NALLOC` would have to be at least 1 machine page (4,096
bytes or more — it’s not even possible to allocate less than a page). Even that
value is way too low because it would incur a heavy cost on the OS’ page table.
(Small requests to the OS increase the likelihood that the OS will need to
create many page-table entries, each of which uses up some space in the kernel.
Additionally, each request to the kernel incurs a substantial time cost.
Therefore, modern allocators assume they are running on a modern machine with a
large memory, and therefore make larger requests to the OS. Partition Alloc, for
example, sets its equivalent of `NALLOC` to 2 MiB.)

## `free`: Putting Storage Chunks Back On The Free List

```
/* free:   put block ap in free list */
void kr_free(void *ap)
{
    Header *bp, *p;

    bp = (Header *)ap - 1;     /* point to block header */
    for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
        if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
            break;  /* freed block at start of end of arena */

    if (bp + bp->s.size == p->s.ptr) { /* join to upper nbr */
        bp->s.size += p->s.ptr->s.size;
        bp->s.ptr = p->s.ptr->s.ptr;
    } else
        bp->s.ptr = p->s.ptr;
    if (p + p->s.size == bp) {         /* join to lower nbr */
        p->s.size += bp->s.size;
        p->s.ptr = bp->s.ptr;
    } else
        p->s.ptr = bp;
    freep = p;
}
```

This `free` never returns memory to the OS, which would require checking that a
`free`-managed block is directly adjacent to the current program break and then
calling `sbrk` with, presumably (?) a negative argument. However, `sbrk` appears
not to work on macOS, and negative values appear to be treated as positive on
Linux. Perhaps it was not possible to return memory to early Unix systems.

This implementation has no way to check whether the region you are `free`ing was
ever allocated by `malloc` in the first place, cannot tell whether `ap` has
already been placed on the free list, and blindly assumes that 1
`Header`’s-worth of bytes before `ap` actually is a header (it might not be).

This implementation is therefore buggy in an exploitable way (see [Memory safety
on Wikipedia](https://en.wikipedia.org/wiki/Memory_safety) and [Heap feng shui
on Wikipedia](https://en.wikipedia.org/wiki/Heap_feng_shui)). 

Some implementations go to various lengths to frustrate exploitation, including
heap canaries (an additional magic number stored in the allocator’s equivalent
of `Header`, and sometimes elsewhere), [guard
pages](https://www.cisa.gov/uscert/bsi/articles/knowledge/coding-practices/guard-pages),
keeping additional bookkeeping data structures to detect whether a given pointer
is really live and was really allocated by `malloc`, keeping objects of
different sizes and/or types in different arenas, and moving heap metadata (such
as `Header`) _out of line_ (i.e. not immediately adjacent to memory given to the
caller). There are probably more techniques than those. For more information,
see e.g.:

* [kalloc_type](https://security.apple.com/blog/towards-the-next-generation-of-xnu-memory-safety/)
* [Scudo](https://llvm.org/docs/ScudoHardenedAllocator.html)
* [PartitionAlloc](https://chromium.googlesource.com/chromium/src/base/allocator/partition_allocator/+/refs/heads/main/PartitionAlloc.md)
* [Isolation Alloc](https://github.com/struct/isoalloc)

Indeed, exercise 8-7 (p. 189) asks us to “Improve these routines so they take
more pains with error checking.” Safety-aware allocators since the time of K&R
have been an extensive exercise in doing that homework.
