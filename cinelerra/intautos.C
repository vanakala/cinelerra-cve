#include "intauto.h"
#include "intautos.h"

IntAutos::IntAutos(EDL *edl, Track *track)
 : Autos(edl, track)
{
}

IntAutos::~IntAutos()
{
}


Auto* IntAutos::new_auto()
{
	return new IntAuto(edl, this);
}

int IntAutos::automation_is_constant(long start, long end)
{
	Auto *current_auto, *before = 0, *after = 0;
	int result;

	result = 1;          // default to constant
	if(!last && !first) return result; // no automation at all

// quickly get autos just outside range	
	get_neighbors(start, end, &before, &after);

// autos before range
	if(before) 
		current_auto = before;   // try first auto
	else 
		current_auto = first;

// test autos in range	
	for( ; result && 
		current_auto && 
		current_auto->next && 
		current_auto->position < end; 
		current_auto = current_auto->next)
	{
// not constant
		if(((IntAuto*)current_auto->next)->value != ((IntAuto*)current_auto)->value) 
			result = 0;
	}

	return result;
}

double IntAutos::get_automation_constant(long start, long end)
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

	return ((IntAuto*)current_auto)->value;
}


void IntAutos::dump()
{
	printf("	Default %p: position: %ld value: %d\n", default_auto, default_auto->position, ((IntAuto*)default_auto)->value);
	for(Auto* current = first; current; current = NEXT)
	{
		printf("	%p position: %ld value: %d\n", current, current->position, ((IntAuto*)current)->value);
	}
}
