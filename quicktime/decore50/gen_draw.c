// gen_draw.c //

#include "mp4_decoder.h"

#include "gen_draw.h"

/**
 *
**/

extern int MV[2][6][MBR+1][MBC+2]; // motion vectors
extern int modemap[MBR+1][MBC+2]; // macroblock modes

/***/

static void colorBlock_mv(unsigned char *buff, int stride, int mb_xpos, int mb_ypos, int bnum, int xsize, int ysize);
static void drawline(int stride, int x0, int y0, int dx, int dy, unsigned char *buff, char colour, int xsize, int ysize);
static void check_and_colour(unsigned char *buff, int x0, int y0, int xsize, int ysize, char colour);

/***/

// Purpose: draw motion vectors in a displayable buffer
void colorBuffer_mv(unsigned char *buff, int stride, int xsize, int ysize)
{
	int mb_xsize, mb_ysize;
	int mb_xpos, mb_ypos;
	int bnum;

	mb_xsize = xsize>>4;
	mb_ysize = ysize>>4;

	for (mb_ypos = 0; mb_ypos < mb_ysize; mb_ypos++)
	{
		for (mb_xpos = 0; mb_xpos < mb_xsize; mb_xpos++)
		{
			switch (modemap[mb_xpos+1][mb_ypos+1])
			{
			case MODE_INTER: case MODE_INTER_Q:
				{
					colorBlock_mv(buff, stride, mb_xpos, mb_ypos, -1, xsize, ysize);
				}
				break;
			case MODE_INTER4V: case MODE_INTER4V_Q:
				{
					for (bnum = 0; bnum < 4; bnum++)
					{
						colorBlock_mv(buff, stride, mb_xpos, mb_ypos, bnum, xsize, ysize);
					}
				}
				break;
			default:
				break;
			}
		}
	}
}

/***/

// Purpose: draw motion vector for this block, bnum is -1 if it's an entire macroblock
static void colorBlock_mv(unsigned char *buff, int stride, int mb_xpos, int mb_ypos, int bnum, int xsize, int ysize)
{
	int mvx, mvy;
	int xpos = mb_xpos<<4;
	int ypos = mb_ypos<<4;

	if (bnum == -1) {
		xpos += 8;
		ypos += 8;
		bnum = 0; // retrieve correct block number for MV extraction
	} 
	else {
		xpos += (bnum & 1) ? 12 : 4;
		ypos += (bnum & 2) ? 12 : 4;
	}

	mvx = MV[0][bnum][mb_xpos+1][mb_ypos+1];
	mvy = MV[1][bnum][mb_xpos+1][mb_ypos+1];

	drawline(stride, xpos, ypos, mvx, mvy, buff, 0, xsize, ysize);
}

/***/

// John Funnell < johnf@mail.nu >
// Purpose: line drawing function - pilfered from the Net
static void drawline(int stride, int x0, int y0, int dx, int dy, unsigned char *buff, char colour, int xsize, int ysize) {
	int y1 = y0 + dy;
	int x1 = x0 + dx;
	int stepx, stepy;
	int fraction;

	if (dy < 0) { 
		dy = -dy;  stepy = -stride; 
	} 
	else { 
		stepy = stride; 
	}
	if (dx < 0) { 
		dx = -dx;  stepx = -1; 
	} else { 
		stepx = 1; 
	}

	// retrieve pixel coord
	dy <<= 1; 
	dx <<= 1;

	// retrieve image coordinates
	y0 *= stride;
	y1 *= stride;

	check_and_colour(buff, x0, y0, xsize, ysize, colour);
	if (dx > dy) {
		fraction = dy - (dx >> 1);
		while (x0 != x1) {
			if (fraction >= 0) {
				y0 += stepy;
				fraction -= dx;
			}
			x0 += stepx;
			fraction += dy;
			check_and_colour(buff, x0, y0, xsize, ysize, colour);
		}
	} else {
		fraction = dx - (dy >> 1);
		while (y0 != y1) {
			if (fraction >= 0) {
				x0 += stepx;
				fraction -= dy;
			}
			y0 += stepy;
			fraction += dx;
			check_and_colour(buff, x0, y0, xsize, ysize, colour);
		}
	}
}

/***/

static void check_and_colour(unsigned char *buff, int x0, int y0, int xsize, int ysize, char colour)
{
	if (x0 < 0)
		return;
	if (x0 > xsize)
		return;
	if (y0 < 0)
		return;
	if (y0 > (xsize*(ysize-1)))
		return;

	buff[x0+y0] = colour;
}
