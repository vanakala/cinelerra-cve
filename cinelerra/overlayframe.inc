#ifndef OVERLAYFRAME_INC
#define OVERLAYFRAME_INC


#define TRANSFER_NORMAL       0
#define TRANSFER_ADDITION     1
#define TRANSFER_SUBTRACT     2
#define TRANSFER_MULTIPLY     3
#define TRANSFER_DIVIDE 	  4
#define TRANSFER_REPLACE      5

// Interpolation types

#define INTERPOLATION_TYPES 3

#define NEAREST_NEIGHBOR 0        // Nearest for enlargement and reduction
#define CUBIC_CUBIC      1        // Bicubic enlargement and reduction
#define CUBIC_LINEAR     2        // Bicubic enlargement and linear reduction
#define LINEAR_LINEAR    3        // Linear enlargement and reduction

class OverlayFrame;

#endif
