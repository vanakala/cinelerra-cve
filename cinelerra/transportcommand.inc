// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef TRANSPORTCOMMAND_INC
#define TRANSPORTCOMMAND_INC

// Directions
#define PLAY_FORWARD 0
#define PLAY_REVERSE 1

// Commands
#define COMMAND_NONE         0
#define SINGLE_FRAME_FWD     1
#define NORMAL_FWD           2
#define FAST_FWD             3
#define SINGLE_FRAME_REWIND  4
#define NORMAL_REWIND        5
#define FAST_REWIND          6
#define STOP                 7
#define SLOW_FWD             9
#define SLOW_REWIND          10
#define REWIND               11
#define GOTO_END             12
#define CURRENT_FRAME        13

// Level of change.  Changes have to be inclusive of all the lesser changes.
// Delete cache
#define CHANGE_ALL            0xf
// Delete render engines
#define CHANGE_EDL            0x2
// Search EDL and synchronize parameters.
#define CHANGE_PARAMS         0x1
#define CHANGE_NONE           0x0

#define CMDOPT_CMD            0x0f
#define CMDOPT_USEINOUT       0x80

class TransportCommand;

#endif
