#include "autos.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "filexml.h"
#include "track.h"
#include "transportque.inc"

#include <string.h>


Autos::Autos(EDL *edl, Track *track)
 : List<Auto>()
{
	this->edl = edl;
	this->track = track;
//printf("Autos::Autos 1 %p %p %p\n", this, first, last);
}



Autos::~Autos()
{
	while(last) delete last;
	delete default_auto;
}

int Autos::create_objects()
{
// Default
	default_auto = new_auto();
	default_auto->is_default = 1;
	return 0;
}

Auto* Autos::new_auto()
{
	return new Auto(edl, this);
}

void Autos::resample(double old_rate, double new_rate)
{
	for(Auto *current = first; current; current = NEXT)
	{
		current->position = (long)((double)current->position * 
			new_rate / 
			old_rate + 
			0.5);
	}
}

void Autos::equivalent_output(Autos *autos, long startproject, long *result)
{
	if(
// Default keyframe differs
		(!total() && !(*default_auto == *autos->default_auto))
		)
	{
		if(*result < 0 || *result > startproject) *result = startproject;
	}
	else
// Search for difference
	{
		for(Auto *current = first, *that_current = autos->first; 
			current || that_current; 
			current = NEXT,
			that_current = that_current->next)
		{
// Total differs
			if(current && !that_current)
			{
				long position1 = (autos->last ? autos->last->position : startproject);
				long position2 = current->position;
				if(*result < 0 || *result > MIN(position1, position2))
					*result = MIN(position1, position2);
				break;
			}
			else
			if(!current && that_current)
			{
				long position1 = (last ? last->position : startproject);
				long position2 = that_current->position;
				if(*result < 0 || *result > MIN(position1, position2))
					*result = MIN(position1, position2);
				break;
			}
			else
// Keyframes differ
			if(!(*current == *that_current) || 
				current->position != that_current->position)
			{
				long position1 = (current->previous ? 
					current->previous->position : 
					startproject);
				long position2 = (that_current->previous ? 
					that_current->previous->position : 
					startproject);
				if(*result < 0 || *result > MIN(position1, position2))
					*result = MIN(position1, position2);
				break;
			}
		}
	}
}

void Autos::copy_from(Autos *autos)
{
	Auto *current = autos->first, *this_current = first;

	default_auto->copy_from(autos->default_auto);

// Detect common memory leak bug
	if(autos->first && !autos->last)
	{
		printf("Autos::copy_from inconsistent pointers\n");
		exit(1);
	}

	for(current = autos->first; current; current = NEXT)
	{
//printf("Autos::copy_from 1 %p\n", current);
//sleep(1);
		if(!this_current)
		{
			append(this_current = new_auto());
		}
		this_current->copy_from(current);
		this_current = this_current->next;
	}

	for( ; this_current; )
	{
		Auto *next_current = this_current->next;
		delete this_current;
		this_current = next_current;
	}
}


// We don't replace it in pasting but
// when inserting the first EDL of a load operation we need to replace
// the default keyframe.
void Autos::insert_track(Autos *automation, 
	long start_unit, 
	long length_units,
	int replace_default)
{
// Insert silence
	insert(start_unit, start_unit + length_units);

	if(replace_default) default_auto->copy_from(automation->default_auto);
	for(Auto *current = automation->first; current; current = NEXT)
	{
		Auto *new_auto = insert_auto(start_unit + current->position);
		new_auto->copy_from(current);
// Override copy_from
		new_auto->position = current->position + start_unit;
	}
}

Auto* Autos::get_prev_auto(long position, int direction, Auto* &current)
{
// Get on or before position
	if(direction == PLAY_FORWARD)
	{
// Try existing result
		if(current)
		{
			while(current && current->position < position) current = NEXT;
			while(current && current->position > position) current = PREVIOUS;
		}

		if(!current)
		{
			for(current = last; 
				current && current->position > position; 
				current = PREVIOUS) ;
		}
		if(!current) current = (first ? first : default_auto);
	}
	else
// Get on or after position
	if(direction == PLAY_REVERSE)
	{
		if(current)
		{
			while(current && current->position > position) current = PREVIOUS;
			while(current && current->position < position) current = NEXT;
		}

		if(!current)
		{
			for(current = first; 
				current && current->position < position; 
				current = NEXT) ;
		}

		if(!current) current = (last ? last : default_auto);
	}

	return current;
}

Auto* Autos::get_prev_auto(int direction, Auto* &current)
{
	double position_double = edl->local_session->selectionstart;
	position_double = edl->align_to_frame(position_double, 0);
	long position = track->to_units(position_double, 0);

	return get_prev_auto(position, direction, current);

	return current;
}

int Autos::auto_exists_for_editing(double position)
{
	int result = 0;
	
	if(edl->session->auto_keyframes)
	{
		double unit_position = position;
		unit_position = edl->align_to_frame(unit_position, 0);
		unit_position = track->to_units(unit_position, 0);

		for(Auto *current = first; 
			current; 
			current = NEXT)
		{
			if(edl->equivalent(current->position, unit_position))
			{
				result = 1;
				break;
			}
		}
	}
	else
	{
		result = 1;
	}

	return result;
}

Auto* Autos::get_auto_for_editing(double position)
{
	if(position < 0)
	{
		position = edl->local_session->selectionstart;
	}

	Auto *result = 0;
	position = edl->align_to_frame(position, 0);




//printf("Autos::get_auto_for_editing %p %p\n", first, default_auto);

	if(edl->session->auto_keyframes)
	{
		result = insert_auto(track->to_units(position, 0));
	}
	else
		result = get_prev_auto(track->to_units(position, 0), 
			PLAY_FORWARD, 
			result);

//printf("Autos::get_auto_for_editing %p %p %p\n", default_auto, first, result);
	return result;
}


Auto* Autos::get_next_auto(long position, int direction, Auto* &current)
{
	if(direction == PLAY_FORWARD)
	{
		if(current)
		{
			while(current && current->position > position) current = PREVIOUS;
			while(current && current->position < position) current = NEXT;
		}

		if(!current)
		{
			for(current = first;
				current && current->position <= position;
				current = NEXT)
				;
		}

		if(!current) current = (last ? last : default_auto);
	}
	else
	if(direction == PLAY_REVERSE)
	{
		if(current)
		{
			while(current && current->position < position) current = NEXT;
			while(current && current->position > position) current = PREVIOUS;
		}

		if(!current)
		{
			for(current = last;
				current && current->position > position;
				current = PREVIOUS)
				;
		}

		if(!current) current = (first ? first : default_auto);
	}

	return current;
}

Auto* Autos::insert_auto(long position)
{
	Auto *current, *result;

// Test for existence
	for(current = first; 
		current && !edl->equivalent(current->position, position); 
		current = NEXT)
	{
		;
	}

//printf("Autos::insert_auto %p\n", current);
// Insert new
	if(!current)
	{
// Get first one on or before as a template
		for(current = last; 
			current && current->position > position; 
			current = PREVIOUS)
		{
			;
		}

		if(current)
		{
			insert_after(current, result = new_auto());
			result->copy_from(current);
		}
		else
		{
			current = first;
			if(!current) current = default_auto;

			insert_before(first, result = new_auto());
			if(current) result->copy_from(current);
		}

		result->position = position;
	}
	else
	{
		result = current;
	}

	return result;
}

int Autos::clear_all()
{
	Auto *current_, *current;
	
	for(current = first; current; current = current_)
	{
		current_ = NEXT;
		remove(current);
	}
	add_auto(0, default_);
	return 0;
}

int Autos::insert(long start, long end)
{
	long length;
	Auto *current = first;

	for( ; current && current->position < start; current = NEXT)
		;

	length = end - start;

	for(; current; current = NEXT)
	{
		current->position += length;
	}
	return 0;
}

void Autos::paste(long start, 
	long length, 
	double scale, 
	FileXML *file, 
	int default_only)
{
	int total = 0;
	int result = 0;

//printf("Autos::paste %ld\n", start);
	do{
		result = file->read_tag();

		if(!result)
		{
// End of list
			if(strstr(file->tag.get_title(), "AUTOS") && 
				file->tag.get_title()[0] == '/')
			{
				result = 1;
			}
			else
			if(!strcmp(file->tag.get_title(), "AUTO"))
			{
				Auto *current = 0;

// Paste first active auto into default				
				if(default_only)
				{
					if(total == 1)
					{
						current = default_auto;
					}
				}
				else
// Paste default auto into default
				if(total == 0)
					current = default_auto;
				else
				{
					long position = Units::to_long(
						(double)file->tag.get_property("POSITION", 0) *
							scale + 
							start);
// Paste active auto into track
					current = insert_auto(position);
				}

				if(current)
				{
					current->load(file);
				}
				total++;
			}
		}
	}while(!result);
	
}


int Autos::paste_silence(long start, long end)
{
	insert(start, end);
	return 0;
}

int Autos::copy(long start, 
	long end, 
	FileXML *file, 
	int default_only,
	int autos_only)
{
// First auto is always loaded into default even if it is discarded in a paste
// operation
	if(!autos_only)
	{
		default_auto->copy(0, 0, file, default_only);
	}

//printf("Autos::copy 1 %d %d %p\n", default_only, start, autoof(start));
	if(!default_only)
	{
		for(Auto* current = autoof(start); 
			current && current->position <= end; 
			current = NEXT)
		{
// Want to copy single keyframes by putting the cursor on them
			if(current->position >= start && current->position <= end)
			{
				current->copy(start, end, file, default_only);
			}
		}
	}
// Copy default auto again to make it the active auto on the clipboard
	else
	{
// Need to force position to 0 for the case of plugins
// and default status to 0.
		default_auto->copy(0, 0, file, default_only);
	}
//printf("Autos::copy 2\n");

	return 0;
}

// Remove 3 consecutive autos with the same value
// Remove autos which are out of order
void Autos::optimize()
{
	int done = 0;

	while(!done)
	{
		int consecutive = 0;
		done = 1;
		
		
		for(Auto *current = first; current; current = NEXT)
		{
// Get 3rd consecutive auto of equal value
			if(current != first)
			{
				if(*current == *PREVIOUS)
				{
					consecutive++;
					if(consecutive >= 3)
					{
						delete PREVIOUS;
						break;
					}
				}
				else
					consecutive = 0;
				
				if(done && current->position <= PREVIOUS->position)
				{
					delete current;
					break;
				}
			}
		}
	}
}


void Autos::remove_nonsequential(Auto *keyframe)
{
	if((keyframe->next && keyframe->next->position <= keyframe->position) ||
		(keyframe->previous && keyframe->previous->position >= keyframe->position))
	{
		delete keyframe;
	}
}




void Autos::clear(long start, 
	long end, 
	int shift_autos)
{
	long length;
	Auto *next, *current;
	length = end - start;


	current = autoof(start);

// If a range is selected don't delete the ending keyframe but do delete
// the beginning keyframe because shifting end handle forward shouldn't
// delete the first keyframe of the next edit.

	while(current && 
		((end != start && current->position < end) ||
		(end == start && current->position <= end)))
	{
		next = NEXT;
		remove(current);
		current = next;
	}

	while(current && shift_autos)
	{
		current->position -= length;
		current = NEXT;
	}
}

int Autos::clear_auto(long position)
{
	Auto *current;
	current = autoof(position);
	if(current->position == position) remove(current);
}


int Autos::load(FileXML *file)
{
	while(last)
		remove(last);    // remove any existing autos

	int result = 0, first_auto = 1;
	Auto *current;
	
	do{
		result = file->read_tag();
		
		if(!result)
		{
			if(strstr(file->tag.get_title(), "AUTOS") && file->tag.get_title()[0] == '/')
			{
				result = 1;
			}
			else
			if(!strcmp(file->tag.get_title(), "AUTO"))
			{
				if(first_auto)
				{
					default_auto->load(file);
					default_auto->position = 0;
					first_auto = 0;
				}
				else
				{
					current = append(new_auto());
					current->position = file->tag.get_property("POSITION", (long)0);
					current->load(file);
				}
			}
		}
	}while(!result);
	return 0;
}






int Autos::slope_adjustment(long ax, double slope)
{
	return (int)(ax * slope);
}


int Autos::scale_time(float rate_scale, int scale_edits, int scale_autos, long start, long end)
{
	Auto *current;
	
	for(current = first; current && scale_autos; current = NEXT)
	{
//		if(current->position >= start && current->position <= end)
//		{
			current->position = (long)((current->position - start) * rate_scale + start + 0.5);
//		}
	}
	return 0;
}

Auto* Autos::autoof(long position)
{
	Auto *current;

	for(current = first; 
		current && current->position < position; 
		current = NEXT)
	{ 
		;
	}
	return current;     // return 0 on failure
}

Auto* Autos::nearest_before(long position)
{
	Auto *current;

	for(current = last; current && current->position >= position; current = PREVIOUS)
	{ ; }


	return current;     // return 0 on failure
}

Auto* Autos::nearest_after(long position)
{
	Auto *current;

	for(current = first; current && current->position <= position; current = NEXT)
	{ ; }


	return current;     // return 0 on failure
}

int Autos::get_neighbors(long start, long end, Auto **before, Auto **after)
{
	if(*before == 0) *before = first;
	if(*after == 0) *after = last; 

	while(*before && (*before)->next && (*before)->next->position <= start)
		*before = (*before)->next;
	
	while(*after && (*after)->previous && (*after)->previous->position >= end)
		*after = (*after)->previous;

	while(*before && (*before)->position > start) *before = (*before)->previous;
	
	while(*after && (*after)->position < end) *after = (*after)->next;
	return 0;
}

int Autos::automation_is_constant(long start, long end)
{
	return 0;
}

double Autos::get_automation_constant(long start, long end)
{
	return 0;
}


int Autos::init_automation(long &buffer_position,
				long &input_start, 
				long &input_end, 
				int &automate, 
				double &constant, 
				long input_position,
				long buffer_len,
				Auto **before, 
				Auto **after,
				int reverse)
{
	buffer_position = 0;

// set start and end boundaries for automation info
	input_start = reverse ? input_position - buffer_len : input_position;
	input_end = reverse ? input_position : input_position + buffer_len;

// test automation for constant value
// and set up *before and *after
	if(automate)
	{
		if(automation_is_constant(input_start, input_end))
		{
			constant += get_automation_constant(input_start, input_end);
			automate = 0;
		}
	}
	return automate;
}


int Autos::init_slope(Auto **current_auto, 
				double &slope_start, 
				double &slope_value,
				double &slope_position, 
				long &input_start, 
				long &input_end, 
				Auto **before, 
				Auto **after,
				int reverse)
{
// apply automation
	*current_auto = reverse ? *after : *before;
// no auto before start so use first auto in range
// already know there is an auto since automation isn't constant
	if(!*current_auto)
	{
		*current_auto = reverse ? last : first;
//		slope_value = (*current_auto)->value;
		slope_start = input_start;
		slope_position = 0;
	}
	else
	{
// otherwise get the first slope point and advance auto
//		slope_value = (*current_auto)->value;
		slope_start = (*current_auto)->position;
		slope_position = reverse ? slope_start - input_end : input_start - slope_start;
		(*current_auto) = reverse ? (*current_auto)->previous : (*current_auto)->next;
	}
	return 0;
}


int Autos::get_slope(Auto **current_auto, 
				double &slope_start, 
				double &slope_end, 
				double &slope_value,
				double &slope, 
				long buffer_len, 
				long buffer_position,
				int reverse)
{
// get the slope
	if(*current_auto)
	{
		slope_end = reverse ? slope_start - (*current_auto)->position : (*current_auto)->position - slope_start;
		if(slope_end) 
//			slope = ((*current_auto)->value - slope_value) / slope_end;
//		else
			slope = 0;
	}
	else
	{
		slope = 0;
		slope_end = buffer_len - buffer_position;
	}
	return 0;
}

int Autos::advance_slope(Auto **current_auto, 
				double &slope_start, 
				double &slope_value,
				double &slope_position, 
				int reverse)
{
	if(*current_auto) 
	{
		slope_start = (*current_auto)->position;
//		slope_value = (*current_auto)->value;
		(*current_auto) = reverse ? (*current_auto)->previous : (*current_auto)->next;
		slope_position = 0;
	}
	return 0;
}

float Autos::value_to_percentage()
{
	return 0;
}

long Autos::get_length()
{
	if(last) 
		return last->position + 1;
	else
		return 0;
}










