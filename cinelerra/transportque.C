#include "clip.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "tracks.h"
#include "transportque.h"

TransportCommand::TransportCommand()
{
// In rendering we want a master EDL so settings don't get clobbered
// in the middle of a job.
	edl = new EDL;
	edl->create_objects();
	reset();
	command = 0;
	change_type = 0;
}

TransportCommand::~TransportCommand()
{
	delete edl;
}

void TransportCommand::reset()
{
	playbackstart = 0;
	start_position = 0;
	end_position = 0;
	infinite = 0;
	realtime = 0;
	resume = 0;
// Don't reset the change type for commands which don't perform the change
	if(command != STOP) change_type = 0;
	command = COMMAND_NONE;
}

EDL* TransportCommand::get_edl()
{
	return edl;
}

TransportCommand& TransportCommand::operator=(TransportCommand &command)
{
	this->command = command.command;
	this->change_type = command.change_type;
//printf("TransportCommand::operator= 1\n");
	*this->edl = *command.edl;
//printf("TransportCommand::operator= 2\n");
	this->start_position = command.start_position;
	this->end_position = command.end_position;
	this->playbackstart = command.playbackstart;
	this->realtime = command.realtime;
	this->resume = command.resume;
	return *this;
}

int TransportCommand::single_frame()
{
	return (command == SINGLE_FRAME_FWD ||
		command == SINGLE_FRAME_REWIND ||
		command == CURRENT_FRAME);
}


int TransportCommand::get_direction()
{
	switch(command)
	{
		case SINGLE_FRAME_FWD:
		case NORMAL_FWD:
		case FAST_FWD:
		case SLOW_FWD:
		case CURRENT_FRAME:
			return PLAY_FORWARD;
			break;

		case SINGLE_FRAME_REWIND:
		case NORMAL_REWIND:
		case FAST_REWIND:
		case SLOW_REWIND:
			return PLAY_REVERSE;
			break;

		default:
			return PLAY_FORWARD;
			break;
	}
}

float TransportCommand::get_speed()
{
	switch(command)
	{
		case SLOW_FWD:
		case SLOW_REWIND:
			return 0.5;
			break;
		
		case NORMAL_FWD:
		case NORMAL_REWIND:
		case SINGLE_FRAME_FWD:
		case SINGLE_FRAME_REWIND:
		case CURRENT_FRAME:
			return 1;
			break;
		
		case FAST_FWD:
		case FAST_REWIND:
			return 2;
			break;
	}
}

// Assume starting without pause
void TransportCommand::set_playback_range(EDL *edl)
{
	if(!edl) edl = this->edl;
	switch(command)
	{
		case SLOW_FWD:
		case FAST_FWD:
		case NORMAL_FWD:
			start_position = edl->local_session->selectionstart;
			if(EQUIV(edl->local_session->selectionend, edl->local_session->selectionstart))
				end_position = edl->tracks->total_playable_length();
			else
				end_position = edl->local_session->selectionend;
// this prevents a crash if start position is after the loop when playing forwards
 		    if (edl->local_session->loop_playback && start_position > edl->local_session->loop_end)  
 			{
				    start_position = edl->local_session->loop_start;
			}
			break;
		
		case SLOW_REWIND:
		case FAST_REWIND:
		case NORMAL_REWIND:
			end_position = edl->local_session->selectionend;
			if(EQUIV(edl->local_session->selectionend, edl->local_session->selectionstart))
				start_position = 0;
			else
				start_position = edl->local_session->selectionstart;
// this prevents a crash if start position is before the loop when playing backwards
			if (edl->local_session->loop_playback && start_position <= edl->local_session->loop_start)
			{
					start_position = edl->local_session->loop_end;
					end_position = edl->local_session->loop_end;
			}
			break;
		
		case CURRENT_FRAME:
		case SINGLE_FRAME_FWD:
			start_position = edl->local_session->selectionstart;
			end_position = start_position + 
				1.0 / 
				edl->session->frame_rate;
			break;
		
		case SINGLE_FRAME_REWIND:
			start_position = edl->local_session->selectionend;
			end_position = start_position - 
				1.0 / 
				edl->session->frame_rate;
			break;
	}

	switch(get_direction())
	{
		case PLAY_FORWARD:
			playbackstart = start_position;
			break;

		case PLAY_REVERSE:
			playbackstart = end_position;
			break;
	}
// printf("TransportCommand::set_playback_range %f %f\n", 
// start_position * edl->session->frame_rate, 
// end_position * edl->session->frame_rate);
}

void TransportCommand::adjust_playback_range()
{
	if(edl->local_session->in_point >= 0 ||
		edl->local_session->out_point >= 0)
	{
		if(edl->local_session->in_point >= 0)
			start_position = edl->local_session->in_point;
		else
			start_position = 0;

		if(edl->local_session->out_point >= 0)
			end_position = edl->local_session->out_point;
		else
			end_position = edl->tracks->total_playable_length();
	}
}


















TransportQue::TransportQue()
{
	input_lock = new Condition(1, "TransportQue::input_lock");
	output_lock = new Condition(0, "TransportQue::output_lock");
}

TransportQue::~TransportQue()
{
	delete input_lock;
	delete output_lock;
}

int TransportQue::send_command(int command, 
		int change_type, 
		EDL *new_edl, 
		int realtime,
		int resume)
{
	input_lock->lock("TransportQue::send_command 1");
	this->command.command = command;
// Mutually exclusive operation
	this->command.change_type |= change_type;
	this->command.realtime = realtime;
	this->command.resume = resume;

	if(new_edl)
	{
// Just change the EDL if the change requires it because renderengine
// structures won't point to the new EDL otherwise and because copying the
// EDL for every cursor movement is slow.
		if(change_type == CHANGE_EDL ||
			change_type == CHANGE_ALL)
		{
// Copy EDL
			*this->command.get_edl() = *new_edl;
		}
		else
		if(change_type == CHANGE_PARAMS)
		{
			this->command.get_edl()->synchronize_params(new_edl);
		}

// Set playback range
		this->command.set_playback_range(new_edl);
	}

//printf("TransportQue::send_command 3\n");
//printf("TransportQue::send_command 2 %p %d %d\n", new_edl, this->command.playbackstart);
	input_lock->unlock();
	output_lock->unlock();
//printf("TransportQue::send_command 4\n");
	return 0;
}

void TransportQue::update_change_type(int change_type)
{
	input_lock->lock("TransportQue::update_change_type");
	this->command.change_type |= change_type;
	input_lock->unlock();
}
