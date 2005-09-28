#ifndef _ERROR_H
#define _ERROR_H

#include <stdio.h>

#define SUCCESS 0
#define FAILURE 1

#define TRUE 1
#define FALSE 0

// print a warning (when return value is ignored) 
#define WARN(format, args...)  fprintf(stderr, "[%s:%d %s()] " format "\n",  \
                                      __FILE__, __LINE__, __func__, ## args)

// print a warning with return value of FAILURE
#define ERROR(format, args...) (WARN(format, ## args), FAILURE)

// NOTE: ASSERT is used only when you want to issue a warning in a case 
//       that should not happen and when there is no way to recover.  
//       If there is any way to recover, use ERROR() and handle the problem.
//#define ASSERT(x, args...) if (! x) WARN("ASSERT FAILED (" #x ") " args)

#endif /* _ERROR_H */


