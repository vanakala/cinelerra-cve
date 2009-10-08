
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

#ifndef LOADMODE_INC
#define LOADMODE_INC

class LoadMode;

// Load modes for loading files, importing recordings, importing effects
#define LOAD_NOTHING 0
#define LOAD_REPLACE 1
#define LOAD_REPLACE_CONCATENATE 2
#define LOAD_NEW_TRACKS 3
#define LOAD_CONCATENATE 4
#define LOAD_PASTE 5
#define LOAD_RESOURCESONLY 6

#endif
