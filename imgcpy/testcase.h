#ifndef _TEST_CASE_H_
#define _TEST_CASE_H_

typedef struct tTestCase {
  unsigned TestCaseNo;
  const char* const pTextCaseTitle;
  unsigned(*pTestCaseFxn)();
} tTestCase;

extern tTestCase aTestCases[];

#define CHK(_ASSUMPTION_)                                       \
  if (!(_ASSUMPTION_))                                          \
  {                                                             \
    printf ("Error in file `%s' on line %u (res=%u)!\n",        \
            __FILE__, (unsigned)__LINE__, (unsigned)res);       \
    goto lend;                                                  \
  }

#endif // _TEST_CASE_H_
