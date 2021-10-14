#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

static void exit_errno(int err)
{
  fprintf(stderr, "%s\n", strerror(err));
  exit(EXIT_FAILURE);
}

static double to_double(char const *s)
{
  char *end;
  double res = strtod(s, &end);
  if (*end != '\0') {
    fprintf(stderr, "Cannot make sense of '%s'\n", s);
    exit(EXIT_FAILURE);
  }
  return res;
}

static double now(void)
{
  struct timeval tv;
  if (0 != gettimeofday(&tv, NULL)) exit_errno(errno);
  return tv.tv_sec + 0.000001 * tv.tv_usec;
}

static void *waster(double *duration)
{
  assert(duration);
  assert(*duration > 0.);

  double start = now();
  double end = start + *duration;
  int spins = 10000;

  while (spins > 100) {
    double t0 = now();
    for (int i = 0; i < spins; i++) asm(".rept 100; nop; .endr");
    double t1 = now();
    double dt = t1 - t0;
    double rem = end - t1;
    if (rem < 0) break;
    double spins_ = 0.8 * (spins * rem) / dt;
    spins = spins_ > INT_MAX ? INT_MAX : spins_;
    printf("spins=%d\n", spins);
  }

  return NULL;
}

int main(int nargs, char **args)
{
  if (nargs != 3) {
    fprintf(stderr, "waste num_cpu num_seconds\n");
    return EXIT_FAILURE;
  }

  double num_cpu = to_double(args[1]);
  double num_seconds = to_double(args[2]);

  if (num_cpu < 0 || num_seconds < 0) {
    fprintf(stderr, "very funny\n");
    exit(EXIT_FAILURE);
  }

  int num_threads = ceil(num_cpu);
  pthread_t th[num_threads];

  for (int i = 0; i < num_threads; i++, num_cpu -= 1.) {
    assert(num_cpu > 0.);
    double *duration = malloc(sizeof(*duration));
    if (! duration) {
      fprintf(stderr, "seriously?!\n");
      exit(EXIT_FAILURE);
    }
    *duration = num_cpu >= 1. ? num_seconds : num_seconds * num_cpu;

    int err = pthread_create(th+i, NULL, (void*(*)(void*))waster, (void*)duration);
    if (err) exit_errno(err);
  }

  for (int i = 0; i < num_threads; i++) {
    int err = pthread_join(th[i], NULL);
    if (err) exit_errno(err);
  }

  return EXIT_SUCCESS;
}
