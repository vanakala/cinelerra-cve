#ifndef JPEGWRAPPER_H
#define JPEGWRAPPER_H

// jpegsrc doesn't work for C++ so wrap it

#ifdef __cplusplus
extern "C" {
#endif

#include <jpeglib.h>

#ifdef __cplusplus
}
#endif

#endif
