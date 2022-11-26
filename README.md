# K&R `malloc`

There is a very simple freelist-based implementation of `malloc` in _The C
Programming Language_, 2nd edition, by Brian Kernighan and Dennis Ritchie
(“K&R”). Because it’s 🦃 Holiday Season 🎄 and I am taking time off, I decided
to play around with it.

## `kr_malloc*`

This is the `malloc` implementation from pp. 185 – 189 of K&R. I have tried to
transcribe it exactly, even down to the spacing of the end-of-line comments.

To build on a modern system, I had to make a few trivial adjustments.

## `kr_malloc_commentary.md`

This is my commentary of the K&R implementation, from a modern perspective.
