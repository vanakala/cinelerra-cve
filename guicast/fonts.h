#ifndef FONTS_H
#define FONTS_H

#define LARGEFONT  0
#define MEDIUMFONT 2
#define SMALLFONT  1
#define MEDIUM_7SEGMENT 4

// Specialized fonts not available in every widget
#define BOLDFACE    0x8000
#define LARGEFONT_3D  (LARGEFONT | BOLDFACE)
#define MEDIUMFONT_3D  (MEDIUMFONT | BOLDFACE)
#define SMALLFONT_3D   (SMALLFONT | BOLDFACE)

#endif
