// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PREFERENCES_INC
#define PREFERENCES_INC

// Run-time configuration directory
#define BCASTDIR "~/.cinelerra/"
#define BACKUP_PATH BCASTDIR "backup.xml"
#define DEAMON_PORT 27400

class Preferences;
class PlaybackConfig;
class AudioOutConfig;
class VideoOutConfig;
extern Preferences *preferences_global;
extern Preferences *render_preferences;
#endif
