#ifndef MASKAUTO_H
#define MASKAUTO_H


#include "arraylist.h"
#include "auto.h"
#include "maskauto.inc"
#include "maskautos.inc"

class MaskPoint
{
public:
	MaskPoint();

	int operator==(MaskPoint& ptr);
	MaskPoint& operator=(MaskPoint& ptr);

	float x, y;
// Incoming acceleration
	float control_x1, control_y1;
// Outgoing acceleration
	float control_x2, control_y2;
};

class SubMask
{
public:
	SubMask(MaskAuto *keyframe);
	~SubMask();

	int operator==(SubMask& ptr);
	void copy_from(SubMask& ptr);
	void load(FileXML *file);
	void copy(FileXML *file);
	void dump();

	ArrayList<MaskPoint*> points;
	MaskAuto *keyframe;
};

class MaskAuto : public Auto
{
public:
	MaskAuto(EDL *edl, MaskAutos *autos);
	~MaskAuto();
	
	int operator==(Auto &that);
	int operator==(MaskAuto &that);
	int identical(MaskAuto *src);
	void load(FileXML *file);
	void copy(int64_t start, int64_t end, FileXML *file, int default_auto);
	void copy_from(Auto *src);
	void copy_from(MaskAuto *src);

	void dump();
// Retrieve submask with clamping
	SubMask* get_submask(int number);
	

	ArrayList<SubMask*> masks;
// These are constant for the entire track
	int mode;
	float feather;
// 0 - 100
	int value;
};




#endif
