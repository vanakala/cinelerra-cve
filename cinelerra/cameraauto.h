#ifndef CAMERAAUTO_H
#define CAMERAAUTO_H

#include "auto.h"


class CameraAuto : public Auto
{
public:
	CameraAuto();
	~CameraAuto();

	int camera_x, camera_y;		// center of view in source frame
								// 0 to source width/height
	float zoom;            		// amount to expand source
	int radius_in, radius_out;            		// radius of turn in or turn out, 
								// depending on which neighbor
								// isn't in a line with this point
	int acceleration_in, acceleration_out;			// acceleration into and out of the point
};





#endif
