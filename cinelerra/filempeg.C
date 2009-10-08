
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
#include "bcprogressbox.h"
#include "bcsignals.h"
#include "bitspopup.h"
#include "byteorder.h"
#include "clip.h"
#include "condition.h"
#include "edit.h"
#include "file.h"
#include "filempeg.h"
#include "filesystem.h"
#include "guicast.h"
#include "indexfile.h"
#include "interlacemodes.h"
#include "language.h"
#include "mainerror.h"
#include "mwindow.inc"
#include "preferences.h"
#include "vframe.h"
#include "videodevice.inc"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MPEG_YUV420 0
#define MPEG_YUV422 1


#define MJPEG_EXE PLUGIN_DIR "/mpeg2enc.plugin"







// M JPEG dependancies
static double frame_rate_codes[] = 
{
	0,
	24000.0/1001.0,
	24.0,
	25.0,
	30000.0/1001.0,
	30.0,
	50.0,
	60000.0/1001.0,
	60.0
};

static double aspect_ratio_codes[] =
{
	0,
	1.0,
	1.333,
	1.777,
	2.11
};








FileMPEG::FileMPEG(Asset *asset, File *file)
 : FileBase(asset, file)
{
	reset_parameters();
// May also be VMPEG or AMPEG if write status.
	if(asset->format == FILE_UNKNOWN) asset->format = FILE_MPEG;
	asset->byte_order = 0;
	next_frame_lock = new Condition(0, "FileMPEG::next_frame_lock");
	next_frame_done = new Condition(0, "FileMPEG::next_frame_done");
}

FileMPEG::~FileMPEG()
{
	close_file();
	delete next_frame_lock;
	delete next_frame_done;
}

void FileMPEG::get_parameters(BC_WindowBase *parent_window, 
	Asset *asset, 
	BC_WindowBase* &format_window,
	int audio_options,
	int video_options)
{
	if(audio_options && asset->format == FILE_AMPEG)
	{
		MPEGConfigAudio *window = new MPEGConfigAudio(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
	else
	if(video_options && asset->format == FILE_VMPEG)
	{
		MPEGConfigVideo *window = new MPEGConfigVideo(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}

int FileMPEG::check_sig(Asset *asset)
{
	return mpeg3_check_sig(asset->path);
}

void FileMPEG::get_info(Asset *asset, int64_t *bytes, int *stracks)
{
	mpeg3_t *fd;

	int error = 0;
	if((fd = mpeg3_open(asset->path, &error)))
	{
		*bytes = mpeg3_get_bytes(fd);
		*stracks = mpeg3_subtitle_tracks(fd);
		mpeg3_close(fd);
	}
	return;
}

int FileMPEG::reset_parameters_derived()
{
	wrote_header = 0;
	mjpeg_out = 0;
	mjpeg_eof = 0;
	mjpeg_error = 0;


	dvb_out = 0;


	fd = 0;
	video_out = 0;
	audio_out = 0;
	prev_track = 0;
	temp_frame = 0;
	toolame_temp = 0;
	toolame_allocation = 0;
	toolame_result = 0;
	lame_temp[0] = 0;
	lame_temp[1] = 0;
	lame_allocation = 0;
	lame_global = 0;
	lame_output = 0;
	lame_output_allocation = 0;
	lame_fd = 0;
	lame_started = 0;
}


// Just create the Quicktime objects since this routine is also called
// for reopening.
int FileMPEG::open_file(int rd, int wr)
{
SET_TRACE
	int result = 0;
	this->rd = rd;
	this->wr = wr;

	if(rd)
	{
		int error = 0;
		if(!(fd = mpeg3_open(asset->path, &error)))
		{
			char string[BCTEXTLEN];
			if(error == MPEG3_INVALID_TOC_VERSION)
			{
				eprintf("Couldn't open %s because it has an invalid table of contents version.\n"
					"Rebuild the table of contents with mpeg3toc.",
					asset->path);
			}
			else
			if(error == MPEG3_TOC_DATE_MISMATCH)
			{
				eprintf("Couldn't open %s because the table of contents date differs from the source date.\n"
					"Rebuild the table of contents with mpeg3toc.",
					asset->path);
			}
			result = 1;
		}
		else
		{
// Determine if the file needs a table of contents and create one if needed.
// If it has video it must be scanned since video has keyframes.
			if(mpeg3_total_vstreams(fd))
			{
				if(create_index()) return 1;
			}

			mpeg3_set_cpus(fd, file->cpus);

			asset->audio_data = mpeg3_has_audio(fd);
			if(asset->audio_data)
			{
				asset->channels = 0;
				for(int i = 0; i < mpeg3_total_astreams(fd); i++)
				{
					asset->channels += mpeg3_audio_channels(fd, i);
				}
				if(!asset->sample_rate)
					asset->sample_rate = mpeg3_sample_rate(fd, 0);
				asset->audio_length = mpeg3_audio_samples(fd, 0); 
			}

			asset->video_data = mpeg3_has_video(fd);
			if(asset->video_data)
			{
				asset->layers = mpeg3_total_vstreams(fd);
				asset->width = mpeg3_video_width(fd, 0);
				asset->height = mpeg3_video_height(fd, 0);
				asset->interlace_mode = BC_ILACE_MODE_UNDETECTED; // TODO: (to do this, start at hvirtualcvs/libmpeg3/headers.c 
				                                                  //        and find out how to decode info from the header)
				asset->video_length = mpeg3_video_frames(fd, 0);
				asset->vmpeg_cmodel = 
					(mpeg3_colormodel(fd, 0) == MPEG3_YUV422P) ? MPEG_YUV422 : MPEG_YUV420;
				if(!asset->frame_rate)
					asset->frame_rate = mpeg3_frame_rate(fd, 0);

// Enable subtitles
//printf("FileMPEG::open %d\n", file->playback_subtitle);
				if(file->playback_subtitle >= 0)
					mpeg3_show_subtitle(fd, file->playback_subtitle);
			}
		}
	}


	
	if(wr && asset->format == FILE_VMPEG)
	{
// Heroine Virtual encoder
		if(asset->vmpeg_cmodel == MPEG_YUV422)
		{
			char bitrate_string[BCTEXTLEN];
			char quant_string[BCTEXTLEN];
			char iframe_string[BCTEXTLEN];

			sprintf(bitrate_string, "%d", asset->vmpeg_bitrate);
			sprintf(quant_string, "%d", asset->vmpeg_quantization);
			sprintf(iframe_string, "%d", asset->vmpeg_iframe_distance);

// Construct command line
			if(!result)
			{
				append_vcommand_line("mpeg2enc");


				if(asset->aspect_ratio > 0)
				{
					append_vcommand_line("-a");
					if(EQUIV(asset->aspect_ratio, 1))
						append_vcommand_line("1");
					else
					if(EQUIV(asset->aspect_ratio, 1.333))
						append_vcommand_line("2");
					else
					if(EQUIV(asset->aspect_ratio, 1.777))
						append_vcommand_line("3");
					else
					if(EQUIV(asset->aspect_ratio, 2.11))
						append_vcommand_line("4");
				}

				append_vcommand_line(asset->vmpeg_derivative == 1 ? "-1" : "");
				append_vcommand_line(asset->vmpeg_cmodel == MPEG_YUV422 ? "-422" : "");
				if(asset->vmpeg_fix_bitrate)
				{
					append_vcommand_line("--cbr -b");
					append_vcommand_line(bitrate_string);
				}
				else
				{
					append_vcommand_line("-q");
					append_vcommand_line(quant_string);
				}
				append_vcommand_line(!asset->vmpeg_fix_bitrate ? quant_string : "");
				append_vcommand_line("-n");
				append_vcommand_line(iframe_string);
				append_vcommand_line(asset->vmpeg_progressive ? "-p" : "");
				append_vcommand_line(asset->vmpeg_denoise ? "-d" : "");
				append_vcommand_line(file->cpus <= 1 ? "-u" : "");
				append_vcommand_line(asset->vmpeg_seq_codes ? "-g" : "");
				append_vcommand_line(asset->path);

				video_out = new FileMPEGVideo(this);
				video_out->start();
			}
		}
		else
// mjpegtools encoder
		{
			char string[BCTEXTLEN];
			sprintf(mjpeg_command, MJPEG_EXE);

// Must disable interlacing if MPEG-1
			switch (asset->vmpeg_preset)
			{
				case 0: asset->vmpeg_progressive = 1; break;
				case 1: asset->vmpeg_progressive = 1; break;
				case 2: asset->vmpeg_progressive = 1; break;
			}



// The current usage of mpeg2enc requires bitrate of 0 when quantization is fixed and
// quantization of 1 when bitrate is fixed.  Perfectly intuitive.
			if(asset->vmpeg_fix_bitrate)
			{
				sprintf(string, " -b %d -q 1", asset->vmpeg_bitrate / 1000);
			}
			else
			{
				sprintf(string, " -b 0 -q %d", asset->vmpeg_quantization);
			}
			strcat(mjpeg_command, string);






// Aspect ratio
			int aspect_ratio_code = -1;
			if(asset->aspect_ratio > 0)
			{
				for(int i = 0; i < sizeof(aspect_ratio_codes) / sizeof(double); i++)
				{
					if(EQUIV(aspect_ratio_codes[i], asset->aspect_ratio))
					{
						aspect_ratio_code = i;
						break;
					}
				}
			}
			if(aspect_ratio_code < 0)
			{
				eprintf("Unsupported aspect ratio %f\n", asset->aspect_ratio);
				aspect_ratio_code = 2;
			}
			sprintf(string, " -a %d", aspect_ratio_code);
			strcat(mjpeg_command, string);






// Frame rate
			int frame_rate_code = -1;
    		for(int i = 1; sizeof(frame_rate_codes) / sizeof(double); ++i)
			{
				if(EQUIV(asset->frame_rate, frame_rate_codes[i]))
				{
					frame_rate_code = i;
					break;
				}
			}
			if(frame_rate_code < 0)
			{
				frame_rate_code = 4;
				eprintf("Unsupported frame rate %f\n", asset->frame_rate);
			}
			sprintf(string, " -F %d", frame_rate_code);
			strcat(mjpeg_command, string);





			strcat(mjpeg_command, 
				asset->vmpeg_progressive ? " -I 0" : " -I 1");
			


			sprintf(string, " -M %d", file->cpus);
			strcat(mjpeg_command, string);


			if(!asset->vmpeg_progressive)
			{
				strcat(mjpeg_command, asset->vmpeg_field_order ? " -z b" : " -z t");
			}


			sprintf(string, " -f %d", asset->vmpeg_preset);
			strcat(mjpeg_command, string);


			sprintf(string, " -g %d -G %d", asset->vmpeg_iframe_distance, asset->vmpeg_iframe_distance);
			strcat(mjpeg_command, string);


			if(asset->vmpeg_seq_codes) strcat(mjpeg_command, " -s");


			sprintf(string, " -R %d", CLAMP(asset->vmpeg_pframe_distance, 0, 2));
			strcat(mjpeg_command, string);

			sprintf(string, " -o '%s'", asset->path);
			strcat(mjpeg_command, string);



			eprintf("Running %s\n", mjpeg_command);
			if(!(mjpeg_out = popen(mjpeg_command, "w")))
			{
				eprintf("Error while opening \"%s\" for writing. \n%m\n", mjpeg_command);
			}

			video_out = new FileMPEGVideo(this);
			video_out->start();
		}
	}
	else
	if(wr && asset->format == FILE_AMPEG)
	{
		char command_line[BCTEXTLEN];
		char encoder_string[BCTEXTLEN];
		char argument_string[BCTEXTLEN];

//printf("FileMPEG::open_file 1 %d\n", asset->ampeg_derivative);
		encoder_string[0] = 0;

		if(asset->ampeg_derivative == 2)
		{
			char string[BCTEXTLEN];
			append_acommand_line("toolame");
			append_acommand_line("-m");
			append_acommand_line((asset->channels >= 2) ? "j" : "m");
			sprintf(string, "%f", (float)asset->sample_rate / 1000);
			append_acommand_line("-s");
			append_acommand_line(string);
			sprintf(string, "%d", asset->ampeg_bitrate);
			append_acommand_line("-b");
			append_acommand_line(string);
			append_acommand_line("-");
			append_acommand_line(asset->path);

			audio_out = new FileMPEGAudio(this);
			audio_out->start();
		}
		else
		if(asset->ampeg_derivative == 3)
		{
			lame_global = lame_init();
			lame_set_brate(lame_global, asset->ampeg_bitrate);
			lame_set_quality(lame_global, 0);
			lame_set_in_samplerate(lame_global, 
				asset->sample_rate);
			lame_set_num_channels(lame_global,
				asset->channels);
			if((result = lame_init_params(lame_global)) < 0)
			{
				eprintf(_("encode: lame_init_params returned %d\n"), result);
				lame_close(lame_global);
				lame_global = 0;
			}
			else
			if(!(lame_fd = fopen(asset->path, "w")))
			{
				eprintf("Error while opening \"%s\" for writing. \n%m\n", asset->path);
				lame_close(lame_global);
				lame_global = 0;
				result = 1;
			}
		}
		else
		{
			eprintf("ampeg_derivative=%d\n", asset->ampeg_derivative);
			result = 1;
		}
	}
	else
// Transport stream for DVB capture
	if(wr)
	{
		if(!(dvb_out = fopen(asset->path, "w")))
		{
			eprintf("Error while opening \"%s\" for writing. \n%m\n", asset->path);
			result = 1;
		}
		
	}


//asset->dump();
SET_TRACE
	return result;
}








int FileMPEG::create_index()
{
// Calculate TOC path
	char index_filename[BCTEXTLEN];
	char source_filename[BCTEXTLEN];
	IndexFile::get_index_filename(source_filename, 
		file->preferences->index_directory, 
		index_filename, 
		asset->path);
	char *ptr = strrchr(index_filename, '.');
	int error = 0;

	if(!ptr) return 1;

// File is a table of contents.
	if(fd && mpeg3_has_toc(fd)) return 0;

	sprintf(ptr, ".toc");

	mpeg3_t *fd_toc;


// Test existing copy of TOC
	mpeg3_close(fd);     // Always free old fd
	if((fd_toc = mpeg3_open(index_filename, &error)))
	{
// Just exchange old fd 
		fd = fd_toc;
	} else
	{
// Create progress window.
// This gets around the fact that MWindowGUI is locked.
		char progress_title[BCTEXTLEN];
		char string[BCTEXTLEN];
		sprintf(progress_title, "Creating %s\n", index_filename);
		int64_t total_bytes;
		mpeg3_t *index_file = mpeg3_start_toc(asset->path, index_filename, &total_bytes);
		struct timeval new_time;
		struct timeval prev_time;
		struct timeval start_time;
		struct timeval current_time;
		gettimeofday(&prev_time, 0);
		gettimeofday(&start_time, 0);

		BC_ProgressBox *progress = new BC_ProgressBox(-1, 
			-1, 
			progress_title, 
			total_bytes);
		progress->start();
		int result = 0;
		while(1)
		{
			int64_t bytes_processed;
			mpeg3_do_toc(index_file, &bytes_processed);
			gettimeofday(&new_time, 0);

			if(new_time.tv_sec - prev_time.tv_sec >= 1)
			{
				gettimeofday(&current_time, 0);
				int64_t elapsed_seconds = current_time.tv_sec - start_time.tv_sec;
				int64_t total_seconds = elapsed_seconds * total_bytes / bytes_processed;
				int64_t eta = total_seconds - elapsed_seconds;
				progress->update(bytes_processed, 1);
				sprintf(string, 
					"%sETA: %lldm%llds",
					progress_title,
					eta / 60,
					eta % 60);
				progress->update_title(string, 1);
// 				fprintf(stderr, "ETA: %dm%ds        \r", 
// 					bytes_processed * 100 / total_bytes,
// 					eta / 60,
// 					eta % 60);
// 				fflush(stdout);
				prev_time = new_time;
			}
			if(bytes_processed >= total_bytes) break;
			if(progress->is_cancelled()) 
			{
				result = 1;
				break;
			}
		}

		mpeg3_stop_toc(index_file);

		progress->stop_progress();
		delete progress;

// Remove if error
		if(result)
		{
			remove(index_filename);
			return 1;
		}

// Reopen file from index path instead of asset path.
		if(!(fd = mpeg3_open(index_filename, &error)))
			return 1;
	}

	return 0;
}






void FileMPEG::append_vcommand_line(const char *string)
{
	if(string[0])
	{
		char *argv = strdup(string);
		vcommand_line.append(argv);
	}
}

void FileMPEG::append_acommand_line(const char *string)
{
	if(string[0])
	{
		char *argv = strdup(string);
		acommand_line.append(argv);
	}
}


int FileMPEG::close_file()
{
	mjpeg_eof = 1;
	next_frame_lock->unlock();

	if(fd)
	{
		mpeg3_close(fd);
	}

	if(video_out)
	{
// End of sequence signal
		if(file->asset->vmpeg_cmodel == MPEG_YUV422)
		{
			mpeg2enc_set_input_buffers(1, 0, 0, 0);
		}
		delete video_out;
		video_out = 0;
	}

	vcommand_line.remove_all_objects();
	acommand_line.remove_all_objects();

	if(audio_out)
	{
		toolame_send_buffer(0, 0);
		delete audio_out;
		audio_out = 0;
	}

	if(lame_global)
		lame_close(lame_global);

	if(temp_frame) delete temp_frame;
	if(toolame_temp) delete [] toolame_temp;

	if(lame_temp[0]) delete [] lame_temp[0];
	if(lame_temp[1]) delete [] lame_temp[1];
	if(lame_output) delete [] lame_output;
	if(lame_fd) fclose(lame_fd);

	if(mjpeg_out) fclose(mjpeg_out);


	if(dvb_out)
		fclose(dvb_out);

	reset_parameters();

	FileBase::close_file();
	return 0;
}

int FileMPEG::get_best_colormodel(Asset *asset, int driver)
{
//printf("FileMPEG::get_best_colormodel 1\n");
	switch(driver)
	{
		case PLAYBACK_X11:
			return BC_RGB888;
			if(asset->vmpeg_cmodel == MPEG_YUV420) return BC_YUV420P;
			if(asset->vmpeg_cmodel == MPEG_YUV422) return BC_YUV422P;
			break;
		case PLAYBACK_X11_XV:
		case PLAYBACK_ASYNCHRONOUS:
			if(asset->vmpeg_cmodel == MPEG_YUV420) return BC_YUV420P;
			if(asset->vmpeg_cmodel == MPEG_YUV422) return BC_YUV422P;
			break;
		case PLAYBACK_X11_GL:
			return BC_YUV888;
			break;
		case PLAYBACK_LML:
		case PLAYBACK_BUZ:
			return BC_YUV422P;
			break;
		case PLAYBACK_DV1394:
		case PLAYBACK_FIREWIRE:
			return BC_YUV422P;
			break;
		case VIDEO4LINUX:
		case VIDEO4LINUX2:
			if(asset->vmpeg_cmodel == MPEG_YUV420) return BC_YUV420P;
			if(asset->vmpeg_cmodel == MPEG_YUV422) return BC_YUV422P;
			break;
		case CAPTURE_BUZ:
		case CAPTURE_LML:
			return BC_YUV422;
			break;
		case CAPTURE_FIREWIRE:
		case CAPTURE_IEC61883:
			return BC_YUV422P;
			break;
	}
//printf("FileMPEG::get_best_colormodel 100\n");
}

int FileMPEG::colormodel_supported(int colormodel)
{
	return colormodel;
}

int FileMPEG::get_index(char *index_path)
{
	if(!fd) return 1;


// Convert the index tables from tracks to channels.
	if(mpeg3_index_tracks(fd))
	{
// Calculate size of buffer needed for all channels
		int buffer_size = 0;
		for(int i = 0; i < mpeg3_index_tracks(fd); i++)
		{
			buffer_size += mpeg3_index_size(fd, i) *
				mpeg3_index_channels(fd, i) *
				2;
		}

		asset->index_buffer = new float[buffer_size];

// Size of index buffer in floats
		int current_offset = 0;
// Current asset channel
		int current_channel = 0;
		asset->index_zoom = mpeg3_index_zoom(fd);
		asset->index_offsets = new int64_t[asset->channels];
		asset->index_sizes = new int64_t[asset->channels];
		for(int i = 0; i < mpeg3_index_tracks(fd); i++)
		{
			for(int j = 0; j < mpeg3_index_channels(fd, i); j++)
			{
				asset->index_offsets[current_channel] = current_offset;
				asset->index_sizes[current_channel] = mpeg3_index_size(fd, i) * 2;
				memcpy(asset->index_buffer + current_offset,
					mpeg3_index_data(fd, i, j),
					mpeg3_index_size(fd, i) * sizeof(float) * 2);

				current_offset += mpeg3_index_size(fd, i) * 2;
				current_channel++;
			}
		}

		FileSystem fs;
		asset->index_bytes = fs.get_size(asset->path);

		asset->write_index(index_path, buffer_size * sizeof(float));
		delete [] asset->index_buffer;

		return 0;
	}

	return 1;
}


int FileMPEG::can_copy_from(Edit *edit, int64_t position)
{
	if(!fd) return 0;
	return 0;
}

int FileMPEG::set_audio_position(int64_t sample)
{
#if 0
	if(!fd) return 1;
	
	int channel, stream;
	to_streamchannel(file->current_channel, stream, channel);

//printf("FileMPEG::set_audio_position %d %d %d\n", sample, mpeg3_get_sample(fd, stream), last_sample);
	if(sample != mpeg3_get_sample(fd, stream) &&
		sample != last_sample)
	{
		if(sample >= 0 && sample < asset->audio_length)
		{
//printf("FileMPEG::set_audio_position seeking stream %d\n", sample);
			return mpeg3_set_sample(fd, sample, stream);
		}
		else
			return 1;
	}
#endif
	return 0;
}

int FileMPEG::set_video_position(int64_t x)
{
	if(!fd) return 1;
	if(x >= 0 && x < asset->video_length)
	{
//printf("FileMPEG::set_video_position 1 %lld\n", x);
		mpeg3_set_frame(fd, x, file->current_layer);
	}
	else
		return 1;
}

int64_t FileMPEG::get_memory_usage()
{
	if(rd && fd)
	{
		int64_t result = mpeg3_memory_usage(fd);
		return result;
	}
	return 0;
}


int FileMPEG::write_samples(double **buffer, int64_t len)
{
	int result = 0;

//printf("FileMPEG::write_samples 1\n");
	if(asset->ampeg_derivative == 2)
	{
// Convert to int16
		int channels = MIN(asset->channels, 2);
		int64_t audio_size = len * channels * 2;
		if(toolame_allocation < audio_size)
		{
			if(toolame_temp) delete [] toolame_temp;
			toolame_temp = new unsigned char[audio_size];
			toolame_allocation = audio_size;
		}

		for(int i = 0; i < channels; i++)
		{
			int16_t *output = ((int16_t*)toolame_temp) + i;
			double *input = buffer[i];
			for(int j = 0; j < len; j++)
			{
				int sample = (int)(*input * 0x7fff);
				*output = (int16_t)(CLIP(sample, -0x8000, 0x7fff));
				output += channels;
				input++;
			}
		}
		result = toolame_send_buffer((char*)toolame_temp, audio_size);
	}
	else
	if(asset->ampeg_derivative == 3)
	{
		int channels = MIN(asset->channels, 2);
		int64_t audio_size = len * channels;
		if(!lame_global) return 1;
		if(!lame_fd) return 1;
		if(lame_allocation < audio_size)
		{
			if(lame_temp[0]) delete [] lame_temp[0];
			if(lame_temp[1]) delete [] lame_temp[1];
			lame_temp[0] = new float[audio_size];
			lame_temp[1] = new float[audio_size];
			lame_allocation = audio_size;
		}

		if(lame_output_allocation < audio_size * 4)
		{
			if(lame_output) delete [] lame_output;
			lame_output_allocation = audio_size * 4;
			lame_output = new char[lame_output_allocation];
		}

		for(int i = 0; i < channels; i++)
		{
			float *output = lame_temp[i];
			double *input = buffer[i];
			for(int j = 0; j < len; j++)
			{
				*output++ = *input++ * (float)32768;
			}
		}

		result = lame_encode_buffer_float(lame_global,
			lame_temp[0],
			(channels > 1) ? lame_temp[1] : lame_temp[0],
			len,
			(unsigned char*)lame_output,
			lame_output_allocation);
		if(result > 0)
		{
			char *real_output = lame_output;
			int bytes = result;
			if(!lame_started)
			{
				for(int i = 0; i < bytes; i++)
					if(lame_output[i])
					{
						real_output = &lame_output[i];
						lame_started = 1;
						bytes -= i;
						break;
					}
			}
			if(bytes > 0 && lame_started)
			{
				result = !fwrite(real_output, 1, bytes, lame_fd);
				if(result)
					eprintf("Error while writing samples");
			}
			else
				result = 0;
		}
		else
			result = 1;
	}

	return result;
}

int FileMPEG::write_frames(VFrame ***frames, int len)
{
	int result = 0;

	if(video_out)
	{
		int temp_w = (int)((asset->width + 15) / 16) * 16;
		int temp_h;


		int output_cmodel = 
			(asset->vmpeg_cmodel == MPEG_YUV420) ? BC_YUV420P : BC_YUV422P;
		
		
// Height depends on progressiveness
		if(asset->vmpeg_progressive || asset->vmpeg_derivative == 1)
			temp_h = (int)((asset->height + 15) / 16) * 16;
		else
			temp_h = (int)((asset->height + 31) / 32) * 32;

//printf("FileMPEG::write_frames 1\n");
		
// Only 1 layer is supported in MPEG output
		for(int i = 0; i < 1; i++)
		{
			for(int j = 0; j < len && !result; j++)
			{
				VFrame *frame = frames[i][j];
				
				
				
				if(asset->vmpeg_cmodel == MPEG_YUV422)
				{
					if(frame->get_w() == temp_w &&
						frame->get_h() == temp_h &&
						frame->get_color_model() == output_cmodel)
					{
						mpeg2enc_set_input_buffers(0, 
							(char*)frame->get_y(),
							(char*)frame->get_u(),
							(char*)frame->get_v());
					}
					else
					{
						if(temp_frame &&
							(temp_frame->get_w() != temp_w ||
							temp_frame->get_h() != temp_h ||
							temp_frame->get_color_model() || output_cmodel))
						{
							delete temp_frame;
							temp_frame = 0;
						}


						if(!temp_frame)
						{
							temp_frame = new VFrame(0, 
								temp_w, 
								temp_h, 
								output_cmodel);
						}

						cmodel_transfer(temp_frame->get_rows(), 
							frame->get_rows(),
							temp_frame->get_y(),
							temp_frame->get_u(),
							temp_frame->get_v(),
							frame->get_y(),
							frame->get_u(),
							frame->get_v(),
							0,
							0,
							asset->width,
							asset->height,
							0,
							0,
							asset->width,
							asset->height,
							frame->get_color_model(), 
							temp_frame->get_color_model(),
							0, 
							frame->get_w(),
							temp_w);

						mpeg2enc_set_input_buffers(0, 
							(char*)temp_frame->get_y(),
							(char*)temp_frame->get_u(),
							(char*)temp_frame->get_v());
					}
				}
				else
				{
// MJPEG uses the same dimensions as the input
					if(frame->get_color_model() == output_cmodel)
					{
						mjpeg_y = frame->get_y();
						mjpeg_u = frame->get_u();
						mjpeg_v = frame->get_v();
					}
					else
					{
						if(!temp_frame)
						{
							temp_frame = new VFrame(0, 
								asset->width, 
								asset->height, 
								output_cmodel);
						}

						cmodel_transfer(temp_frame->get_rows(), 
							frame->get_rows(),
							temp_frame->get_y(),
							temp_frame->get_u(),
							temp_frame->get_v(),
							frame->get_y(),
							frame->get_u(),
							frame->get_v(),
							0,
							0,
							asset->width,
							asset->height,
							0,
							0,
							asset->width,
							asset->height,
							frame->get_color_model(), 
							temp_frame->get_color_model(),
							0, 
							frame->get_w(),
							temp_w);

						mjpeg_y = temp_frame->get_y();
						mjpeg_u = temp_frame->get_u();
						mjpeg_v = temp_frame->get_v();
					}




					next_frame_lock->unlock();
					next_frame_done->lock("FileMPEG::write_frames");
					if(mjpeg_error) result = 1;
				}





			}
		}
	}



	return result;
}

int FileMPEG::read_frame(VFrame *frame)
{
	if(!fd) return 1;
	int result = 0;
	int src_cmodel;

// printf("FileMPEG::read_frame\n");
// frame->dump_stacks();
// frame->dump_params();

	if(mpeg3_colormodel(fd, 0) == MPEG3_YUV420P)
		src_cmodel = BC_YUV420P;
	else
	if(mpeg3_colormodel(fd, 0) == MPEG3_YUV422P)
		src_cmodel = BC_YUV422P;

	switch(frame->get_color_model())
	{
		case MPEG3_RGB565:
		case MPEG3_BGR888:
		case MPEG3_BGRA8888:
		case MPEG3_RGB888:
		case MPEG3_RGBA8888:
		case MPEG3_RGBA16161616:
SET_TRACE
			mpeg3_read_frame(fd, 
					frame->get_rows(), /* Array of pointers to the start of each output row */
					0,                    /* Location in input frame to take picture */
					0, 
					asset->width, 
					asset->height, 
					asset->width,                   /* Dimensions of output_rows */
					asset->height, 
					frame->get_color_model(),             /* One of the color model #defines */
					file->current_layer);
SET_TRACE
			break;

// Use Temp
		default:
// Read these directly
			if(frame->get_color_model() == src_cmodel)
			{
SET_TRACE
				mpeg3_read_yuvframe(fd,
					(char*)frame->get_y(),
					(char*)frame->get_u(),
					(char*)frame->get_v(),
					0,
					0,
					asset->width,
					asset->height,
					file->current_layer);
SET_TRACE
			}
			else
// Process through temp frame
			{
				char *y, *u, *v;
SET_TRACE
				mpeg3_read_yuvframe_ptr(fd,
					&y,
					&u,
					&v,
					file->current_layer);
SET_TRACE
				if(y && u && v)
				{
					cmodel_transfer(frame->get_rows(), 
						0,
						frame->get_y(),
						frame->get_u(),
						frame->get_v(),
						(unsigned char*)y,
						(unsigned char*)u,
						(unsigned char*)v,
						0,
						0,
						asset->width,
						asset->height,
						0,
						0,
						asset->width,
						asset->height,
						src_cmodel, 
						frame->get_color_model(),
						0, 
						asset->width,
						frame->get_w());
				}
			}
			break;
	}

SET_TRACE
	return result;
}


void FileMPEG::to_streamchannel(int channel, int &stream_out, int &channel_out)
{
	for(stream_out = 0, channel_out = file->current_channel; 
		stream_out < mpeg3_total_astreams(fd) && 
			channel_out >= mpeg3_audio_channels(fd, stream_out);
		channel_out -= mpeg3_audio_channels(fd, stream_out), stream_out++)
	;
}

int FileMPEG::read_samples(double *buffer, int64_t len)
{
	if(!fd) return 0;
	if(len < 0) return 0;

// This is directed to a FileMPEGBuffer
	float *temp_float = new float[len];
// Translate pure channel to a stream and a channel in the mpeg stream
	int stream, channel;
	to_streamchannel(file->current_channel, stream, channel);
	
	
	
//printf("FileMPEG::read_samples 1 current_sample=%ld len=%ld channel=%d\n", file->current_sample, len, channel);

	mpeg3_set_sample(fd, 
		file->current_sample,
		stream);
	mpeg3_read_audio(fd, 
		temp_float,      /* Pointer to pre-allocated buffer of floats */
		0,      /* Pointer to pre-allocated buffer of int16's */
		channel,          /* Channel to decode */
		len,         /* Number of samples to decode */
		stream);          /* Stream containing the channel */


//	last_sample = file->current_sample;
	for(int i = 0; i < len; i++) buffer[i] = temp_float[i];

	delete [] temp_float;
	return 0;
}

int FileMPEG::prefer_samples_float()
{
	return 1;
}

int FileMPEG::read_samples_float(float *buffer, int64_t len)
{
	if(!fd) return 0;

// Translate pure channel to a stream and a channel in the mpeg stream
	int stream, channel;
	to_streamchannel(file->current_channel, stream, channel);
	
	
//printf("FileMPEG::read_samples 1 current_sample=%ld len=%ld channel=%d\n", file->current_sample, len, channel);

	mpeg3_set_sample(fd, 
		file->current_sample,
		stream);
	mpeg3_read_audio(fd, 
		buffer,      	/* Pointer to pre-allocated buffer of floats */
		0,		/* Pointer to pre-allocated buffer of int16's */
		channel,          /* Channel to decode */
		len,         /* Number of samples to decode */
		stream);          /* Stream containing the channel */


//	last_sample = file->current_sample;

//printf("FileMPEG::read_samples 100\n");
	return 0;
}



char* FileMPEG::strtocompression(char *string)
{
	return "";
}

char* FileMPEG::compressiontostr(char *string)
{
	return "";
}







FileMPEGVideo::FileMPEGVideo(FileMPEG *file)
 : Thread(1, 0, 0)
{
	this->file = file;
	
	
	if(file->asset->vmpeg_cmodel == MPEG_YUV422)
	{
		mpeg2enc_init_buffers();
		mpeg2enc_set_w(file->asset->width);
		mpeg2enc_set_h(file->asset->height);
		mpeg2enc_set_rate(file->asset->frame_rate);
	}
}

FileMPEGVideo::~FileMPEGVideo()
{
	Thread::join();
}

void FileMPEGVideo::run()
{
	if(file->asset->vmpeg_cmodel == MPEG_YUV422)
	{
		printf("FileMPEGVideo::run ");
		for(int i = 0; i < file->vcommand_line.total; i++)
		printf("%s ", file->vcommand_line.values[i]);
		printf("\n");
		mpeg2enc(file->vcommand_line.total, file->vcommand_line.values);
	}
	else
	{
		while(1)
		{
			file->next_frame_lock->lock("FileMPEGVideo::run");
			if(file->mjpeg_eof) 
			{
				file->next_frame_done->unlock();
				break;
			}



// YUV4 sequence header
			if(!file->wrote_header)
			{
				file->wrote_header = 1;

				char string[BCTEXTLEN];
				char interlace_string[BCTEXTLEN];
				if(!file->asset->vmpeg_progressive)
				{
					sprintf(interlace_string, file->asset->vmpeg_field_order ? "b" : "t");
				}
				else
				{
					sprintf(interlace_string, "p");
				}

				fprintf(file->mjpeg_out, "YUV4MPEG2 W%d H%d F%d:%d I%s A%d:%d C%s\n",
					file->asset->width,
					file->asset->height,
					(int)(file->asset->frame_rate * 1001),
					1001,
					interlace_string,
					(int)(file->asset->aspect_ratio * 1000),
					1000,
					"420mpeg2");
			}

// YUV4 frame header
			fprintf(file->mjpeg_out, "FRAME\n");

// YUV data
			if(!fwrite(file->mjpeg_y, file->asset->width * file->asset->height, 1, file->mjpeg_out))
				file->mjpeg_error = 1;
			if(!fwrite(file->mjpeg_u, file->asset->width * file->asset->height / 4, 1, file->mjpeg_out))
				file->mjpeg_error = 1;
			if(!fwrite(file->mjpeg_v, file->asset->width * file->asset->height / 4, 1, file->mjpeg_out))
				file->mjpeg_error = 1;
			fflush(file->mjpeg_out);

			file->next_frame_done->unlock();
		}
		pclose(file->mjpeg_out);
		file->mjpeg_out = 0;
	}
}




















FileMPEGAudio::FileMPEGAudio(FileMPEG *file)
 : Thread(1, 0, 0)
{
	this->file = file;
	toolame_init_buffers();
}

FileMPEGAudio::~FileMPEGAudio()
{
	Thread::join();
}

void FileMPEGAudio::run()
{
	file->toolame_result = toolame(file->acommand_line.total, file->acommand_line.values);
}








MPEGConfigAudio::MPEGConfigAudio(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Audio Compression",
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	310,
	120,
	-1,
	-1,
	0,
	0,
	1)
{
	this->parent_window = parent_window;
	this->asset = asset;
}

MPEGConfigAudio::~MPEGConfigAudio()
{
}

int MPEGConfigAudio::create_objects()
{
	int x = 10, y = 10;
	int x1 = 150;
	MPEGLayer *layer;


	if(asset->format == FILE_MPEG)
	{
		add_subwindow(new BC_Title(x, y, _("No options for MPEG transport stream.")));
		return 0;
	}


	add_tool(new BC_Title(x, y, _("Layer:")));
	add_tool(layer = new MPEGLayer(x1, y, this));
	layer->create_objects();

	y += 30;
	add_tool(new BC_Title(x, y, _("Kbits per second:")));
	add_tool(bitrate = new MPEGABitrate(x1, y, this));
	bitrate->create_objects();
	
	
	add_subwindow(new BC_OKButton(this));
	show_window();
	flush();
	return 0;
}

int MPEGConfigAudio::close_event()
{
	set_done(0);
	return 1;
}







MPEGLayer::MPEGLayer(int x, int y, MPEGConfigAudio *gui)
 : BC_PopupMenu(x, y, 100, layer_to_string(gui->asset->ampeg_derivative))
{
	this->gui = gui;
}

void MPEGLayer::create_objects()
{
	add_item(new BC_MenuItem(layer_to_string(2)));
	add_item(new BC_MenuItem(layer_to_string(3)));
}

int MPEGLayer::handle_event()
{
	gui->asset->ampeg_derivative = string_to_layer(get_text());
	gui->bitrate->set_layer(gui->asset->ampeg_derivative);
	return 1;
};

int MPEGLayer::string_to_layer(char *string)
{
	if(!strcasecmp(layer_to_string(2), string))
		return 2;
	if(!strcasecmp(layer_to_string(3), string))
		return 3;

	return 2;
}

char* MPEGLayer::layer_to_string(int layer)
{
	switch(layer)
	{
		case 2:
			return _("II");
			break;
		
		case 3:
			return _("III");
			break;
			
		default:
			return _("II");
			break;
	}
}







MPEGABitrate::MPEGABitrate(int x, int y, MPEGConfigAudio *gui)
 : BC_PopupMenu(x, 
 	y, 
	100, 
 	bitrate_to_string(gui->string, gui->asset->ampeg_bitrate))
{
	this->gui = gui;
}

void MPEGABitrate::create_objects()
{
	set_layer(gui->asset->ampeg_derivative);
}

void MPEGABitrate::set_layer(int layer)
{
	while(total_items())
	{
		remove_item(0);
	}

	if(layer == 2)
	{
		add_item(new BC_MenuItem("160"));
		add_item(new BC_MenuItem("192"));
		add_item(new BC_MenuItem("224"));
		add_item(new BC_MenuItem("256"));
		add_item(new BC_MenuItem("320"));
		add_item(new BC_MenuItem("384"));
	}
	else
	{
		add_item(new BC_MenuItem("8"));
		add_item(new BC_MenuItem("16"));
		add_item(new BC_MenuItem("24"));
		add_item(new BC_MenuItem("32"));
		add_item(new BC_MenuItem("40"));
		add_item(new BC_MenuItem("48"));
		add_item(new BC_MenuItem("56"));
		add_item(new BC_MenuItem("64"));
		add_item(new BC_MenuItem("80"));
		add_item(new BC_MenuItem("96"));
		add_item(new BC_MenuItem("112"));
		add_item(new BC_MenuItem("128"));
		add_item(new BC_MenuItem("144"));
		add_item(new BC_MenuItem("160"));
		add_item(new BC_MenuItem("192"));
		add_item(new BC_MenuItem("224"));
		add_item(new BC_MenuItem("256"));
		add_item(new BC_MenuItem("320"));
	}
}

int MPEGABitrate::handle_event()
{
	gui->asset->ampeg_bitrate = string_to_bitrate(get_text());
	return 1;
};

int MPEGABitrate::string_to_bitrate(char *string)
{
	return atol(string);
}


char* MPEGABitrate::bitrate_to_string(char *string, int bitrate)
{
	sprintf(string, "%d", bitrate);
	return string;
}









MPEGConfigVideo::MPEGConfigVideo(BC_WindowBase *parent_window, 
	Asset *asset)
 : BC_Window(PROGRAM_NAME ": Video Compression",
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	500,
	400,
	-1,
	-1,
	0,
	0,
	1)
{
	this->parent_window = parent_window;
	this->asset = asset;
	reset_cmodel();
}

MPEGConfigVideo::~MPEGConfigVideo()
{
}

int MPEGConfigVideo::create_objects()
{
	int x = 10, y = 10;
	int x1 = x + 150;
	int x2 = x + 300;

	if(asset->format == FILE_MPEG)
	{
		add_subwindow(new BC_Title(x, y, _("No options for MPEG transport stream.")));
		return 0;
	}

	add_subwindow(new BC_Title(x, y, _("Color model:")));
	add_subwindow(cmodel = new MPEGColorModel(x1, y, this));
	cmodel->create_objects();
	y += 30;

	update_cmodel_objs();

	add_subwindow(new BC_OKButton(this));
	show_window();
	flush();
	return 0;
}

int MPEGConfigVideo::close_event()
{
	set_done(0);
	return 1;
}


void MPEGConfigVideo::delete_cmodel_objs()
{
	delete preset;
	delete derivative;
	delete bitrate;
	delete fixed_bitrate;
	delete quant;
	delete fixed_quant;
	delete iframe_distance;
	delete pframe_distance;
	delete top_field_first;
	delete progressive;
	delete denoise;
	delete seq_codes;
	titles.remove_all_objects();
	reset_cmodel();
}

void MPEGConfigVideo::reset_cmodel()
{
	preset = 0;
	derivative = 0;
	bitrate = 0;
	fixed_bitrate = 0;
	quant = 0;
	fixed_quant = 0;
	iframe_distance = 0;
	pframe_distance = 0;
	top_field_first = 0;
	progressive = 0;
	denoise = 0;
	seq_codes = 0;
}

void MPEGConfigVideo::update_cmodel_objs()
{
	BC_Title *title;
	int x = 10;
	int y = 40;
	int x1 = x + 150;
	int x2 = x + 280;

	delete_cmodel_objs();

	if(asset->vmpeg_cmodel == MPEG_YUV420)
	{
		add_subwindow(title = new BC_Title(x, y + 5, _("Format Preset:")));
		titles.append(title);
		add_subwindow(preset = new MPEGPreset(x1, y, this));
		preset->create_objects();
		y += 30;
	}

	add_subwindow(title = new BC_Title(x, y + 5, _("Derivative:")));
	titles.append(title);
	add_subwindow(derivative = new MPEGDerivative(x1, y, this));
	derivative->create_objects();
	y += 30;

	add_subwindow(title = new BC_Title(x, y + 5, _("Bitrate:")));
	titles.append(title);
	add_subwindow(bitrate = new MPEGBitrate(x1, y, this));
	add_subwindow(fixed_bitrate = new MPEGFixedBitrate(x2, y, this));
	y += 30;

	add_subwindow(title = new BC_Title(x, y, _("Quantization:")));
	titles.append(title);
	quant = new MPEGQuant(x1, y, this);
	quant->create_objects();
	add_subwindow(fixed_quant = new MPEGFixedQuant(x2, y, this));
	y += 30;

	add_subwindow(title = new BC_Title(x, y, _("I frame distance:")));
	titles.append(title);
	iframe_distance = new MPEGIFrameDistance(x1, y, this);
	iframe_distance->create_objects();
	y += 30;

	if(asset->vmpeg_cmodel == MPEG_YUV420)
	{
  		add_subwindow(title = new BC_Title(x, y, _("P frame distance:")));
		titles.append(title);
		pframe_distance = new MPEGPFrameDistance(x1, y, this);
		pframe_distance->create_objects();
  		y += 30;

		add_subwindow(top_field_first = new BC_CheckBox(x, y, &asset->vmpeg_field_order, _("Bottom field first")));
  		y += 30;
	}

	add_subwindow(progressive = new BC_CheckBox(x, y, &asset->vmpeg_progressive, _("Progressive frames")));
	y += 30;
	add_subwindow(denoise = new BC_CheckBox(x, y, &asset->vmpeg_denoise, _("Denoise")));
	y += 30;
	add_subwindow(seq_codes = new BC_CheckBox(x, y, &asset->vmpeg_seq_codes, _("Sequence start codes in every GOP")));

}













MPEGDerivative::MPEGDerivative(int x, int y, MPEGConfigVideo *gui)
 : BC_PopupMenu(x, y, 150, derivative_to_string(gui->asset->vmpeg_derivative))
{
	this->gui = gui;
}

void MPEGDerivative::create_objects()
{
	add_item(new BC_MenuItem(derivative_to_string(1)));
	add_item(new BC_MenuItem(derivative_to_string(2)));
}

int MPEGDerivative::handle_event()
{
	gui->asset->vmpeg_derivative = string_to_derivative(get_text());
	return 1;
};

int MPEGDerivative::string_to_derivative(char *string)
{
	if(!strcasecmp(derivative_to_string(1), string))
		return 1;
	if(!strcasecmp(derivative_to_string(2), string))
		return 2;

	return 1;
}

char* MPEGDerivative::derivative_to_string(int derivative)
{
	switch(derivative)
	{
		case 1:
			return _("MPEG-1");
			break;
		
		case 2:
			return _("MPEG-2");
			break;
			
		default:
			return _("MPEG-1");
			break;
	}
}











MPEGPreset::MPEGPreset(int x, int y, MPEGConfigVideo *gui)
 : BC_PopupMenu(x, y, 200, value_to_string(gui->asset->vmpeg_preset))
{
	this->gui = gui;
}

void MPEGPreset::create_objects()
{
	for(int i = 0; i < 10; i++)
	{
		add_item(new BC_MenuItem(value_to_string(i)));
	}
}

int MPEGPreset::handle_event()
{
	gui->asset->vmpeg_preset = string_to_value(get_text());
	return 1;
}

int MPEGPreset::string_to_value(char *string)
{
	for(int i = 0; i < 10; i++)
	{
		if(!strcasecmp(value_to_string(i), string))
			return i;
	}
	return 0;
}

char* MPEGPreset::value_to_string(int derivative)
{
	switch(derivative)
	{
		case 0: return _("Generic MPEG-1"); break;
		case 1: return _("standard VCD"); break;
		case 2: return _("user VCD"); break;
		case 3: return _("Generic MPEG-2"); break;
		case 4: return _("standard SVCD"); break;
		case 5: return _("user SVCD"); break;
		case 6: return _("VCD Still sequence"); break;
		case 7: return _("SVCD Still sequence"); break;
		case 8: return _("DVD NAV"); break;
		case 9: return _("DVD"); break;
		default: return _("Generic MPEG-1"); break;
	}
}











MPEGBitrate::MPEGBitrate(int x, int y, MPEGConfigVideo *gui)
 : BC_TextBox(x, y, 100, 1, gui->asset->vmpeg_bitrate)
{
	this->gui = gui;
}


int MPEGBitrate::handle_event()
{
	gui->asset->vmpeg_bitrate = atol(get_text());
	return 1;
};





MPEGQuant::MPEGQuant(int x, int y, MPEGConfigVideo *gui)
 : BC_TumbleTextBox(gui, 
 	(int64_t)gui->asset->vmpeg_quantization, 
	(int64_t)1,
	(int64_t)100,
	x, 
	y,
	100)
{
	this->gui = gui;
}

int MPEGQuant::handle_event()
{
	gui->asset->vmpeg_quantization = atol(get_text());
	return 1;
};

MPEGFixedBitrate::MPEGFixedBitrate(int x, int y, MPEGConfigVideo *gui)
 : BC_Radial(x, y, gui->asset->vmpeg_fix_bitrate, _("Fixed bitrate"))
{
	this->gui = gui;
}

int MPEGFixedBitrate::handle_event()
{
	update(1);
	gui->asset->vmpeg_fix_bitrate = 1;
	gui->fixed_quant->update(0);
	return 1;
};

MPEGFixedQuant::MPEGFixedQuant(int x, int y, MPEGConfigVideo *gui)
 : BC_Radial(x, y, !gui->asset->vmpeg_fix_bitrate, _("Fixed quantization"))
{
	this->gui = gui;
}

int MPEGFixedQuant::handle_event()
{
	update(1);
	gui->asset->vmpeg_fix_bitrate = 0;
	gui->fixed_bitrate->update(0);
	return 1;
};









MPEGIFrameDistance::MPEGIFrameDistance(int x, int y, MPEGConfigVideo *gui)
 : BC_TumbleTextBox(gui, 
 	(int64_t)gui->asset->vmpeg_iframe_distance, 
	(int64_t)1,
	(int64_t)100,
	x, 
	y,
	50)
{
	this->gui = gui;
}

int MPEGIFrameDistance::handle_event()
{
	gui->asset->vmpeg_iframe_distance = atoi(get_text());
	return 1;
}







MPEGPFrameDistance::MPEGPFrameDistance(int x, int y, MPEGConfigVideo *gui)
 : BC_TumbleTextBox(gui, 
 	(int64_t)gui->asset->vmpeg_pframe_distance, 
	(int64_t)0,
	(int64_t)2,
	x, 
	y,
	50)
{
	this->gui = gui;
}

int MPEGPFrameDistance::handle_event()
{
	gui->asset->vmpeg_pframe_distance = atoi(get_text());
	return 1;
}








MPEGColorModel::MPEGColorModel(int x, int y, MPEGConfigVideo *gui)
 : BC_PopupMenu(x, y, 150, cmodel_to_string(gui->asset->vmpeg_cmodel))
{
	this->gui = gui;
}

void MPEGColorModel::create_objects()
{
	add_item(new BC_MenuItem(cmodel_to_string(0)));
	add_item(new BC_MenuItem(cmodel_to_string(1)));
}

int MPEGColorModel::handle_event()
{
	gui->asset->vmpeg_cmodel = string_to_cmodel(get_text());
	gui->update_cmodel_objs();
	return 1;
};

int MPEGColorModel::string_to_cmodel(char *string)
{
	if(!strcasecmp(cmodel_to_string(0), string))
		return 0;
	if(!strcasecmp(cmodel_to_string(1), string))
		return 1;
	return 1;
}

char* MPEGColorModel::cmodel_to_string(int cmodel)
{
	switch(cmodel)
	{
		case MPEG_YUV420:
			return _("YUV 4:2:0");
			break;
		
		case MPEG_YUV422:
			return _("YUV 4:2:2");
			break;
			
		default:
			return _("YUV 4:2:0");
			break;
	}
}






