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
	DRAG_CAMERA_PRE,       
	DRAG_CAMERA,       
	DRAG_MASK_PRE,      // 20
	DRAG_MASK,
	DRAG_MODE_PRE,         
	DRAG_MODE,         
	DRAG_PROJECTOR_PRE,    
	DRAG_PROJECTOR,    
	DRAG_PLUGINKEY_PRE,    
	DRAG_PLUGINKEY,    
	DRAG_PLUGINHANDLE1,        // Waiting to move out of drag threshold
	DRAG_PLUGINHANDLE2         // Dragging outside drag threshold
};


#endif
