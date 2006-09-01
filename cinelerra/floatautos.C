#include "automation.inc"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "floatauto.h"
#include "floatautos.h"
#include "track.h"
#include "localsession.h"
#include "transportque.inc"

FloatAutos::FloatAutos(EDL *edl,
				Track *track, 
				float default_)
 : Autos(edl, track)
{
	this->default_ = default_;
	type = AUTOMATION_TYPE_FLOAT;
}

FloatAutos::~FloatAutos()
{
}

void FloatAutos::straighten(int64_t start, int64_t end)
{
	FloatAuto *current = (FloatAuto*)first;
	while(current)
	{
		FloatAuto *previous_auto = (FloatAuto*)PREVIOUS;
		FloatAuto *next_auto = (FloatAuto*)NEXT;

// Is current auto in range?		
		if(current->position >= start && current->position < end)
		{
			float current_value = current->value;

// Determine whether to set the control in point.
			if(previous_auto && previous_auto->position >= start)
			{
				float previous_value = previous_auto->value;
				current->control_in_value = (previous_value - current_value) / 6.0;
				if(!current->control_in_position)
					current->control_in_position = -track->to_units(1.0, 0);
			}

// Determine whether to set the control out point
			if(next_auto && next_auto->position < end)
			{
				float next_value = next_auto->value;
				current->control_out_value = (next_value - current_value) / 6.0;
				if(!current->control_out_position)
					current->control_out_position = track->to_units(1.0, 0);
			}
		}
		current = (FloatAuto*)NEXT;
	}
}

int FloatAutos::draw_joining_line(BC_SubWindow *canvas, int vertical, int center_pixel, int x1, int y1, int x2, int y2)
{
	if(vertical)
		canvas->draw_line(center_pixel - y1, x1, center_pixel - y2, x2);
	else
		canvas->draw_line(x1, center_pixel + y1, x2, center_pixel + y2);
}

Auto* FloatAutos::add_auto(int64_t position, float value)
{
	FloatAuto* current = (FloatAuto*)autoof(position);
	FloatAuto* result;
	
	insert_before(current, result = (FloatAuto*)new_auto());

	result->position = position;
	result->value = value;
	
	return result;
}

Auto* FloatAutos::new_auto()
{
	FloatAuto *result = new FloatAuto(edl, this);
	result->value = default_;
	return result;
}

int FloatAutos::get_testy(float slope, int cursor_x, int ax, int ay)
{
	return (int)(slope * (cursor_x - ax)) + ay;
}

int FloatAutos::automation_is_constant(int64_t start, 
	int64_t length, 
	int direction,
	double &constant)
{
	int total_autos = total();
	int64_t end;
	if(direction == PLAY_FORWARD)
	{
		end = start + length;
	}
	else
	{
		end = start + 1;
		start -= length;
	}


// No keyframes on track
	if(total_autos == 0)
	{
		constant = ((FloatAuto*)default_auto)->value;
		return 1;
	}
	else
// Only one keyframe on track.
	if(total_autos == 1)
	{
		constant = ((FloatAuto*)first)->value;
		return 1;
	}
	else
// Last keyframe is before region
	if(last->position <= start)
	{
		constant = ((FloatAuto*)last)->value;
		return 1;
	}
	else
// First keyframe is after region
	if(first->position > end)
	{
		constant = ((FloatAuto*)first)->value;
		return 1;
	}

// Scan sequentially
	int64_t prev_position = -1;
	for(Auto *current = first; current; current = NEXT)
	{
		int test_current_next = 0;
		int test_previous_current = 0;
		FloatAuto *float_current = (FloatAuto*)current;

// keyframes before and after region but not in region
		if(prev_position >= 0 &&
			prev_position < start && 
			current->position >= end)
		{
// Get value now in case change doesn't occur
			constant = float_current->value;
			test_previous_current = 1;
		}
		prev_position = current->position;

// Keyframe occurs in the region
		if(!test_previous_current &&
			current->position < end && 
			current->position >= start)
		{

// Get value now in case change doesn't occur
			constant = float_current->value;

// Keyframe has neighbor
			if(current->previous)
			{
				test_previous_current = 1;
			}

			if(current->next)
			{
				test_current_next = 1;
			}
		}

		if(test_current_next)
		{
//printf("FloatAutos::automation_is_constant 1 %d\n", start);
			FloatAuto *float_next = (FloatAuto*)current->next;

// Change occurs between keyframes
			if(!EQUIV(float_current->value, float_next->value) ||
				!EQUIV(float_current->control_out_value, 0) ||
				!EQUIV(float_next->control_in_value, 0))
			{
				return 0;
			}
		}

		if(test_previous_current)
		{
			FloatAuto *float_previous = (FloatAuto*)current->previous;

// Change occurs between keyframes
			if(!EQUIV(float_current->value, float_previous->value) ||
				!EQUIV(float_current->control_in_value, 0) ||
				!EQUIV(float_previous->control_out_value, 0))
			{
// printf("FloatAutos::automation_is_constant %d %d %d %f %f %f %f\n", 
// start, 
// float_previous->position, 
// float_current->position, 
// float_previous->value, 
// float_current->value, 
// float_previous->control_out_value, 
// float_current->control_in_value);
				return 0;
			}
		}
	}

// Got nothing that changes in the region.
	return 1;
}

double FloatAutos::get_automation_constant(int64_t start, int64_t end)
{
	Auto *current_auto, *before = 0, *after = 0;
	
// quickly get autos just outside range	
	get_neighbors(start, end, &before, &after);

// no auto before range so use first
	if(before)
		current_auto = before;
	else
		current_auto = first;

// no autos at all so use default value
	if(!current_auto) current_auto = default_auto;

	return ((FloatAuto*)current_auto)->value;
}


float FloatAutos::get_value(int64_t position, 
	int direction, 
	FloatAuto* &previous, 
	FloatAuto* &next)
{
	double slope;
	double intercept;
	int64_t slope_len;
// Calculate bezier equation at position
	float y0, y1, y2, y3;
 	float t;

	previous = (FloatAuto*)get_prev_auto(position, direction, (Auto* &)previous, 0);
	next = (FloatAuto*)get_next_auto(position, direction, (Auto* &)next, 0);

// Constant
	if(!next && !previous)
	{
		return ((FloatAuto*)default_auto)->value;
	}
	else
	if(!previous)
	{
		return next->value;
	}
	else
	if(!next)
	{
		return previous->value;
	}
	else
	if(next == previous)
	{
		return previous->value;
	}
	else
	{
		if(direction == PLAY_FORWARD &&
			EQUIV(previous->value, next->value) &&
			EQUIV(previous->control_out_value, 0) &&
			EQUIV(next->control_in_value, 0))
		{
			return previous->value;
		}
		else
		if(direction == PLAY_REVERSE &&
			EQUIV(previous->value, next->value) &&
			EQUIV(previous->control_in_value, 0) &&
			EQUIV(next->control_out_value, 0))
		{
			return previous->value;
		}
	}


// Interpolate
	y0 = previous->value;
	y3 = next->value;

	if(direction == PLAY_FORWARD)
	{
		y1 = previous->value + previous->control_out_value * 2;
		y2 = next->value + next->control_in_value * 2;
		t = (double)(position - previous->position) / 
			(next->position - previous->position);
// division by 0
		if(next->position - previous->position == 0) return previous->value;
	}
	else
	{
		y1 = previous->value + previous->control_in_value * 2;
		y2 = next->value + next->control_out_value * 2;
		t = (double)(previous->position - position) / 
			(previous->position - next->position);
// division by 0
		if(previous->position - next->position == 0) return previous->value;
	}

 	float tpow2 = t * t;
	float tpow3 = t * t * t;
	float invt = 1 - t;
	float invtpow2 = invt * invt;
	float invtpow3 = invt * invt * invt;
	
	float result = (  invtpow3 * y0
		+ 3 * t     * invtpow2 * y1
		+ 3 * tpow2 * invt     * y2 
		+     tpow3            * y3);
//printf("FloatAutos::get_value %f %f %d %d %d %d\n", result, t, direction, position, previous->position, next->position);

	return result;



// 	get_fade_automation(slope,
// 		intercept,
// 		position,
// 		slope_len,
// 		PLAY_FORWARD);
// 
// 	return (float)intercept;
}


void FloatAutos::get_fade_automation(double &slope,
	double &intercept,
	int64_t input_position,
	int64_t &slope_len,
	int direction)
{
	Auto *current = 0;
	FloatAuto *prev_keyframe = 
		(FloatAuto*)get_prev_auto(input_position, direction, current);
	FloatAuto *next_keyframe = 
		(FloatAuto*)get_next_auto(input_position, direction, current);
	int64_t new_slope_len;

	if(direction == PLAY_FORWARD)
	{
		new_slope_len = next_keyframe->position - prev_keyframe->position;

//printf("FloatAutos::get_fade_automation %d %d %d\n", 
//	prev_keyframe->position, input_position, next_keyframe->position);

// Two distinct automation points within range
		if(next_keyframe->position > prev_keyframe->position)
		{
			slope = ((double)next_keyframe->value - prev_keyframe->value) / 
				new_slope_len;
			intercept = ((double)input_position - prev_keyframe->position) * slope + prev_keyframe->value;

			if(next_keyframe->position < input_position + new_slope_len)
				new_slope_len = next_keyframe->position - input_position;
			slope_len = MIN(slope_len, new_slope_len);
		}
		else
// One automation point within range
		{
			slope = 0;
			intercept = prev_keyframe->value;
		}
	}
	else
	{
		new_slope_len = prev_keyframe->position - next_keyframe->position;
// Two distinct automation points within range
		if(next_keyframe->position < prev_keyframe->position)
		{
			slope = ((double)next_keyframe->value - prev_keyframe->value) / new_slope_len;
			intercept = ((double)prev_keyframe->position - input_position) * slope + prev_keyframe->value;

			if(prev_keyframe->position > input_position - new_slope_len)
				new_slope_len = input_position - prev_keyframe->position;
			slope_len = MIN(slope_len, new_slope_len);
		}
		else
// One automation point within range
		{
			slope = 0;
			intercept = next_keyframe->value;
		}
	}
}

void FloatAutos::get_extents(float *min, 
	float *max,
	int *coords_undefined,
	int64_t unit_start,
	int64_t unit_end)
{
	if(!edl)
	{
		printf("FloatAutos::get_extents edl == NULL\n");
		return;
	}

	if(!track)
	{
		printf("FloatAutos::get_extents track == NULL\n");
		return;
	}

// Use default auto
	if(!first)
	{
		FloatAuto *current = (FloatAuto*)default_auto;
		if(*coords_undefined)
		{
			*min = *max = current->value;
			*coords_undefined = 0;
		}

		*min = MIN(current->value, *min);
		*max = MAX(current->value, *max);
	}

// Test all handles
	for(FloatAuto *current = (FloatAuto*)first; current; current = (FloatAuto*)NEXT)
	{
		if(current->position >= unit_start && current->position < unit_end)
		{
			if(*coords_undefined)
			{
				*min = *max = current->value;
				*coords_undefined = 0;
			}
			
			*min = MIN(current->value, *min);
			*min = MIN(current->value + current->control_in_value, *min);
			*min = MIN(current->value + current->control_out_value, *min);

			*max = MAX(current->value, *max);
			*max = MAX(current->value + current->control_in_value, *max);
			*max = MAX(current->value + current->control_out_value, *max);
		}
	}

// Test joining regions
	FloatAuto *prev = 0;
	FloatAuto *next = 0;
	int64_t unit_step = edl->local_session->zoom_sample;
	if(track->data_type == TRACK_VIDEO)
		unit_step = (int64_t)(unit_step * 
			edl->session->frame_rate / 
			edl->session->sample_rate);
	unit_step = MAX(unit_step, 1);
	for(int64_t position = unit_start; 
		position < unit_end; 
		position += unit_step)
	{
		float value = get_value(position,
			PLAY_FORWARD,
			prev,
			next);
		if(*coords_undefined)
		{
			*min = *max = value;
			*coords_undefined = 0;
		}
		else
		{
			*min = MIN(value, *min);
			*max = MAX(value, *max);
		}	
	}
}

void FloatAutos::dump()
{
	printf("	FloatAutos::dump %p\n", this);
	printf("	Default: position %lld value=%f\n", 
		default_auto->position, 
		((FloatAuto*)default_auto)->value);
	for(Auto* current = first; current; current = NEXT)
	{
		printf("	position %lld value=%f invalue=%f outvalue=%f inposition=%lld outposition=%lld\n", 
			current->position, 
			((FloatAuto*)current)->value,
			((FloatAuto*)current)->control_in_value,
			((FloatAuto*)current)->control_out_value,
			((FloatAuto*)current)->control_in_position,
			((FloatAuto*)current)->control_out_position);
	}
}
