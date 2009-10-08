
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

#ifndef PLUGINCOMMANDS_H
#define PLUGINCOMMANDS_H

#define OK                                 0
#define CANCEL                             1
#define CHECK_HEADER                       43
#define GET_AUDIO                          2
#define GET_MULTICHANNEL                   3
#define GET_REALTIME                       4
// Get the title given by the plugin
#define GET_TITLE                          5
// Get the title given by the module
#define GET_STRING                         84
#define GET_VIDEO                          6
#define GET_FILEIO                         40
#define LOAD_DEFAULTS                      8
#define SAVE_DEFAULTS                      9

// Non-realtime plugins
#define GET_PARAMETERS                     11
#define GET_AUDIO_PARAMETERS               41
#define GET_VIDEO_PARAMETERS               42
#define GET_SAMPLERATE                     12
#define GET_FRAMERATE                      34
#define GET_FRAMESIZE                      35
#define GET_SMP                            53
#define SET_INTERACTIVE                    13
#define SET_RANGE                          14
#define GET_BUFFERS                        15
#define GET_REALTIME_BUFFERS               36
#define START_PLUGIN                       16
#define COMPLETED                          17
#define EXIT_PLUGIN                        18
#define READ_SAMPLES                       19
#define WRITE_SAMPLES                      20
#define READ_FRAMES                        37
#define WRITE_FRAMES                       38

// Realtime plugins
#define SHOW_GUI                           22
#define HIDE_GUI                           23
#define START_GUI                          24
#define STOP_GUI                           25
#define SET_STRING                         26
#define SAVE_DATA                          27
#define LOAD_DATA                          28
#define START_REALTIME                     7
#define STOP_REALTIME                      31
#define PROCESS_REALTIME                   32
#define CONFIGURE_CHANGE                   33
#define GET_USE_FLOAT                      39
#define GET_USE_ALPHA                      46
#define GET_USE_INTERPOLATION              47
#define GET_ASPECT_RATIO                   51
#define GUI_ON                             48
#define GUI_OFF                            49
#define GET_GUI_STATUS                     50
#define RESTART_REALTIME                   52

// Commands for I/O plugins
#define INTERRUPT_PARAMETERS               86
#define SET_CPUS                           83
#define OPEN_FILE                          44
#define CLOSE_FILE                         45
#define FILE_GET_HAS_AUDIO                 54
#define FILE_GET_CHANNELS                  56
#define FILE_GET_RATE                      57
#define FILE_GET_BITS                      58
#define FILE_GET_BYTE_ORDER                59
#define FILE_GET_SIGNED                    60
#define FILE_GET_HEADER                    61
#define FILE_GET_HAS_VIDEO                 55
#define FILE_GET_LAYERS                    62
#define FILE_GET_FRAME_RATE                63
#define FILE_GET_WIDTH                     64
#define FILE_GET_HEIGHT                    65
#define FILE_GET_QUALITY                   66
#define FILE_GET_COMPRESSION               67
#define FILE_GET_ALENGTH                   68
#define FILE_GET_VLENGTH                   69
#define FILE_SEEK_END                      70
#define FILE_SEEK_START                    71
#define FILE_GET_VIDEO_POSITION            72
#define FILE_GET_AUDIO_POSITION            73
#define FILE_SET_VIDEO_POSITION            74
#define FILE_SET_AUDIO_POSITION            75
#define FILE_SET_CHANNEL                   76
#define FILE_SET_LAYER                     77
#define FILE_READ_SAMPLES                  78
#define FILE_READ_FRAME                    79
#define FILE_READ_FRAME_PTR                80
#define FILE_RAW_FRAME_POSSIBLE            81
#define FILE_STRATEGY_POSSIBLE             85
#define FILE_READ_RAW_FRAME                82
#define FILE_WRITE_SAMPLES                 88
#define FILE_WRITE_FRAME                   89

#endif
