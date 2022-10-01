#include "cmaintask.h"
#ifdef __cplusplus
extern "C" {
#endif

void task1wrapper(void *);
void testclasstask2(void *);

void
  testwork1(void *p)
{
    task1wrapper(p);
}
void
testwork2(void *p)
{
    testclasstask2(p);
}

#ifdef __cplusplus
}
#endif