#ifndef WORKAROUNDS_H
#define WORKAROUNDS_H

// Work around gcc bugs by forcing external linkage

int64_t quicktime_add(int64_t a, int64_t b);
int64_t quicktime_add3(int64_t a, int64_t b, int64_t c);
uint16_t quicktime_copy(int value);

#endif
