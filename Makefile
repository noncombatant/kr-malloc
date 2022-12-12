CC = clang
STANDARD = -Weverything -std=c2x -Wno-poison-system-directories
DEBUG = -g -O0
RELEASE = -O3 -DNDEBUG
CFLAGS = $(STANDARD) $(RELEASE)

all: original modern arena

original: malloc_test.c original_kr_malloc.c original_kr_malloc.h get_utc_nanoseconds.c
	$(CC) $(CFLAGS) -DORIGINAL -o original_test malloc_test.c original_kr_malloc.c get_utc_nanoseconds.c
	./original_test

modern: malloc_test.c modern_kr_malloc.c modern_kr_malloc.h get_utc_nanoseconds.c
	$(CC) $(CFLAGS) -DMODERN -o modern_test malloc_test.c modern_kr_malloc.c get_utc_nanoseconds.c
	./modern_test

arena: malloc_test.c arena_malloc.c arena_malloc.h get_utc_nanoseconds.c
	$(CC) $(CFLAGS) -DARENA -o arena_test malloc_test.c arena_malloc.c get_utc_nanoseconds.c
	./arena_test
	$(CC) $(CFLAGS) -o arena_threads_test arena_threads_test.c arena_malloc.c get_utc_nanoseconds.c
	./arena_threads_test 100000 5 64

clean:
	- rm -f *.o
	- rm -f original_test modern_test arena_test arena_threads_test
	- rm -rf *.dSYM
