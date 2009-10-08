
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

#include "asset.h"
#include "file.h"
#include "errorbox.h"
#include "formatcheck.h"
#include "mwindow.inc"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

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
			errorbox.create_objects(_("The format you selected doesn't support video."));
			errorbox.run_window();
			result = 1;
		}
	}
	
	if(!result && asset->audio_data)
	{
		if(!File::supports_audio(asset->format))
		{
			ErrorBox errorbox(PROGRAM_NAME ": Error");
			errorbox.create_objects(_("The format you selected doesn't support audio."));
			errorbox.run_window();
			result = 1;
		}

		if(!result && asset->bits == BITSIMA4 && asset->format != FILE_MOV)
		{
			ErrorBox errorbox(PROGRAM_NAME ": Error");
			errorbox.create_objects(_("IMA4 compression is only available in Quicktime movies."));
			errorbox.run_window();
			result = 1;
		}

		if(!result && asset->bits == BITSULAW && 
			asset->format != FILE_MOV &&
			asset->format != FILE_PCM)
		{
			ErrorBox errorbox(PROGRAM_NAME ": Error");
			errorbox.create_objects(_("ULAW compression is only available in\n" 
				"Quicktime Movies and PCM files."));
			errorbox.run_window();
			result = 1;
		}
	}

	return result;
}
