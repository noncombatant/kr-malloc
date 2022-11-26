# Commentary On ‘Modern’ K&R `malloc`

In this version, I use modern types, modern style, simplify the use of `Header`,
and add the bare-minimum safety of arithmetic checking.

The actual significant change is that `more_memory` (we no longer use magnetic
core memory, so I changed the name of the function!) uses `mmap`, not `sbrk`.
This allocator still never returns memory back to the operating system (by
calling `munmap`), though.

This allocator does not incorporate any of the improvements suggested in the
exercises in K&R, p. 189.

Most of the added lines in this version are comments that explain why the code
is the way it is. Hopefully, the comments are sufficient and correct.
