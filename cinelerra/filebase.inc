#ifndef FILEBASE_INC
#define FILEBASE_INC

class FileBase;

// color models for transfers
// Want to abstract the color model down to the pixel level so pass
// the color model to the transfer routine.

#define FILEBASE_GREY 1
#define FILEBASE_YUV 2
#define FILEBASE_RAW 3
#define FILEBASE_RGBA 4


#endif
