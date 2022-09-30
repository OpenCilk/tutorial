#include <stdio.h>
#include <stdlib.h>

#include <cilk/cilk.h>

#include "ctimer.h"

long fib(long n) {
  if (n < 2)
    return n;
  long x = cilk_spawn fib(n-1);
  long y = fib(n-2);
  cilk_sync;
  return x + y;
}

int main(int argc, char *argv[]) {
  long n = 10;
  if (argc > 1)
    n = atol(argv[1]);

  ctimer_t t;
  ctimer_start(&t);

  long result = fib(n);

  ctimer_stop(&t);
  ctimer_measure(&t);

  printf("fib(%ld) = %ld\n", n, result);
  ctimer_print(t, "fib");
  return 0;
}
