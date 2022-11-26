CC = clang
CFLAGS = -Weverything -std=c2x -O3 -Wno-poison-system-directories

all: original modern

modern: kr_malloc_test.c modern_kr_malloc.c modern_kr_malloc.h get_utc_nanoseconds.c
	cp modern_kr_malloc.c kr_malloc.c
	cp modern_kr_malloc.h kr_malloc.h
	$(CC) $(CFLAGS) -o kr_malloc_test kr_malloc_test.c kr_malloc.c get_utc_nanoseconds.c
	./kr_malloc_test

original: kr_malloc_test.c original_kr_malloc.c original_kr_malloc.h get_utc_nanoseconds.c
	cp original_kr_malloc.c kr_malloc.c
	cp original_kr_malloc.h kr_malloc.h
	$(CC) $(CFLAGS) -DORIGINAL_FLAVOR -o kr_malloc_test kr_malloc_test.c kr_malloc.c get_utc_nanoseconds.c
	./kr_malloc_test

clean:
	- rm -f *.o
	- rm -f kr_malloc_test
	- rm -f kr_malloc.h kr_malloc.c
	- rm -rf kr_malloc_test.dSYM
