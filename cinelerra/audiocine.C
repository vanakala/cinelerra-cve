#include "audiocine.h"



AudioCine::AudioCine(AudioDevice *device)
 : AudioLowLevel(device)
{
}


AudioCine::~AudioCine()
{
}


int AudioCine::open_input()
{
}

int AudioCine::open_output()
{
}

int AudioCine::write_buffer(char *buffer, int size)
{
}

int AudioCine::read_buffer(char *buffer, int size)
{
}

int AudioCine::close_all()
{
}

int64_t AudioCine::device_position()
{
}

int AudioCine::flush_device()
{
}

int AudioCine::interrupt_playback()
{
}





