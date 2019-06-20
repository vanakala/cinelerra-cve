
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

#ifndef MWINDOW_INC
#define MWINDOW_INC

#define PROGRAM_NAME "Cinelerra-CVE"
#define DEFAULT_THEME "S.U.V."
#define CONFIG_FILE "Cinelerra_rc"

// Behavior of region selections
#define SELECTION_SAMPLES 0
#define SELECTION_EDIT 1

// Behaviour of paste effects operations
#define PASTE_NONE    1
#define PASTE_AUTOS   2
#define PASTE_PLUGINS 3
#define PASTE_ALL     4
class MWindow;

#endif
