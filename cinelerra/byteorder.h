
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef BYTEORDER_H
#define BYTEORDER_H

#include <sys/types.h>

inline int get_byte_order()
{                // 1 if little endian
	return (*(u_int32_t*)"a   ") & 0x00000001;
}

#define SWAP_ITERATE \
					byte1 = buffer1[i]; \
      				byte2 = buffer2[i]; \
      				buffer1[i] = byte2; \
      				buffer2[i] = byte1; \
					i += 2;
      							                                         
#define SWAP_24BIT_ITERATE \
					byte1 = buffer1[i]; \
      				byte2 = buffer2[i]; \
      				byte3 = buffer3[i]; \
      				buffer1[i] = byte3; \
      				buffer2[i] = byte2; \
      				buffer3[i] = byte1; \
      				i += 3;

#define SWAP_32BIT_ITERATE \
					byte1 = buffer1[i]; \
      				byte2 = buffer2[i]; \
      				byte3 = buffer3[i]; \
      				byte4 = buffer4[i]; \
      				buffer1[i] = byte4; \
      				buffer2[i] = byte1; \
      				buffer3[i] = byte2; \
      				buffer4[i] = byte3; \
      				i += 4;

inline int swap_bytes(int wordsize, unsigned char *buffer, long len)
{
	register unsigned char byte1, byte2, byte3, byte4;
	register unsigned char *buffer1 = buffer;
	register unsigned char *buffer2 = buffer + 1;
	register unsigned char *buffer3 = buffer + 2;
	register unsigned char *buffer4 = buffer + 3;

	register long i = 0, j = 0, k = 0;

//printf("swap bytes\n");

	switch(wordsize)
	{
		case 1:
			return 0;
			break;
		
		case 2:
  			len -= 8;
  			while(i < len){
    			SWAP_ITERATE
    			SWAP_ITERATE
    			SWAP_ITERATE
    			SWAP_ITERATE
  			}

  			len += 8;
  			while(i < len){
    			SWAP_ITERATE
  			}
			return 0;
			break;

		case 3:
  			len -= 12;
  			while(i < len){
    			SWAP_24BIT_ITERATE
    			SWAP_24BIT_ITERATE
    			SWAP_24BIT_ITERATE
    			SWAP_24BIT_ITERATE
  			}

  			len += 12;
  			while(i < len){
    			SWAP_24BIT_ITERATE
  			}
			return 0;
			break;

		case 4:
			len -= 16;
			while(i < len)
			{
				SWAP_32BIT_ITERATE
				SWAP_32BIT_ITERATE
				SWAP_32BIT_ITERATE
				SWAP_32BIT_ITERATE
			}

			len += 16;
			while(i < len)
			{
				SWAP_32BIT_ITERATE
			}
			return 0;
			break;
	}
	return 1;
}

#endif
