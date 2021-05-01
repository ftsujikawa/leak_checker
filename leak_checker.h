#ifndef __LEAK_CHECKER_H__
#define __LEAK_CHECKER_H__
#include <stdio.h>

extern void leak_checker_init(void);
extern void leak_checker_finish(void);

typedef struct
{
  void *p;
  size_t sz;
  void *caller;
} LC_T;

#endif