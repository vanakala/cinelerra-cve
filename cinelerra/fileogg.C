
#include "clip.h"
#include "asset.h"
#include "bcsignals.h"
#include "byteorder.h"
#include "edit.h"
#include "file.h"
#include "fileogg.h"
#include "guicast.h"
#include "language.h"
#include "mwindow.inc"
#include "quicktime.h"
#include "vframe.h"
#include "videodevice.inc"
#include "cmodel_permutation.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

FileOGG::FileOGG(Asset *asset, File *file)
 : FileBase(asset, file)
{
	if(asset->format == FILE_UNKNOWN)
		asset->format = FILE_OGG;
	asset->byte_order = 0;
	reset_parameters();
}

FileOGG::~FileOGG()
{

	int i = 0;
	if (tf) delete tf;
	if (temp_frame) delete temp_frame;
	if(stream) close_file();

}

void FileOGG::get_parameters(BC_WindowBase *parent_window,
	Asset *asset,
	BC_WindowBase* &format_window,
	int audio_options,
	int video_options)
{
	if(audio_options)
	{
		OGGConfigAudio *window = new OGGConfigAudio(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
	else
	if(video_options)
	{
		OGGConfigVideo *window = new OGGConfigVideo(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}

}

int FileOGG::reset_parameters_derived()
{
	tf = 0;
	temp_frame = 0;
	stream = 0;
}

int FileOGG::open_file(int rd, int wr)
{
	this->rd = rd;
	this->wr = wr;

TRACE("FileOGG::open_file 10")
	if (!tf)
		tf = new theoraframes_info_t;
	if(wr)
	{

TRACE("FileOGG::open_file 20")

		if((stream = fopen(asset->path, "w+b")) == 0)
		{
			perror(_("FileOGG::open_file rdwr"));
			return 1;
		}

		tf->audio_bytesout = 0;
		tf->video_bytesout = 0;
		tf->videoflag = 0;		
		tf->audioflag = 0;
		tf->videotime = 0;
		tf->audiotime = 0;
		/* yayness.  Set up Ogg output stream */
		srand (time (NULL));
		ogg_stream_init (&tf->vo, rand ());    

		if(asset->video_data)
		{
			ogg_stream_init (&tf->to, rand ());    /* oops, add one ot the above */
		
			theora_info_init (&tf->ti);
			
			tf->ti.frame_width = asset->width; 
			tf->ti.frame_height = asset->height;
			
			tf->ti.width = ((asset->width + 15) >>4)<<4; // round up to the nearest multiple of 16 
			tf->ti.height = ((asset->height + 15) >>4)<<4; // round up to the nearest multiple of 16
			if (tf->ti.width != tf->ti.frame_width || tf->ti.height != tf->ti.frame_height)
				printf("FileOGG: WARNING: Encoding theora when width or height are not dividable by 16 is suboptimal\n");
			
			tf->ti.offset_x = 0;
			tf->ti.offset_y = tf->ti.height - tf->ti.frame_height;
			tf->ti.fps_numerator = (unsigned int)(asset->frame_rate * 1000000);
			tf->ti.fps_denominator = 1000000;
			
			if (asset->aspect_ratio > 0)
			{
				// Cinelerra uses frame aspect ratio, theora uses pixel aspect ratio
				float pixel_aspect = asset->aspect_ratio / asset->width * asset->height;
				tf->ti.aspect_numerator = (unsigned int)(pixel_aspect * 1000000);
				tf->ti.aspect_denominator = 1000000;
			} else
			{
				tf->ti.aspect_numerator = 1000000;
				tf->ti.aspect_denominator = 1000000;
			}
			if(EQUIV(asset->frame_rate, 25) || EQUIV(asset->frame_rate, 50))
				tf->ti.colorspace = OC_CS_ITU_REC_470BG;
			else if((asset->frame_rate > 29 && asset->frame_rate < 31) || (asset->frame_rate > 59 && asset->frame_rate < 61) )
				tf->ti.colorspace = OC_CS_ITU_REC_470M;
			else
				tf->ti.colorspace = OC_CS_UNSPECIFIED;

			if (asset->theora_fix_bitrate)
			{
				tf->ti.target_bitrate = asset->theora_bitrate; 
				tf->ti.quality = 0;
			} else
			{
				tf->ti.target_bitrate = 0;
				tf->ti.quality = asset->theora_quality;     // video quality 0-63
			}
			tf->ti.dropframes_p = 0;
			tf->ti.quick_p = 1;
			tf->ti.keyframe_auto_p = 1;
			tf->ti.keyframe_frequency = asset->theora_keyframe_frequency;
			tf->ti.keyframe_frequency_force = asset->theora_keyframe_force_frequency;
			tf->ti.keyframe_data_target_bitrate = (unsigned int) (tf->ti.target_bitrate * 1.5) ;
			tf->ti.keyframe_auto_threshold = 80;
			tf->ti.keyframe_mindistance = 8;
			tf->ti.noise_sensitivity = 1;		
			tf->ti.sharpness = 2;
			
					
			if (theora_encode_init (&tf->td, &tf->ti))
			{
				printf("FileOGG: initialization of theora codec failed\n");
			}
		}
		/* init theora done */

		/* initialize Vorbis too, if we have audio. */
		if(asset->audio_data)
		{
			vorbis_info_init (&tf->vi);
			/* Encoding using a VBR quality mode.  */
			int ret;
			if(!asset->vorbis_vbr) 
			{
				ret = vorbis_encode_init(&tf->vi, 
			    				asset->channels, 
			    				asset->sample_rate, 
			    				asset->vorbis_max_bitrate, 
			    				asset->vorbis_bitrate,
			    				asset->vorbis_min_bitrate); 
			} else
			{
				// Set true VBR as demonstrated by http://svn.xiph.org/trunk/vorbis/doc/vorbisenc/examples.html
				ret = vorbis_encode_setup_managed(&tf->vi,
					asset->channels, 
					asset->sample_rate, 
					-1, 
					asset->vorbis_bitrate, 
					-1);
				ret |= vorbis_encode_ctl(&tf->vi, OV_ECTL_RATEMANAGE_AVG, NULL);
				ret |= vorbis_encode_setup_init(&tf->vi);
			}
			if (ret)
			{
				fprintf (stderr,
					"The Vorbis encoder could not set up a mode according to\n"
					"the requested quality or bitrate.\n\n");
				return 1;
			}

			vorbis_comment_init (&tf->vc);
			vorbis_comment_add_tag (&tf->vc, "ENCODER", PACKAGE_STRING);
			/* set up the analysis state and auxiliary encoding storage */
			vorbis_analysis_init (&tf->vd, &tf->vi);
			vorbis_block_init (&tf->vd, &tf->vb);

		}
		/* audio init done */

		/* write the bitstream header packets with proper page interleave */

		/* first packet will get its own page automatically */
		if(asset->video_data)
		{
			theora_encode_header (&tf->td, &tf->op);
			ogg_stream_packetin (&tf->to, &tf->op);
			if (ogg_stream_pageout (&tf->to, &tf->og) != 1)
			{
				fprintf (stderr, "Internal Ogg library error.\n");
				return 1;
			}
			fwrite (tf->og.header, 1, tf->og.header_len, stream);
			fwrite (tf->og.body, 1, tf->og.body_len, stream);

			/* create the remaining theora headers */
			theora_comment_init (&tf->tc);
			theora_comment_add_tag (&tf->tc, "ENCODER", PACKAGE_STRING);
			theora_encode_comment (&tf->tc, &tf->op);
			ogg_stream_packetin (&tf->to, &tf->op);
			theora_comment_clear(&tf->tc);

			theora_encode_tables (&tf->td, &tf->op);
			ogg_stream_packetin (&tf->to, &tf->op);
		}
		if(asset->audio_data){
			ogg_packet header;
			ogg_packet header_comm;
			ogg_packet header_code;

			vorbis_analysis_headerout (&tf->vd, &tf->vc, &header,
				       &header_comm, &header_code);
			ogg_stream_packetin (&tf->vo, &header);    /* automatically placed in its own
						 * page */
			if (ogg_stream_pageout (&tf->vo, &tf->og) != 1)
			{
				fprintf (stderr, "Internal Ogg library error.\n");
				return 1;
			}
			fwrite (tf->og.header, 1, tf->og.header_len, stream);
			fwrite (tf->og.body, 1, tf->og.body_len, stream);

			/* remaining vorbis header packets */
			ogg_stream_packetin (&tf->vo, &header_comm);
			ogg_stream_packetin (&tf->vo, &header_code);
		}

		/* Flush the rest of our headers. This ensures
		 * the actual data in each stream will start
		 * on a new page, as per spec. */
		while (1 && asset->video_data)
		{
			int result = ogg_stream_flush (&tf->to, &tf->og);
			if (result < 0)
			{
				/* can't get here */
				fprintf (stderr, "Internal Ogg library error.\n");
				return 1;
			}
			if (result == 0)
				break;
			fwrite (tf->og.header, 1, tf->og.header_len, stream);
			fwrite (tf->og.body, 1, tf->og.body_len, stream);
		}
		while (1 && asset->audio_data)
		{
			int result = ogg_stream_flush (&tf->vo, &tf->og);
			if (result < 0)
			{
				/* can't get here */
				fprintf (stderr, "Internal Ogg library error.\n");
				return 1;
			}
			if (result == 0)
				break;
			fwrite (tf->og.header, 1, tf->og.header_len, stream);
			fwrite (tf->og.body, 1, tf->og.body_len, stream);
		}


	}
	if (rd)
		return 1;
	return 0;
}

int FileOGG::check_sig(Asset *asset)
{
	return 0;
}

int FileOGG::close_file()
{
	if (wr)
	{
		if (asset->audio_data)
			write_samples_vorbis(0, 0, 1); // set eos
		if (asset->video_data)
			write_frames_theora(0, 1, 1); // set eos

		flush_ogg(1); // flush all
		
		if (asset->audio_data)
		{
			vorbis_block_clear (&tf->vb);
			vorbis_dsp_clear (&tf->vd);
			vorbis_comment_clear (&tf->vc);
			vorbis_info_clear (&tf->vi);
		}
		if (asset->video_data)
		{
			theora_info_clear (&tf->ti);
			ogg_stream_clear (&tf->to);
			theora_clear (&tf->td);

		}
		ogg_stream_clear (&tf->vo);
		
		if (stream) fclose(stream);
		stream = 0;
	}	
}

int FileOGG::close_file_derived()
{
//printf("FileOGG::close_file_derived(): 1\n");
	if (stream) fclose(stream);
	stream = 0;
}

int64_t FileOGG::get_video_position()
{
	return 0;
}

int64_t FileOGG::get_audio_position()
{
	return 0;
}

int FileOGG::set_video_position(int64_t x)
{
	return 1;
}

int FileOGG::set_audio_position(int64_t x)
{
	return 1;
}

int FileOGG::flush_ogg(int e_o_s)
{
	/* flush out the ogg pages to stream */

	int flushloop=1;

	while(flushloop)
	{
		int video = -1;
		flushloop=0;
		//some debuging 
		//fprintf(stderr,"\ndiff: %f\n",info->audiotime-info->videotime);
		while(asset->video_data && (e_o_s || 
			((tf->videotime <= tf->audiotime || !asset->video_data) && tf->videoflag == 1)))
		{
		    
			tf->videoflag = 0;
			while(ogg_stream_pageout (&tf->to, &tf->videopage) > 0)
			{
				tf->videotime=
					theora_granule_time (&tf->td, ogg_page_granulepos(&tf->videopage));
				/* flush a video page */
				tf->video_bytesout +=
					fwrite (tf->videopage.header, 1, tf->videopage.header_len, stream);
				tf->video_bytesout +=
					fwrite (tf->videopage.body, 1, tf->videopage.body_len, stream);
	
	//		    tf->vkbps = rint (tf->video_bytesout * 8. / tf->videotime * .001);

	//		    print_stats(info, tf->videotime);
				video=1;
				tf->videoflag = 1;
				flushloop=1;
			}
			if(e_o_s)
				break;
		}

		while (asset->audio_data && (e_o_s || 
			((tf->audiotime < tf->videotime || !asset->video_data) && tf->audioflag==1)))
		{
	
			tf->audioflag = 0;
			while(ogg_stream_pageout (&tf->vo, &tf->audiopage) > 0){    
				/* flush an audio page */
				tf->audiotime=
					vorbis_granule_time (&tf->vd, ogg_page_granulepos(&tf->audiopage));
				tf->audio_bytesout +=
					fwrite (tf->audiopage.header, 1, tf->audiopage.header_len, stream);
				tf->audio_bytesout +=
					fwrite (tf->audiopage.body, 1, tf->audiopage.body_len, stream);

	//		    tf->akbps = rint (tf->audio_bytesout * 8. / tf->audiotime * .001);
	//		    print_stats(info, tf->audiotime);
				video=0;
				tf->audioflag = 1;
				flushloop=1;
			}
			if(e_o_s)
				break;
		}
	}
	return 0;
}

int FileOGG::write_samples_vorbis(double **buffer, int64_t len, int e_o_s)
{
	int i,j, count = 0;
	float **vorbis_buffer;
	if(e_o_s)
	{
		vorbis_analysis_wrote (&tf->vd, 0);
	} else
	{
		vorbis_buffer = vorbis_analysis_buffer (&tf->vd, len);
		/* uninterleave samples */
		for(i = 0; i<asset->channels; i++){
			for (j = 0; j < len; j++){
				vorbis_buffer[i][j] = buffer[i][j];
			}
		}
		vorbis_analysis_wrote (&tf->vd, len);
	}
	while(vorbis_analysis_blockout (&tf->vd, &tf->vb) == 1){
	    
	    /* analysis, assume we want to use bitrate management */
	    vorbis_analysis (&tf->vb, NULL);
	    vorbis_bitrate_addblock (&tf->vb);
	    
	    /* weld packets into the bitstream */
	    while (vorbis_bitrate_flushpacket (&tf->vd, &tf->op)){
		ogg_stream_packetin (&tf->vo, &tf->op);
	    }

	}
	tf->audioflag=1;
	flush_ogg(0);
	return 0;


}

int FileOGG::write_samples(double **buffer, int64_t len)
{
	if (len > 0)
		return write_samples_vorbis(buffer, len, 0);
	return 0;
}

int FileOGG::write_frames_theora(VFrame ***frames, int len, int e_o_s)
{
	// due to clumsy theora's design we need to delay writing out by one frame
	// always stay one frame behind, so we can correctly encode e_o_s

	int i, j, result = 0;

	if(!stream) return 0;

	
	for(j = 0; j < len && !result; j++)
	{
		if (temp_frame) // encode previous frame if available
		{
			yuv_buffer yuv;
			yuv.y_width = tf->ti.width;
			yuv.y_height = tf->ti.height;
			yuv.y_stride = temp_frame->get_bytes_per_line();

			yuv.uv_width = tf->ti.width / 2;
			yuv.uv_height = tf->ti.height / 2;
			yuv.uv_stride = temp_frame->get_bytes_per_line() /2;

			yuv.y = temp_frame->get_y();
			yuv.u = temp_frame->get_u();
			yuv.v = temp_frame->get_v();
			int ret = theora_encode_YUVin (&tf->td, &yuv);
			if (ret)
			{
				printf("FileOGG: theora_encode_YUVin failed with code %i\n", ret);
				printf("yuv_buffer: y_width: %i, y_height: %i, y_stride: %i, uv_width: %i, uv_height: %i, uv_stride: %i\n",
					yuv.y_width,
					yuv.y_height,
					yuv.y_stride,
					yuv.uv_width,
					yuv.uv_height,
					yuv.uv_stride);
			}
			theora_encode_packetout (&tf->td, e_o_s, &tf->op); 
			ogg_stream_packetin (&tf->to, &tf->op);
			tf->videoflag=1;
			flush_ogg(0);  // eos flush is done later at close_file
		}
// If we have e_o_s, don't encode any new frames
		if (e_o_s) 
			break;

		if (!temp_frame)
		{
			temp_frame = new VFrame (0, 
						tf->ti.width, 
						tf->ti.height,
						BC_YUV420P);
		} 
		VFrame *frame = frames[0][j];
		int in_color_model = frame->get_color_model();
		if (in_color_model == BC_YUV422P &&
		    temp_frame->get_w() == frame->get_w() &&
		    temp_frame->get_h() == frame->get_h() &&
		    temp_frame->get_bytes_per_line() == frame->get_bytes_per_line())
		{
			temp_frame->copy_from(frame);
		} else
		{

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
				frame->get_w(),
				frame->get_h(),
				0,
				0,
				frame->get_w(),  // temp_frame can be larger than frame if width not dividable by 16
				frame->get_h(),	 // CHECK: does cmodel_transfer to 420 work in above situation ?
				frame->get_color_model(),
				BC_YUV420P,
				0,
				frame->get_w(),
				temp_frame->get_w());

		}
	}						
				
	return 0;
}


int FileOGG::write_frames(VFrame ***frames, int len)
{
	
	return write_frames_theora(frames, len, 0);
}

int FileOGG::colormodel_supported(int colormodel)
{
	return colormodel;
}


int FileOGG::get_best_colormodel(Asset *asset, int driver)
{
	switch(driver)
	{
		case PLAYBACK_X11:
			return BC_RGB888;
			break;
		case PLAYBACK_X11_XV:
			return BC_YUV422;
			break;
		case PLAYBACK_DV1394:
		case PLAYBACK_FIREWIRE:
			return BC_COMPRESSED;
			break;
		case PLAYBACK_LML:
		case PLAYBACK_BUZ:
			return BC_YUV422P;
			break;
		case VIDEO4LINUX:
		case VIDEO4LINUX2:
		case CAPTURE_BUZ:
		case CAPTURE_LML:
		case VIDEO4LINUX2JPEG:
			return BC_YUV422;
			break;
		case CAPTURE_FIREWIRE:
			return BC_COMPRESSED;
			break;
	}
	return BC_RGB888;
}


OGGConfigAudio::OGGConfigAudio(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Audio Compression",
	parent_window->get_abs_cursor_x(1),
	parent_window->get_abs_cursor_y(1),
	350,
	250)
{
	this->parent_window = parent_window;
	this->asset = asset;
}

OGGConfigAudio::~OGGConfigAudio()
{

}

int OGGConfigAudio::create_objects()
{
//	add_tool(new BC_Title(10, 10, _("There are no audio options for this format")));

	int x = 10, y = 10;
	int x1 = 150;
	char string[BCTEXTLEN];

	add_tool(fixed_bitrate = new OGGVorbisFixedBitrate(x, y, this));
	add_tool(variable_bitrate = new OGGVorbisVariableBitrate(x1, y, this));

	y += 30;
	sprintf(string, "%d", asset->vorbis_min_bitrate);
	add_tool(new BC_Title(x, y, _("Min bitrate:")));
	add_tool(new OGGVorbisMinBitrate(x1, y, this, string));

	y += 30;
	add_tool(new BC_Title(x, y, _("Avg bitrate:")));
	sprintf(string, "%d", asset->vorbis_bitrate);
	add_tool(new OGGVorbisAvgBitrate(x1, y, this, string));

	y += 30;
	add_tool(new BC_Title(x, y, _("Max bitrate:")));
	sprintf(string, "%d", asset->vorbis_max_bitrate);
	add_tool(new OGGVorbisMaxBitrate(x1, y, this, string));


	add_subwindow(new BC_OKButton(this));
	show_window();
	flush();
	return 0;



}

int OGGConfigAudio::close_event()
{
	set_done(0);
	return 1;
}

OGGVorbisFixedBitrate::OGGVorbisFixedBitrate(int x, int y, OGGConfigAudio *gui)
 : BC_Radial(x, y, !gui->asset->vorbis_vbr, _("Average bitrate"))
{
	this->gui = gui;
}
int OGGVorbisFixedBitrate::handle_event()
{
	gui->asset->vorbis_vbr = 0;
	gui->variable_bitrate->update(0);
	return 1;
}

OGGVorbisVariableBitrate::OGGVorbisVariableBitrate(int x, int y, OGGConfigAudio *gui)
 : BC_Radial(x, y, gui->asset->vorbis_vbr, _("Variable bitrate"))
{
	this->gui = gui;
}
int OGGVorbisVariableBitrate::handle_event()
{
	gui->asset->vorbis_vbr = 1;
	gui->fixed_bitrate->update(0);
	return 1;
}


OGGVorbisMinBitrate::OGGVorbisMinBitrate(int x, 
	int y, 
	OGGConfigAudio *gui, 
	char *text)
 : BC_TextBox(x, y, 180, 1, text)
{
	this->gui = gui;
}
int OGGVorbisMinBitrate::handle_event()
{
	gui->asset->vorbis_min_bitrate = atol(get_text());
	return 1;
}



OGGVorbisMaxBitrate::OGGVorbisMaxBitrate(int x, 
	int y, 
	OGGConfigAudio *gui,
	char *text)
 : BC_TextBox(x, y, 180, 1, text)
{
	this->gui = gui;
}
int OGGVorbisMaxBitrate::handle_event()
{
	gui->asset->vorbis_max_bitrate = atol(get_text());
	return 1;
}



OGGVorbisAvgBitrate::OGGVorbisAvgBitrate(int x, int y, OGGConfigAudio *gui, char *text)
 : BC_TextBox(x, y, 180, 1, text)
{
	this->gui = gui;
}
int OGGVorbisAvgBitrate::handle_event()
{
	gui->asset->vorbis_bitrate = atol(get_text());
	return 1;
}





OGGConfigVideo::OGGConfigVideo(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Video Compression",
	parent_window->get_abs_cursor_x(1),
	parent_window->get_abs_cursor_y(1),
	450,
	220)
{
	this->parent_window = parent_window;
	this->asset = asset;
}

OGGConfigVideo::~OGGConfigVideo()
{

}

int OGGConfigVideo::create_objects()
{
//	add_tool(new BC_Title(10, 10, _("There are no video options for this format")));
	int x = 10, y = 10;
	int x1 = x + 150;
	int x2 = x + 300;

	add_subwindow(new BC_Title(x, y + 5, _("Bitrate:")));
	add_subwindow(new OGGTheoraBitrate(x1, y, this));
	add_subwindow(fixed_bitrate = new OGGTheoraFixedBitrate(x2, y, this));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Quality:")));
	add_subwindow(new BC_ISlider(x + 80, 
		y,
		0,
		200,
		200,
		0,
		63,
		asset->theora_quality,
		0,
		0,
		&asset->theora_quality));

	
	add_subwindow(fixed_quality = new OGGTheoraFixedQuality(x2, y, this));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Keyframe frequency:")));
	OGGTheoraKeyframeFrequency *keyframe_frequency = 
		new OGGTheoraKeyframeFrequency(x1 + 60, y, this);
	keyframe_frequency->create_objects();
	y += 30;
	
	add_subwindow(new BC_Title(x, y, _("Keyframe force frequency:")));
	OGGTheoraKeyframeForceFrequency *keyframe_force_frequency = 
		new OGGTheoraKeyframeForceFrequency(x1 + 60, y, this);
	keyframe_force_frequency->create_objects();
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Sharpness:")));
	OGGTheoraSharpness *sharpness = 
		new OGGTheoraSharpness(x1 + 60, y, this);
	sharpness->create_objects();
	y += 30;
	

	add_subwindow(new BC_OKButton(this));
	return 0;
}




int OGGConfigVideo::close_event()
{
	set_done(0);
	return 1;
}

OGGTheoraBitrate::OGGTheoraBitrate(int x, int y, OGGConfigVideo *gui)
 : BC_TextBox(x, y, 100, 1, gui->asset->theora_bitrate)
{
	this->gui = gui;
}


int OGGTheoraBitrate::handle_event()
{
	// TODO: MIN / MAX check
	gui->asset->theora_bitrate = atol(get_text());
	return 1;
};




OGGTheoraFixedBitrate::OGGTheoraFixedBitrate(int x, int y, OGGConfigVideo *gui)
 : BC_Radial(x, y, gui->asset->theora_fix_bitrate, _("Fixed bitrate"))
{
	this->gui = gui;
}

int OGGTheoraFixedBitrate::handle_event()
{
	update(1);
	gui->asset->theora_fix_bitrate = 1;
	gui->fixed_quality->update(0);
	return 1;
};

OGGTheoraFixedQuality::OGGTheoraFixedQuality(int x, int y, OGGConfigVideo *gui)
 : BC_Radial(x, y, !gui->asset->theora_fix_bitrate, _("Fixed quality"))
{
	this->gui = gui;
}

int OGGTheoraFixedQuality::handle_event()
{
	update(1);
	gui->asset->theora_fix_bitrate = 0;
	gui->fixed_bitrate->update(0);
	return 1;
};

OGGTheoraKeyframeFrequency::OGGTheoraKeyframeFrequency(int x, int y, OGGConfigVideo *gui)
 : BC_TumbleTextBox(gui, 
 	(int64_t)gui->asset->theora_keyframe_frequency, 
	(int64_t)1,
	(int64_t)500,
	x, 
	y,
	40)
{
	this->gui = gui;
}

int OGGTheoraKeyframeFrequency::handle_event()
{
	gui->asset->theora_keyframe_frequency = atol(get_text());
	return 1;
}

OGGTheoraKeyframeForceFrequency::OGGTheoraKeyframeForceFrequency(int x, int y, OGGConfigVideo *gui)
 : BC_TumbleTextBox(gui, 
 	(int64_t)gui->asset->theora_keyframe_frequency, 
	(int64_t)1,
	(int64_t)500,
	x, 
	y,
	40)
{
	this->gui = gui;
}

int OGGTheoraKeyframeForceFrequency::handle_event()
{
	gui->asset->theora_keyframe_frequency = atol(get_text());
	return 1;
}


OGGTheoraSharpness::OGGTheoraSharpness(int x, int y, OGGConfigVideo *gui)
 : BC_TumbleTextBox(gui, 
 	(int64_t)gui->asset->theora_sharpness, 
	(int64_t)0,
	(int64_t)2,
	x, 
	y,
	40)
{
	this->gui = gui;
}

int OGGTheoraSharpness::handle_event()
{
	gui->asset->theora_sharpness = atol(get_text());
	return 1;
}


