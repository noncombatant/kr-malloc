# Commentary On `arena_malloc`

This allocator is mostly the same as modern\_kr\_malloc.c, with the API-breaking
change that the `malloc` and `free` functions take an `Arena` object. The arena
is a metadata structure that maintains the state that is global (and sometimes
implicit) in the original and modern versions of the allocator. The arena is a
powerful abstraction that enables callers to fine-tune the behavior of the
allocator.

The coarse-grained lock on the Arena makes the locking regime simple, but it
ruins multi-threaded performance. You really want to reduce lock contention by
using multiple arenas. For example:

* 1 arena per thread
* 1 arena per object type
* 1 arena per object size class
* 1 arena per lifetime
* ...

Separating allocations this way makes sense for reasons other than
multi-threaded time-efficiency, as well. Here are some examples of why:

* Separating objects that are of fixed size from those that are variable in size
  can improve data locality and space-efficiency.
* Grouping objects by size size class can improve data locality and
  space-efficiency.
* Grouping objects that all share a lifetime can simplify (and speed up)
  cleanup: you can use `arena_destroy` rather than invoking `arena_free` on each
  object.
* If you have a set of objects that seem to suffer from use-after-free (UAF)
  bugs (for example, DOM nodes in a web renderer), it is good to separate them
  from arbitrarily-sized objects (such as strings). This can function as an
  exploit mitigation, making it harder for an attacker to exploit the UAF bugs.

You can use the `arena_threads_test` program with different parameters to
measure how the allocator performs under different loads.
