
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef RECORD_INC
#define RECORD_INC

class RecordMenuItem;
class Record;

// Context for start_recording
// Skip start delay
#define CONTEXT_INTERACTIVE  0
// Include start delay
#define CONTEXT_BATCH        1
// Only 1 frame omitting start delay
#define CONTEXT_SINGLEFRAME  2

// States for record
#define RECORD_NOTHING 0
#define RECORD_INTRO 1
#define RECORD_CAPTURING 2

// Operations for the CAPTURING state
#define IS_DONE       0
#define IS_MONITORING 1
#define IS_RECORDING  2
#define IS_DUPLEXING  3
#define IS_PREVIEWING 4

// Record modes
#define RECORD_INFINITE 0
#define RECORD_TIMED 1
#define RECORD_LOOP 2
#define RECORD_SCENETOSCENE 3

// Record start time
#define RECORD_START 0
#define RECORD_DELAY 1





#define BATCH_DELAY 500000

#endif
