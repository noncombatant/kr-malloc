#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "kr_malloc.h"

#define iterations 1000

int main() {
  void* ps[iterations];
  for (size_t i = 1; i < iterations; i++) {
#ifdef ORIGINAL_FLAVOR
    void* p = ps[i] = kr_malloc(i * 1);
#else
    void* p = ps[i] = kr_malloc(i, 1);
#endif
    if (p == NULL) {
      printf("%s\n", strerror(errno));
      return errno;
    }
  }

  for (size_t i = 1; i < iterations; i++) {
    kr_free(ps[i]);
  }
}
