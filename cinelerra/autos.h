#ifndef AUTOS_H
#define AUTOS_H

// Base class for automation lists.
// Units are the native units for the track data type.

#include "auto.h"
#include "edl.inc"
#include "guicast.h"
#include "filexml.inc"
#include "track.inc"

#define AUTOS_VIRTUAL_HEIGHT 160

class Autos : public List<Auto>
{
public:
	Autos(EDL *edl, 
		Track *track);
		
	virtual ~Autos();

	void resample(double old_rate, double new_rate);

	int create_objects();
	void equivalent_output(Autos *autos, int64_t startproject, int64_t *result);
	void copy_from(Autos *autos);
	virtual Auto* new_auto();
	virtual float value_to_percentage();
// Get existing auto on or before position.
// If use_default is true, return default_auto if none exists 
// on or before position.
// Return 0 if none exists and use_default is false.
// If &current is nonzero it is used as a starting point for searching.
	Auto* get_prev_auto(int64_t position, int direction, Auto* &current, int use_default = 1);
	Auto* get_prev_auto(int direction, Auto* &current);
	Auto* get_next_auto(int64_t position, int direction, Auto* &current, int use_default = 1);
// Determine if a keyframe exists before creating it.
	int auto_exists_for_editing(double position);
// Get keyframe for editing with automatic creation if enabled
	Auto* get_auto_for_editing(double position = -1);
// Insert keyframe at the point if it doesn't exist
	Auto* insert_auto(int64_t position);
	void insert_track(Autos *automation, 
		int64_t start_unit, 
		int64_t length_units,
		int replace_default);
	virtual int load(FileXML *xml);
	void paste(int64_t start, 
		int64_t length, 
		double scale, 
		FileXML *file, 
		int default_only);
	void remove_nonsequential(Auto *keyframe);
	void optimize();

	EDL *edl;
	Track *track;
// Default settings if no autos.
// Having a persistent keyframe created problems when files were loaded and
// we wanted to keep only 1 auto.
// Default auto has position 0 except in effects, where multiple default autos
// exist.
	Auto *default_auto;





	int clear_all();
	int insert(int64_t start, int64_t end);
	int paste_silence(int64_t start, int64_t end);
	int copy(int64_t start, 
		int64_t end, 
		FileXML *xml, 
		int default_only,
		int autos_only);
// Stores the background rendering position in result
	void clear(int64_t start, 
		int64_t end, 
		int shift_autos);
	int clear_auto(int64_t position);
	int save(FileXML *xml);
	virtual int slope_adjustment(int64_t ax, double slope);
	virtual float fix_value(float value) {};
	int release_auto();
	virtual int release_auto_derived() {};
	virtual Auto* add_auto(int64_t position, float value) { printf("virtual Autos::add_auto\n"); };
	virtual Auto* append_auto() { printf("virtual Autos::append_auto();\n"); };
	int scale_time(float rate_scale, int scale_edits, int scale_autos, int64_t start, int64_t end);
	int64_t get_length();

// rendering utilities
	int get_neighbors(int64_t start, int64_t end, Auto **before, Auto **after);
// 1 if automation doesn't change
	virtual int automation_is_constant(int64_t start, int64_t end);       
	virtual double get_automation_constant(int64_t start, int64_t end);
	int init_automation(int64_t &buffer_position,
				int64_t &input_start, 
				int64_t &input_end, 
				int &automate, 
				double &constant, 
				int64_t input_position,
				int64_t buffer_len,
				Auto **before, 
				Auto **after,
				int reverse);

	int init_slope(Auto **current_auto, 
				double &slope_start, 
				double &slope_value,
				double &slope_position, 
				int64_t &input_start, 
				int64_t &input_end, 
				Auto **before, 
				Auto **after,
				int reverse);

	int get_slope(Auto **current_auto, 
				double &slope_start, 
				double &slope_end, 
				double &slope_value,
				double &slope, 
				int64_t buffer_len, 
				int64_t buffer_position,
				int reverse);

	int advance_slope(Auto **current_auto, 
				double &slope_start, 
				double &slope_value,
				double &slope_position, 
				int reverse);

	Auto* autoof(int64_t position);   // return nearest auto equal to or after position
										                  // 0 if after all autos
	Auto* nearest_before(int64_t position);    // return nearest auto before or 0
	Auto* nearest_after(int64_t position);     // return nearest auto after or 0

	int color;
	Auto *selected;
	int skip_selected;      // if selected was added
	int64_t selected_position, selected_position_;      // original position for moves
	double selected_value, selected_value_;      // original position for moves
	float virtual_h;  // height cursor moves to cover entire range when track height is less than this
	double min, max;    // boundaries of this auto
	double default_;
	int virtual_center;
	int stack_number;
	int stack_total;
};






#endif
