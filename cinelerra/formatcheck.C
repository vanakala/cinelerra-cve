#include "assets.h"
#include "file.h"
#include "errorbox.h"
#include "formatcheck.h"
#include "mwindow.inc"

FormatCheck::FormatCheck(Asset *asset)
{
	this->asset = asset;
}

FormatCheck::~FormatCheck()
{
}

int FormatCheck::check_format()
{
	int result = 0;

	if(!result && asset->video_data)
	{
// Only 1 format can store video.
		if(!File::supports_video(asset->format))
		{
			ErrorBox errorbox(PROGRAM_NAME ": Error");
			errorbox.create_objects("The format you selected doesn't support video.");
			errorbox.run_window();
			result = 1;
		}
	}
	
	if(!result && asset->audio_data)
	{
		if(!File::supports_audio(asset->format))
		{
			ErrorBox errorbox(PROGRAM_NAME ": Error");
			errorbox.create_objects("The format you selected doesn't support audio.");
			errorbox.run_window();
			result = 1;
		}

		if(!result && asset->bits == BITSIMA4 && asset->format != FILE_MOV)
		{
			ErrorBox errorbox(PROGRAM_NAME ": Error");
			errorbox.create_objects("IMA4 compression is only available in Quicktime movies.");
			errorbox.run_window();
			result = 1;
		}

		if(!result && asset->bits == BITSULAW && 
			asset->format != FILE_MOV &&
			asset->format != FILE_PCM)
		{
			ErrorBox errorbox(PROGRAM_NAME ": Error");
			errorbox.create_objects("ULAW compression is only available in\n" 
				"Quicktime Movies and PCM files.");
			errorbox.run_window();
			result = 1;
		}
	}

	return result;
}
