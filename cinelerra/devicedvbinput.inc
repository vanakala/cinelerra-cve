
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

#ifndef DEVICEDVBINPUT_INC
#define DEVICEDVBINPUT_INC


class DeviceDVBInput;


#define NETTUNE_TIMEOUT 5000000
#define BUFFER_FRAGMENTS 24


/**
 * Commands sent to the tuner over the network.
 * The tuner server always sends the is_busy value when it is opened.
 **/

/**
 * Flush the socket if we lose sync.
 **/
#define NETTUNE_NONE 0
/**
 * Return signal strength
 **/
#define NETTUNE_SIGNAL 1
/**
 * Read buffer
 **/
#define NETTUNE_READ 2
/**
 * Open the tuner device and set the channel
 **/
#define NETTUNE_OPEN 3

/**
 * Cause the tuner to send a message indicating it is ready for another connection.
 **/
#define NETTUNE_CLOSE 4


/**
 * Set the physical channel to use
 **/
#define NETTUNE_SET_CHANNEL 5
/**
 * Set the pids to use
 **/
#define NETTUNE_SET_AUDIO_PID 6
#define NETTUNE_SET_VIDEO_PID 7

// Set channel table to use
#define NETTUNE_SET_TABLE 8

// Channel tables
#define NETTUNE_AIR 0
#define NETTUNE_CABLE 1

#endif
