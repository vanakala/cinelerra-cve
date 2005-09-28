#ifndef LOCALSESSION_H
#define LOCALSESSION_H

#include "bcwindowbase.inc"
#include "defaults.inc"
#include "edl.inc"
#include "filexml.inc"



// Unique session for every EDL

class LocalSession
{
public:
	LocalSession(EDL *edl);
	~LocalSession();


// Get selected range based on precidence of in/out points and
// highlighted region.
// 1) If a highlighted selection exists it's used.
// 2) If in_point or out_point exists they're used.
// 3) If no in/out points exist, the insertion point is returned.
// highlight_only - forces it to use highlighted region only.
	double get_selectionstart(int highlight_only = 0);
	double get_selectionend(int highlight_only = 0);
	double get_inpoint();
	double get_outpoint();
	int inpoint_valid();
	int outpoint_valid();
	void set_selectionstart(double value);
	void set_selectionend(double value);
	void set_inpoint(double value);
	void set_outpoint(double value);
	void unset_inpoint();
	void unset_outpoint();


	void copy_from(LocalSession *that);
	void save_xml(FileXML *file, double start);
	void load_xml(FileXML *file, unsigned long load_flags);
	int load_defaults(Defaults *defaults);
	int save_defaults(Defaults *defaults);
// Used to copy parameters that affect rendering.
	void synchronize_params(LocalSession *that);

	void boundaries();


	EDL *edl;


// Variables specific to each EDL
// Number of samples if pasted from a clipboard.
// If 0 use longest track
	double clipboard_length;
// Title if clip
	char clip_title[BCTEXTLEN];
	char clip_notes[BCTEXTLEN];
// Folder in parent EDL of clip
	char folder[BCTEXTLEN];

	int loop_playback;
	double loop_start;
	double loop_end;
// Vertical start of track view
	int64_t track_start;
// Horizontal start of view in pixels.  This has to be pixels since either
// samples or seconds would require drawing in fractional pixels.
	int64_t view_start;
// Zooming of the timeline.  Number of samples per pixel.  
	int64_t zoom_sample;
// Amplitude zoom
	int64_t zoom_y;
// Track zoom
	int64_t zoom_track;
// Vertical automation scale
	float automation_min;
	float automation_max;

// Eye dropper
	float red, green, blue;

// Range for CWindow and VWindow preview in seconds.
	double preview_start;
	double preview_end;

private:
// The reason why selection ranges and inpoints have to be separate:
// The selection position has to change to set new in points.
// For editing functions we have a precidence for what determines
// the selection.

	double selectionstart, selectionend;
	double in_point, out_point;
};

#endif
