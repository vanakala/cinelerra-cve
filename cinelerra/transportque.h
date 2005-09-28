#ifndef TRANSPORTQUE_H
#define TRANSPORTQUE_H

#include "canvas.inc"
#include "condition.inc"
#include "edl.inc"
#include "preferences.inc"
#include "transportque.inc"

class TransportCommand
{
public:
	TransportCommand();
	~TransportCommand();

	void reset();
// Get the direction based on the command
	int get_direction();
	float get_speed();
	void copy_from(TransportCommand *command);
	TransportCommand& operator=(TransportCommand &command);
// Get the range to play back from the EDL
	void set_playback_range(EDL *edl = 0, int use_inout = 0);

// Adjust playback range with in/out points for rendering
	void adjust_playback_range();

	int single_frame();
	EDL* get_edl();
	void delete_edl();
	void new_edl();

	int command;
	int change_type;
// lowest numbered second in playback range
	double start_position;
// highest numbered second in playback range
	double end_position;
	int infinite;
// Position used when starting playback
	double playbackstart;
// Send output to device
	int realtime;
// Use persistant starting point
	int resume;

private:
// Copied to render engines
	EDL *edl;
};

class TransportQue
{
public:
	TransportQue();
	~TransportQue();

	int send_command(int command, 
// The change type is ORed to accumulate changes.
		int change_type, 
		EDL *new_edl,
		int realtime,
// Persistent starting point
		int resume = 0,
		int use_inout = 0);
	void update_change_type(int change_type);

	TransportCommand command;
	Condition *input_lock, *output_lock;
};

#endif
