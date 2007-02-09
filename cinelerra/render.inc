#ifndef RENDER_INC
#define RENDER_INC

class PackageRenderer;
class Render;
class RenderItem;
class RenderJob;
class RenderWindow;

// Internal rendering strategies

#define SINGLE_PASS             0
#define FILE_PER_LABEL          1
#define SINGLE_PASS_FARM        2
#define FILE_PER_LABEL_FARM     3
#define BRENDER_FARM            4


#define RANGE_BACKCOMPAT	0
#define RANGE_PROJECT		1
#define RANGE_SELECTION		2
#define RANGE_INOUT		3

#endif
