#ifndef STRATEGIES_INC
#define STRATEGIES_INC

// Rendering strategies
#define VRENDER_MJPG 5        // Hardware decompression
#define VRENDER_YUV420 0      // BC_Bitmap access
#define VRENDER_RGB565 1      // BC_Bitmap access
#define VRENDER_BGR8880 3     // BC_Bitmap access
#define VRENDER_RGB888 2      // Framebuffer
#define VRENDER_VPIXEL 4      // Virtual console


#endif
