#ifndef CLIP_H
#define CLIP_H

// Math macros

#define CLIP(x, y, z) ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x)))
#define RECLIP(x, y, z) ((x) = ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x))))
#define CLAMP(x, y, z) ((x) = ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x))))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define EQUIV(x, y) (fabs((x) - (y)) < 0.001)
#define DISTANCE(x1, y1, x2, y2) \
(sqrt(((x2) - (x1)) * ((x2) - (x1)) + ((y2) - (y1)) * ((y2) - (y1))))


#endif
