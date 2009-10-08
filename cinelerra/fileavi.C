
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

#ifdef USE_AVIFILE
#include "avifile.h"
#include "creators.h"
#include "except.h"
#include "avm_fourcc.h"
#include "StreamInfo.h"
#endif

#include "clip.h"
#include "file.h"
#include "fileavi.h"
#include "fileavi.inc"
#include "interlacemodes.h"
#include "mwindow.inc"
#include "vframe.h"


#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)





#ifdef USE_AVIFILE
int FileAVI::avifile_initialized = 0;
#endif




// Status of AVI derivatives:
// Arne2 - depends on Kino, DV only
// Lavtools - 2 gig limited












FileAVI::FileAVI(Asset *asset, File *file)
 : FileBase(asset, file)
{
	reset();
}

FileAVI::~FileAVI()
{
	close_file();
}

int FileAVI::check_sig(Asset *asset)
{
// Pick whoever gets the most tracks
	int score_lavtools = 0;
	int score_arne2 = 0;
	int score_arne1 = 0;
	int score_avifile = 0;
	int final = 0;
	int result = 0;





	check_sig_lavtools(asset, score_lavtools);
	check_sig_arne2(asset, score_arne2);
	check_sig_arne1(asset, score_arne1);
	check_sig_avifile(asset, score_avifile);

	if(score_lavtools > final) 
	{
		final = score_lavtools;
		result = FILE_AVI_LAVTOOLS;
	}
	if(score_arne2 > final)
	{
		final = score_arne2;
		result = FILE_AVI_ARNE2;
	}
	if(score_arne1 > final)
	{
		final = score_arne1;
		result = FILE_AVI_ARNE1;
	}
	if(score_avifile > final)
	{
		final = score_avifile;
		result = FILE_AVI_AVIFILE;
	}







	return result;
}

int FileAVI::check_sig_arne2(Asset *asset, int &score)
{
	return 0;
}

int FileAVI::check_sig_arne1(Asset *asset, int &score)
{
	return 0;
}

int FileAVI::check_sig_lavtools(Asset *asset, int &score)
{
	return 0;
}

int FileAVI::check_sig_avifile(Asset *asset, int &score)
{
return 0;
#ifdef USE_AVIFILE
	IAviReadFile *in_fd = 0;

	try
	{
		in_fd = CreateIAviReadFile(asset->path);
	}
	catch(FatalError& error)
	{
		if(in_fd) delete in_fd;
		return 0;
	}

	int vtracks = in_fd->VideoStreamCount();
	int atracks = in_fd->AudioStreamCount();

	delete in_fd;
	
	score = vtracks + atracks;
	return 1;
#endif
	return 0;
}


char* FileAVI::vcodec_to_fourcc(char *input, char *output)
{
#ifdef USE_AVIFILE
	initialize_avifile();
	for(avm::vector<CodecInfo>::iterator i = video_codecs.begin();
		i < video_codecs.end();
		i++)
	{
		if(i->direction & CodecInfo::Encode)
		{
			if(!strcasecmp(i->GetName(), input))
			{
				memcpy(output, (char*)&i->fourcc, 4);
				output[4] = 0;
				return output;
			}
		}
	}
	
	output[0] = 0;
	return output;
#else
	return input;
#endif
}

char* FileAVI::fourcc_to_vcodec(char *input, char *output)
{
#ifdef USE_AVIFILE
// Construct codec item list
	initialize_avifile();
	for(avm::vector<CodecInfo>::iterator i = video_codecs.begin();
		i < video_codecs.end();
		i++)
	{
		if(i->direction & CodecInfo::Encode)
		{
			if(!memcmp((char*)&i->fourcc, input, 4))
			{
				memcpy(output, (char*)&i->fourcc, 4);
				strcpy(output, i->GetName());;
				return output;
			}
		}
	}
	
	output[0] = 0;
	return output;
#else
	return input;
#endif
}


char* FileAVI::acodec_to_fourcc(char *input, char *output)
{
#ifdef USE_AVIFILE
// Construct codec item list
	initialize_avifile();
	for(avm::vector<CodecInfo>::iterator i = audio_codecs.begin();
		i < audio_codecs.end();
		i++)
	{
		if(i->direction & CodecInfo::Encode)
		{
			if(!strcasecmp(i->GetName(), input))
			{
				memcpy(output, (char*)&i->fourcc, 4);
				output[4] = 0;
				return output;
			}
		}
	}

	output[0] = 0;
	return output;
#else
	return input;
#endif
}

char* FileAVI::fourcc_to_acodec(char *input, char *output)
{
#ifdef USE_AVIFILE
// Construct codec item list
	initialize_avifile();
	for(avm::vector<CodecInfo>::iterator i = audio_codecs.begin();
		i < audio_codecs.end();
		i++)
	{
		if(i->direction & CodecInfo::Encode)
		{
			if(!memcmp((char*)&i->fourcc, input, 4))
			{
				memcpy(output, (char*)&i->fourcc, 4);
				strcpy(output, i->GetName());
				return output;
			}
		}
	}
	
	output[0] = 0;
	return output;
#else
	return input;
#endif
}


void FileAVI::initialize_avifile()
{
#ifdef USE_AVIFILE
	if(!avifile_initialized)
	{
    	BITMAPINFOHEADER bih;
    	bih.biCompression = 0xffffffff;
    	Creators::CreateVideoDecoder(bih, 0, 0);

    	WAVEFORMATEX wih;
    	memset(&wih, 0xff, sizeof(WAVEFORMATEX));
		Creators::CreateAudioDecoder(&wih, 0);
		avifile_initialized = 1;
	}
#endif
}


int FileAVI::open_file(int rd, int wr)
{
	if(wr)
	{
		switch(asset->format)
		{
			case FILE_AVI_LAVTOOLS:
				return open_lavtools_out(asset);
				break;

			case FILE_AVI_ARNE2:
				return open_arne2_out(asset);
				break;

			case FILE_AVI_ARNE1:
				return open_arne1_out(asset);
				break;

			case FILE_AVI_AVIFILE:
				return open_avifile_out(asset);
				break;
		}
	}
	else
	if(rd)
	{
		asset->format = check_sig(asset);


		switch(asset->format)
		{
			case FILE_AVI_LAVTOOLS:
				return open_lavtools_in(asset);
				break;

			case FILE_AVI_ARNE2:
				return open_arne2_in(asset);
				break;

			case FILE_AVI_ARNE1:
				return open_arne1_in(asset);
				break;

			case FILE_AVI_AVIFILE:
				return open_avifile_in(asset);
				break;
		}
	}

	return 0;
}



int FileAVI::open_avifile_out(Asset *asset)
{
#ifdef USE_AVIFILE
	try
	{
		out_fd = CreateIAviWriteFile(asset->path);
	}
	catch(FatalError& error)
	{
		error.Print();
		close_file();
		return 1;
	}

	if(asset->video_data)
	{
		out_color_model = get_best_colormodel(-1, -1);
		out_bitmap_info = new BitmapInfo(asset->width, asset->height, 
			cmodel_bc_to_avi(out_color_model));
		for(int i = 0; i < asset->layers && i < MAX_STREAMS; i++)
		{
			vstream_out[i] = out_fd->AddVideoStream(*(uint32_t*)asset->vcodec, 
				out_bitmap_info, 
				int(1000000.0 / asset->frame_rate));
		}
	}

	if(asset->audio_data)
	{
		WAVEFORMATEX wfm;
		wfm.wFormatTag = 1; // PCM
		wfm.nChannels = asset->channels;
		wfm.nSamplesPerSec = asset->sample_rate;
		wfm.nAvgBytesPerSec = 2 * asset->sample_rate * asset->channels;
		wfm.nBlockAlign = 2 * asset->channels;
		wfm.wBitsPerSample = 16;
		wfm.cbSize = 0;

		for(int i = 0; i < asset->channels && i < MAX_STREAMS; i++)
		{
			astream_out[i] = out_fd->AddStream(AviStream::Audio,
				&wfm, 
				sizeof(wfm),
				1,                   // uncompressed PCM data
				wfm.nAvgBytesPerSec, // bytes/sec
				wfm.nBlockAlign);    // bytes/sample
		}
	}

	return 0;
#endif
	return 1;
}

int FileAVI::open_arne2_out(Asset *asset)
{
	
	return 0;
}

int FileAVI::open_arne1_out(Asset *asset)
{
	return 0;
}

int FileAVI::open_lavtools_out(Asset *asset)
{
	return 0;
}

	
	
	


int FileAVI::open_avifile_in(Asset *asset)
{
#ifdef USE_AVIFILE
	try
	{
		in_fd = CreateIAviReadFile(asset->path);
	}
	catch(FatalError& error)
	{
		error.Print();
		close_file();
		return 1;
	}

	asset->layers = in_fd->VideoStreamCount();
	if(asset->layers)
	{
		for(int i = 0; i < asset->layers && i < MAX_STREAMS; i++)
		{
			vstream_in[i] = in_fd->GetStream(i, IAviReadStream::Video);
			vstream_in[i]->StartStreaming();
			vstream_in[i]->GetDecoder()->SetDestFmt(24);
			vstream_in[i]->Seek(0);
//printf("FileAVI::open_file %d %p\n", i, vstream[i]);
		}

		StreamInfo *stream_info = vstream_in[0]->GetStreamInfo();
		asset->video_data = 1;
		if(!asset->frame_rate)
			asset->frame_rate = (double)1 / vstream_in[0]->GetFrameTime();
		asset->video_length = stream_info->GetStreamFrames();
	    BITMAPINFOHEADER bh;
		vstream_in[0]->GetVideoFormatInfo(&bh, sizeof(bh));
	    asset->width = bh.biWidth;
	    asset->height = bh.biHeight;
		asset->interlace_mode = BC_ILACE_MODE_UNDETECTED; // FIXME

		uint32_t fourcc = stream_info->GetFormat();
		asset->vcodec[0] = ((char*)&fourcc)[0];
		asset->vcodec[1] = ((char*)&fourcc)[1];
		asset->vcodec[2] = ((char*)&fourcc)[2];
		asset->vcodec[3] = ((char*)&fourcc)[3];
		source_cmodel = BC_RGB888;
		delete stream_info;
	}

	asset->audio_data = in_fd->AudioStreamCount();

	if(asset->audio_data)
	{
		char *extinfo;

		for(int i = 0; i < 1 && i < MAX_STREAMS; i++)
		{
			astream_in[i] = in_fd->GetStream(i, IAviReadStream::Audio);
			astream_in[i]->StartStreaming();
		}

		StreamInfo *stream_info = astream_in[0]->GetStreamInfo();
		asset->channels = stream_info->GetAudioChannels();
		if(asset->sample_rate == 0)
			asset->sample_rate = stream_info->GetAudioSamplesPerSec();
		asset->bits = MAX(16, stream_info->GetAudioBitsPerSample());
		asset->audio_length = stream_info->GetStreamFrames();
		delete stream_info;
	}
asset->dump();
	return 0;
#endif
	return 1;
}

int FileAVI::open_arne2_in(Asset *asset)
{
	return 0;
}

int FileAVI::open_arne1_in(Asset *asset)
{
	return 0;
}

int FileAVI::open_lavtools_in(Asset *asset)
{
	return 0;
}


















int FileAVI::close_file()
{
#ifdef USE_AVIFILE
	if(in_fd) delete in_fd;
	if(out_fd) delete out_fd;
	if(out_bitmap_info) delete out_bitmap_info;
#endif
	if(temp_audio) delete [] temp_audio;
	reset();
}

int FileAVI::cmodel_bc_to_avi(int input)
{
#ifdef USE_AVIFILE
	switch(input)
	{
		case BC_YUV422:
			return fccYUY2;
			break;

		case BC_YUV420P:
			return fccYV12;
			break;
	}
#endif
	return 24;
}

void FileAVI::reset()
{
#ifdef USE_AVIFILE
	in_fd = 0;
	out_fd = 0;
	out_bitmap_info = 0;
#endif
	temp_audio = 0;
	temp_allocated = 0;
}

int FileAVI::get_best_colormodel(int driver, int colormodel)
{
	if(colormodel > -1)
	{
		return colormodel;
	}
	else
	{
		
		return BC_RGB888;
	}
}


void FileAVI::get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int audio_options,
		int video_options,
		char *locked_compressor)
{
	if(audio_options)
	{
		AVIConfigAudio *window = new AVIConfigAudio(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
	else
	if(video_options)
	{
//printf("FileAVI::get_parameters 1\n");
		AVIConfigVideo *window = new AVIConfigVideo(parent_window,
			asset,
			locked_compressor);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}

int FileAVI::set_audio_position(int64_t x)
{
#ifdef USE_AVIFILE
// quicktime sets positions for each track seperately so store position in audio_position
	if(x >= 0 && x < asset->audio_length)
		return astream_in[file->current_layer]->Seek(x);
	else
		return 1;
#endif
}

int FileAVI::set_video_position(int64_t x)
{
#ifdef USE_AVIFILE
	if(x >= 0 && x < asset->video_length)
		return vstream_in[file->current_layer]->Seek(x);
	else
		return 1;
#endif
}

int FileAVI::read_samples(double *buffer, int64_t len)
{
#ifdef USE_AVIFILE
	Unsigned samples_read, bytes_read;

printf("FileAVI::read_samples 1\n");
	if(temp_audio && temp_allocated < len * asset->bits / 8 * asset->channels)
	{
		delete [] temp_audio;
		temp_allocated = 0;
	}
	if(!temp_allocated)
	{
		temp_allocated = len * asset->bits / 8 * asset->channels;
		temp_audio = new unsigned char[temp_allocated];
	}

	astream_in[0]->ReadFrames((void*)temp_audio, 
		(unsigned int)temp_allocated,
	 	(unsigned int)len,
	 	samples_read, 
		bytes_read);
	
// Extract single channel
	switch(asset->bits)
	{
		case 16:
		{
			int16_t *temp_int16 = (int16_t*)temp_audio;
			for(int i = 0, j = file->current_channel; 
				i < len;
				i++, j += asset->channels)
			{
				buffer[i] = (double)temp_int16[j] / 32767;
			}
			break;
		}
	}

#endif


	return 0;
}

int FileAVI::read_frame(VFrame *frame)
{
	int result = 0;

#ifdef USE_AVIFILE
	vstream_in[file->current_layer]->ReadFrame();
	CImage *temp_image = vstream_in[file->current_layer]->GetFrame();
//printf("FileAVI::read_frame 1 %d %d\n", source_cmodel, frame->get_color_model());
	switch(source_cmodel)
	{
		case BC_RGB888:
			if(frame->get_color_model() == BC_RGB888)
				bcopy(temp_image->Data(), 
					frame->get_data(), 
					VFrame::calculate_data_size(asset->width, 
						asset->height, 
						-1, 
						BC_RGB888));
			break;
	}
#endif
	return result;
}

int64_t FileAVI::compressed_frame_size()
{
	int result = 0;
	return result;
}

int FileAVI::read_compressed_frame(VFrame *buffer)
{
	int64_t result = 0;

	return result;
}

int FileAVI::write_compressed_frame(VFrame *buffer)
{
	int result = 0;

	return result;
}

int FileAVI::write_frames(VFrame ***frames, int len)
{
	return 0;
}


int FileAVI::write_samples(double **buffer, int64_t len)
{
	return 0;
}





AVIConfigAudio::AVIConfigAudio(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Audio compression",
 	parent_window->get_abs_cursor_x(1),
	parent_window->get_abs_cursor_y(1),
	calculate_w(asset->format),
	calculate_h(asset->format))
{
	this->parent_window = parent_window;
	this->asset = asset;
}

AVIConfigAudio::~AVIConfigAudio()
{
}

int AVIConfigAudio::calculate_w(int format)
{
	switch(format)
	{
		case FILE_AVI_AVIFILE: return 400; break;
		case FILE_AVI_ARNE2: return 250; break;
	}
}

int AVIConfigAudio::calculate_h(int format)
{
	switch(format)
	{
		case FILE_AVI_AVIFILE: return 200; break;
		case FILE_AVI_ARNE2: return 100; break;
	}
}

int AVIConfigAudio::create_objects()
{
	switch(asset->format)
	{
#ifdef USE_AVIFILE
		case FILE_AVI_AVIFILE:
		{
			generate_codeclist();

			int x = 10, y = 10;
			BC_Title *title;
			add_subwindow(title = new BC_Title(x, y, _("Codec: ")));
			list = new AVIACodecList(this, x, y);
			list->create_objects();
			y += list->get_h();
			break;
		}
#endif

		case FILE_AVI_ARNE2:
			add_subwindow(new BC_Title(10, 10, _("Compressor: 16 bit PCM")));
			break;
	}

	add_subwindow(new BC_OKButton(this));
	return 0;
}

int AVIConfigAudio::close_event()
{
	return 1;
}

int AVIConfigAudio::generate_codeclist()
{
	FileAVI::initialize_avifile();
	codec_items.remove_all_objects();

	switch(asset->format)
	{
#ifdef USE_AVIFILE
		case FILE_AVI_AVIFILE:
			for(avm::vector<CodecInfo>::iterator i = audio_codecs.begin();
				i < audio_codecs.end();
				i++)
			{
				if(i->direction & CodecInfo::Encode)
				{
					codec_items.append(new BC_ListBoxItem((char*)i->GetName()));
				}
			}
			break;
#endif
	}

	return 0;
}

void AVIConfigAudio::update_codecs()
{
	
}









AVIACodecList::AVIACodecList(AVIConfigAudio *gui, int x, int y)
 : BC_PopupTextBox(gui,
 		&gui->codec_items,
		FileAVI::fourcc_to_acodec(gui->asset->acodec, gui->string),
 		x, 
		y,
		200,
		400)
{
	this->gui = gui;
}

AVIACodecList::~AVIACodecList()
{
}

int AVIACodecList::handle_event()
{
	strcpy(gui->asset->acodec, FileAVI::acodec_to_fourcc(get_text(), gui->string));
	return 1;
}





AVIConfigVideo::AVIConfigVideo(BC_WindowBase *parent_window, 
		Asset *asset, 
		char *locked_compressor)
 : BC_Window(PROGRAM_NAME ": Video Compression",
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	calculate_w(asset->format),
	calculate_h(asset->format))
{
	this->parent_window = parent_window;
	this->asset = asset;
	this->locked_compressor = locked_compressor;
	reset();
}

AVIConfigVideo::~AVIConfigVideo()
{
	codec_items.remove_all_objects();
	attribute_items[0].remove_all_objects();
	attribute_items[1].remove_all_objects();
}

void AVIConfigVideo::reset()
{
	attributes = 0;
	attribute = 0;
}

int AVIConfigVideo::calculate_w(int format)
{
	switch(format)
	{
		case FILE_AVI_AVIFILE: return 400; break;
		case FILE_AVI_ARNE2: return 250; break;
	}
}

int AVIConfigVideo::calculate_h(int format)
{
	switch(format)
	{
		case FILE_AVI_AVIFILE: return 320; break;
		case FILE_AVI_ARNE2: return 100; break;
	}
}

int AVIConfigVideo::create_objects()
{
	switch(asset->format)
	{
#ifdef USE_AVIFILE
		case FILE_AVI_AVIFILE:
		{
			generate_codeclist();
			generate_attributelist();

			int x = 10, y = 10, x1 = 90;
			BC_Title *title;
			add_subwindow(title = new BC_Title(x, y, _("Codec: ")));
			list = new AVIVCodecList(this, x1, y);
			list->create_objects();
			y += list->get_h() + 5;

			add_subwindow(title = new BC_Title(x, y, _("Attributes:")));
			add_subwindow(attributes = new AVIVAttributeList(this, x1, y));
			y += attributes->get_h() + 5;

			add_subwindow(new BC_Title(x, y, _("Value:")));
			add_subwindow(attribute = new AVIVAttribute(this, x1, y));
			break;
		}
#endif

		case FILE_AVI_ARNE2:
			add_subwindow(new BC_Title(10, 10, _("Compressor: Consumer DV")));
			break;
	}

	add_subwindow(new BC_OKButton(this));
	return 0;
}

int AVIConfigVideo::close_event()
{
	return 1;
}

int AVIConfigVideo::generate_codeclist()
{
	FileAVI::initialize_avifile();
	switch(asset->format)
	{
		case FILE_AVI_AVIFILE:
#ifdef USE_AVIFILE
// Construct codec item list
			for(avm::vector<CodecInfo>::iterator i = video_codecs.begin();
				i < video_codecs.end();
				i++)
			{
				if(i->direction & CodecInfo::Encode)
				{
					codec_items.append(new BC_ListBoxItem((char*)i->GetName()));
				}
			}
#endif
			break;
	}

	return 0;
}

void AVIConfigVideo::generate_attributelist()
{
#ifdef USE_AVIFILE
// Remember selection number
	int selection_number = attributes ? attributes->get_selection_number(0, 0) : -1;
	attribute_items[0].remove_all_objects();
	attribute_items[1].remove_all_objects();
	FileAVI::initialize_avifile();

	for(avm::vector<CodecInfo>::iterator i = video_codecs.begin();
		i < video_codecs.end();
		i++)
	{
		if(!memcmp((char*)&i->fourcc, asset->vcodec, 4))
		{
			avm::vector<AttributeInfo>& attributes = i->encoder_info;

			for(avm::vector<AttributeInfo>::const_iterator j = attributes.begin();
				j != attributes.end();
				j++)
			{
				char *name = (char*)j->GetName();
				char value[BCTEXTLEN];
				value[0] = 0;

//printf("AVIConfigVideo::generate_attributelist %d\n", j->kind);
				switch(j->kind)
				{
					case AttributeInfo::Integer:
					{
						int temp = 0;
						Creators::GetCodecAttr(*i, name, temp);
						sprintf(value, "%d", temp);
						break;
					}

					case AttributeInfo::Select:
					{
						int temp = 0;
						Creators::GetCodecAttr(*i, name, temp);
						sprintf(value, "%d ( %s )", temp, j->options[temp].c_str());
						break;
					}

					case AttributeInfo::String:
					{
						const char * temp = 0;
						Creators::GetCodecAttr(*i, name, &temp);
						if(temp) strncpy(value, temp, BCTEXTLEN);
						break;
					}
				}

				attribute_items[0].append(new BC_ListBoxItem(name));
				attribute_items[1].append(new BC_ListBoxItem(value));

				int current_number = j - attributes.begin();
				if(current_number == selection_number)
				{
					attribute_items[0].values[current_number]->set_selected(1);
					attribute_items[1].values[current_number]->set_selected(1);
				}
			}
		}
	}
#endif
}

char* AVIConfigVideo::get_current_attribute_text()
{
	BC_ListBoxItem *item = attributes->get_selection(0, 0);

	if(item)
	{
		return item->get_text();
	}
	else
		return "";
}

char* AVIConfigVideo::get_current_attribute_value()
{
	BC_ListBoxItem *item = attributes->get_selection(1, 0);

	if(item)
	{
		return item->get_text();
	}
	else
		return "";
}

void AVIConfigVideo::set_current_attribute(char *text)
{
#ifdef USE_AVIFILE
	int number = attributes->get_selection_number(0, 0);

	if(number >= 0)
	{
		FileAVI::initialize_avifile();

		for(avm::vector<CodecInfo>::iterator i = video_codecs.begin();
			i < video_codecs.end();
			i++)
		{
			if(!memcmp((char*)&i->fourcc, asset->vcodec, 4))
			{
				avm::vector<AttributeInfo>& attributes = i->encoder_info;
				AttributeInfo& attribute = attributes[number];

				switch(attribute.kind)
				{
					case AttributeInfo::Integer:
						Creators::SetCodecAttr(*i, attribute.GetName(), atol(text));
						break;

					case AttributeInfo::Select:
						Creators::SetCodecAttr(*i, attribute.GetName(), atol(text));
						break;

					case AttributeInfo::String:
						Creators::SetCodecAttr(*i, attribute.GetName(), text);
						break;
				}
			}
		}
		
		
		update_attribute(1);
	}
#endif
}



void AVIConfigVideo::update_attribute(int recursive)
{
	generate_attributelist();
	attributes->update(attribute_items,
						0,
						0,
						2,
						attributes->get_xposition(),
						attributes->get_yposition(), 
						0,
						1);

	if(!recursive) attribute->update(get_current_attribute_value());
}









AVIVCodecList::AVIVCodecList(AVIConfigVideo *gui, int x, int y)
 : BC_PopupTextBox(gui,
 		&gui->codec_items,
		FileAVI::fourcc_to_vcodec(gui->asset->vcodec, gui->string),
 		x, 
		y,
		280,
		400)
{
	this->gui = gui;
}

AVIVCodecList::~AVIVCodecList()
{
}

int AVIVCodecList::handle_event()
{
	strcpy(gui->asset->vcodec, FileAVI::vcodec_to_fourcc(get_text(), gui->string));
	gui->update_attribute(0);
	return 1;
}



AVIVAttributeList::AVIVAttributeList(AVIConfigVideo *gui, int x, int y)
 : BC_ListBox(x, 
		y,
		300,
		200,
		LISTBOX_TEXT,
		gui->attribute_items,
		0,
		0,
		2)
{
	this->gui = gui;
}

int AVIVAttributeList::handle_event()
{
	gui->update_attribute(0);
	return 1;
}

int AVIVAttributeList::selection_changed()
{
	gui->update_attribute(0);
	return 1;
}




AVIVAttribute::AVIVAttribute(AVIConfigVideo *gui, int x, int y)
 : BC_TextBox(x, y, 300, 1, gui->get_current_attribute_value())
{
	this->gui = gui;
}

int AVIVAttribute::handle_event()
{
	gui->set_current_attribute(get_text());
	return 1;
}



