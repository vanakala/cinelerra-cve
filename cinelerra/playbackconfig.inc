#ifndef PLAYBACKCONFIG_INC
#define PLAYBACKCONFIG_INC


// Playback strategies
#define PLAYBACK_LOCALHOST 0
#define PLAYBACK_MULTIHEAD 1
#define PLAYBACK_BLONDSYMPHONY 2
#define PLAYBACK_STRATEGIES 3

// OSS can feed more than 1 device handle
#define MAXDEVICES 8

class AudioOutConfig;
class VideoOutConfig;
class PlaybackConfig;

#endif
