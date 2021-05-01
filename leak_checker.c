#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/param.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "leak_checker.h"

#define N 500

static LC_T lc[N];

static void *leak_checker_malloc_hook(size_t, const void *);
static void *leak_checker_realloc_hook(void *, size_t, const void *);
static void leak_checker_free_hook(void *, const void *);

static char *get_sourceline(const void *);

static void *(*old_malloc_hook)(size_t, const void *);
static void *(*old_realloc_hook)(void *, size_t, const void *);
static void (*old_free_hook)(void *, const void *);

void leak_checker_init(void)
{
  for (int i = 0; i < N; ++i)
  {
    lc[i].p = NULL;
    lc[i].sz = 0;
    lc[i].caller = NULL;
  }
  old_malloc_hook = __malloc_hook;
  old_realloc_hook = __realloc_hook;
  old_free_hook = __free_hook;
  __malloc_hook = leak_checker_malloc_hook;
  __realloc_hook = leak_checker_realloc_hook;
  __free_hook= leak_checker_free_hook;
}

void leak_checker_finish(void) {
  char *sl;

  __malloc_hook = old_malloc_hook;
  __realloc_hook = old_realloc_hook;
  __free_hook = old_free_hook;

  printf("\n===== Memory Leak Summary =====\n");
  for (int i = 0; i < N; ++i) {
    if (lc[i].p != NULL) {
      if ((sl = get_sourceline(lc[i].caller)) == NULL) {
        printf("no free area(addr:0x%x, size:%lu) called from 0x%p\n", lc[i].p, lc[i].sz, lc[i].caller);
      }
      else {
        printf("no free area(addr:0x%x, size:%lu) called from %s\n", lc[i].p, lc[i].sz, sl);
      }
    }
  }
}

static void *leak_checker_malloc_hook(size_t size, const void *caller)
{
  void *result;
  char *sl;

  __malloc_hook = old_malloc_hook;
  __realloc_hook = old_realloc_hook;
  __free_hook = old_free_hook;

  result = malloc(size);
  for (int i = 0; i < N; ++i) {
    if (lc[i].p == NULL) {
      lc[i].p = result;
      lc[i].sz = size;
      lc[i].caller = caller;
      break;
    }
  }
  old_malloc_hook = __malloc_hook;
  old_realloc_hook = __realloc_hook;
  old_free_hook = __free_hook;

  if ((sl = get_sourceline(caller)) == NULL) {
    printf("malloc(%u) called from %p returns %p\n", size, caller, result);
  }
  else {
    printf("malloc(%u) called from %s returns %p\n", size, sl, result);
  }

  __malloc_hook = leak_checker_malloc_hook;
  __realloc_hook = leak_checker_realloc_hook;
  __free_hook = leak_checker_free_hook;

  return result;
}

static void *leak_checker_realloc_hook(void *ptr, size_t size, const void *caller)
{
  void *result;
  char *sl;

  __malloc_hook = old_malloc_hook;
  __realloc_hook = old_realloc_hook;
  __free_hook = old_free_hook;

  result = realloc(ptr, size);
  for (int i = 0; i < N; ++i) {
    if (lc[i].p == ptr) {
      lc[i].p = result;
      lc[i].sz = size;
      lc[i].caller = caller;
      break;
    }
  }

  old_malloc_hook = __malloc_hook;
  old_realloc_hook = __realloc_hook;
  old_free_hook = __free_hook;

  if ((sl = get_sourceline(caller)) == NULL) {
    printf("realloc(%p, %u) called from %p returns %p\n", ptr, size, caller, result);
  }
  else {
    printf("realloc(%p, %u) called from %s returns %p\n", ptr, size, sl, result);
  }

  __malloc_hook = leak_checker_malloc_hook;
  __realloc_hook = leak_checker_realloc_hook;
  __free_hook = leak_checker_free_hook;

  return result;
}

static void leak_checker_free_hook(void *ptr, const void *caller)
{
  char *sl;
  int flag;
  void *sv_caller;

  __malloc_hook = old_malloc_hook;
  __realloc_hook = old_realloc_hook;
  __free_hook = old_free_hook;

  flag = 0;
  for (int i = 0; i < N; ++i) {
    if (lc[i].p == ptr) {
      lc[i].p = NULL;
      lc[i].sz = 0;
      sv_caller = lc[i].caller;
      lc[i].caller = NULL;
      flag = 1;
      break;
    }
  }
  if (flag == 0) {
    if ((sl = get_sourceline(sv_caller)) == NULL) {
      printf("***** double free(0x%x) called from 0x%p *****\n", ptr, sv_caller);
    }
    else {
      printf("***** double free(0x%x) called from %s *****\n", ptr, sl);
    }
  }
  else {
    free(ptr);
  }

  old_malloc_hook = __malloc_hook;
  old_realloc_hook = __realloc_hook;
  old_free_hook = __free_hook;

  if ((sl = get_sourceline(caller)) == NULL) {
    printf("free(%p) called from %p\n", ptr, caller);
  }
  else {
    printf("free(%p) called from %s\n", ptr, sl);
  }

  __malloc_hook = leak_checker_malloc_hook;
  __realloc_hook = leak_checker_realloc_hook;
  __free_hook = leak_checker_free_hook;
}

static char *get_sourceline(const void *ptr) {
  static char buf[PATH_MAX * 2];
  char src[512];

  snprintf(src, sizeof(src), "/usr/bin/addr2line %p", ptr);

  FILE *fp;
  if ((fp = popen(src, "r")) == NULL) {
    return NULL;
  }
  if (fgets(buf, sizeof(buf), fp) == NULL) {
    return NULL;
  }
  buf[strlen(buf) - 1] = '\0';

  pclose(fp);

  if (strcmp(buf, "??:?") == 0 || strcmp(buf, "??:0") == 0) {
    return NULL;
  }
  else {
    return buf;
  }
}

#ifdef _TEST_
int main(void)
{
  void *p;

  leak_checker_init();

  p = malloc(1024);
  p = calloc(2048, 2);
  p = realloc(p, 4096);
  free(p);
  free(p);

  leak_checker_finish();

  return 0;
}
#endif