#ifndef OVERLAYFRAME_INC
#define OVERLAYFRAME_INC

// Modes

#define TRANSFER_TYPES 7

#define TRANSFER_NORMAL       0
#define TRANSFER_ADDITION     1
#define TRANSFER_SUBTRACT     2
#define TRANSFER_MULTIPLY     3
#define TRANSFER_DIVIDE 	  4
#define TRANSFER_REPLACE      5
#define TRANSFER_MAX          6

// Interpolation types


#define NEAREST_NEIGHBOR 0        // Nearest for enlargement and reduction
#define CUBIC_CUBIC      1        // Bicubic enlargement and reduction
#define CUBIC_LINEAR     2        // Bicubic enlargement and linear reduction
#define LINEAR_LINEAR    3        // Not explicitely called but used for user

class OverlayFrame;

#endif
