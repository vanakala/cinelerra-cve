
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

#ifndef MAINSESSION_INC
#define MAINSESSION_INC

class MainSession;

// Current operation in main window
enum
{
	NO_OPERATION,     
	DRAG_EDITHANDLE1,    	// Waiting to move out of drag threshold
	DRAG_EDITHANDLE2,    	// Dragging outside drag threshold
	DRAG_PARTITION,   
	SELECT_REGION,    
	DRAG_ASSET,       
	DRAG_ATRANSITION, 
	DRAG_VTRANSITION, 
	DRAG_AEFFECT,     
	DRAG_VEFFECT,     
	DRAG_AEFFECT_COPY,  // 10
	DRAG_VEFFECT_COPY, 
	DRAG_EDIT,         
	DRAG_FADE,         
	DRAG_CZOOM,         
	DRAG_PZOOM,         
	DRAG_PAN_PRE,          
	DRAG_PAN,          
	DRAG_PLAY,         
	DRAG_MUTE,         
	DRAG_CAMERA_X,       
	DRAG_CAMERA_Y,       
	DRAG_CAMERA_Z,       
	DRAG_MASK_PRE,      // 20
	DRAG_MASK,
	DRAG_MODE_PRE,         
	DRAG_MODE,         
	DRAG_PROJECTOR_X,    
	DRAG_PROJECTOR_Y,    
	DRAG_PROJECTOR_Z,    
	DRAG_PLUGINKEY_PRE,    
	DRAG_PLUGINKEY,    
	DRAG_PLUGINHANDLE1,        // Waiting to move out of drag threshold
	DRAG_PLUGINHANDLE2,         // Dragging outside drag threshold
	DRAG_NUDGE
};


#endif
