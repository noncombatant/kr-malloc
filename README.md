# K&R `malloc`

There is a very simple free list-based implementation of `malloc` in _The C
Programming Language_, 2nd edition, by Brian Kernighan and Dennis Ritchie
(“K&R”). Because it’s 🦃 Holiday Season 🎄 and I am taking time off, I decided
to play around with it.

## `kr_malloc*`

This is the `malloc` implementation from pp. 185 – 189 of K&R. I have tried to
transcribe it exactly, even down to the spacing of the end-of-line comments.

To build on a modern system, I had to make a few trivial adjustments.

## `modern_kr_malloc*`

This is a fairly faithful version of `kr_malloc`, but which replaces `sbrk` with
`mmap`, uses C’s type system somewhat better (it is `-Weverything`-clean), and
does a bit of style cleanup.

## `arena_malloc*`

This version parameterizes the global state in the 2 previous versions by making
an `Arena` object an explicit argument to all functions in the public interface.
This enables various interesting enhancements to the API and to the
implementation.
