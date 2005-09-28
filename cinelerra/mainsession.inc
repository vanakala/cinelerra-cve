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
