#include "assetedit.h"
#include "assets.h"
#include "awindow.h"
#include "awindowgui.h"
#include "bcprogressbox.h"
#include "bitspopup.h"
#include "cache.h"
#include "cplayback.h"
#include "cwindow.h"
#include "file.h"
#include "filesystem.h"
#include "indexfile.h"
#include "mainindexes.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "new.h"
#include "transportque.h"

#include <string.h>

AssetEdit::AssetEdit(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
	asset = 0;
	window = 0;
	set_synchronous(0);
}


AssetEdit::~AssetEdit()
{
}


void AssetEdit::edit_asset(Asset *asset)
{
	if(asset)
	{
// Allow more than one window
		this->asset = asset;
		Thread::start();
	}
}


int AssetEdit::set_asset(Asset *asset)
{
	this->asset = asset;
	return 0;
}

void AssetEdit::run()
{
	if(asset)
	{
		new_asset = new Asset(asset->path);
		*new_asset = *asset;
		int result = 0;

		window = new AssetEditWindow(mwindow, this);
		window->create_objects();
		result = window->run_window();

 		if(!result)
 		{
 			if(!asset->equivalent(*new_asset, 1, 1))
 			{
//printf("AssetEdit::run 1\n");
				*asset = *new_asset;
				mwindow->gui->lock_window();
				mwindow->gui->update(0,
					2,
					0,
					0,
					0, 
					0,
					0);
				if(asset->audio_data)
				{
					asset->index_status = INDEX_NOTTESTED;
					mwindow->mainindexes->add_next_asset(asset);
					mwindow->mainindexes->start_build();
				}
				mwindow->gui->unlock_window();
//printf("AssetEdit::run 2\n");


				mwindow->awindow->gui->lock_window();
				mwindow->awindow->gui->update_assets();
				mwindow->awindow->gui->unlock_window();

				mwindow->restart_brender();
				mwindow->sync_parameters(CHANGE_ALL);
//printf("AssetEdit::run 3\n");
 			}
 		}

		delete new_asset;
		delete window;
		window = 0;
	}
}








AssetEditWindow::AssetEditWindow(MWindow *mwindow, AssetEdit *asset_edit)
 : BC_Window(PROGRAM_NAME ": Asset Info", 
 	mwindow->gui->get_abs_cursor_x() - 400 / 2, 
	mwindow->gui->get_abs_cursor_y() - 500 / 2, 
	400, 
	500,
	400,
	500,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->asset_edit = asset_edit;
	this->asset = asset_edit->new_asset;
	bitspopup = 0;
	if(asset->format == FILE_PCM)
		allow_edits = 1;
	else
		allow_edits = 0;
}





AssetEditWindow::~AssetEditWindow()
{
	if(bitspopup) delete bitspopup;
}

int AssetEditWindow::create_objects()
{
	int y = 10, x = 10, x1 = 10, x2 = 150;
	char string[1024];
	int vmargin;
	int hmargin1 = 180, hmargin2 = 290;
	FileSystem fs;

//printf("AssetEditWindow::create_objects 1\n");
	if(allow_edits) 
		vmargin = 30;
	else
		vmargin = 20;
//printf("AssetEditWindow::create_objects 1\n");

	add_subwindow(path_text = new AssetEditPathText(this, y));
	add_subwindow(path_button = new AssetEditPath(mwindow, 
		this, 
		path_text, 
		y, 
		asset->path, 
		PROGRAM_NAME ": Asset path", "Select a file for this asset:"));
	y += 30;
//printf("AssetEditWindow::create_objects 1\n");

	add_subwindow(new BC_Title(x, y, "File format:"));
	x = x2;
	File file;
	add_subwindow(new BC_Title(x, y, file.formattostr(mwindow->plugindb, asset->format), MEDIUMFONT, YELLOW));
//printf("AssetEditWindow::create_objects 1\n");
	x = x1;
	y += 20;

	add_subwindow(new BC_Title(x, y, "Bytes:"));
	sprintf(string, "%lld", fs.get_size(asset->path));
// Do commas
	int len = strlen(string);
	int commas = (len - 1) / 3;
	for(int i = len + commas, j = len, k; j >= 0 && i >= 0; i--, j--)
	{
		k = (len - j - 1) / 3;
		if(k * 3 == len - j - 1 && j != len - 1 && string[j] != 0)
		{
			string[i--] = ',';
		}

		string[i] = string[j];
	}
//printf("AssetEditWindow::create_objects 1\n");

	x = x2;
	add_subwindow(new BC_Title(x, y, string, MEDIUMFONT, YELLOW));
	y += 30;
	x = x1;
//printf("AssetEditWindow::create_objects 1\n");

	if(asset->audio_data)
	{
		add_subwindow(new BC_Title(x, y, "Audio:", LARGEFONT, RED));
//printf("AssetEditWindow::create_objects 1\n");

		y += 30;

		if(asset->acodec[0])
		{
			add_subwindow(new BC_Title(x, y, "Compression:"));
			sprintf(string, "%c%c%c%c", 
				asset->acodec[0], 
				asset->acodec[1], 
				asset->acodec[2], 
				asset->acodec[3]);
			x = x2;
			add_subwindow(new BC_Title(x, y, string, MEDIUMFONT, YELLOW));
			y += vmargin;
			x = x1;
		}

		add_subwindow(new BC_Title(x, y, "Channels:"));
		sprintf(string, "%d", asset->channels);
//printf("AssetEditWindow::create_objects 1\n");

		x = x2;
		if(allow_edits)
		{
			BC_TumbleTextBox *textbox = new AssetEditChannels(this, string, x, y);
			textbox->create_objects();
			y += vmargin;
		}
		else
		{
			add_subwindow(new BC_Title(x, y, string, MEDIUMFONT, YELLOW));
			y += 20;
		}
//printf("AssetEditWindow::create_objects 1\n");

		x = x1;
		add_subwindow(new BC_Title(x, y, "Sample rate:"));
		sprintf(string, "%d", asset->sample_rate);
//printf("AssetEditWindow::create_objects 1\n");

		x = x2;
//		if(allow_edits)
		if(1)
		{
			BC_TextBox *textbox;
			add_subwindow(textbox = new AssetEditRate(this, string, x, y));
			x += textbox->get_w();
			add_subwindow(new SampleRatePulldown(mwindow, textbox, x, y));
		}
		else
		{
			add_subwindow(new BC_Title(x, y, string, MEDIUMFONT, YELLOW));
		}
//printf("AssetEditWindow::create_objects 1\n");

		y += 30;
		x = x1;

		add_subwindow(new BC_Title(x, y, "Bits:"));
		x = x2;
		if(allow_edits)
		{
			bitspopup = new BitsPopup(this, 
				x, 
				y, 
				&asset->bits, 
				1, 
				1, 
				1,
				0,
				1);
			bitspopup->create_objects();
		}
		else
			add_subwindow(new BC_Title(x, y, File::bitstostr(asset->bits), MEDIUMFONT, YELLOW));
//printf("AssetEditWindow::create_objects 1\n");


		x = x1;
		y += vmargin;
		add_subwindow(new BC_Title(x, y, "Header length:"));
		sprintf(string, "%d", asset->header);
//printf("AssetEditWindow::create_objects 1\n");

		x = x2;
		if(allow_edits)
			add_subwindow(new AssetEditHeader(this, string, x, y));
		else
			add_subwindow(new BC_Title(x, y, string, MEDIUMFONT, YELLOW));
//printf("AssetEditWindow::create_objects 1\n");

		y += vmargin;
		x = x1;

		add_subwindow(new BC_Title(x, y, "Byte order:"));
//printf("AssetEditWindow::create_objects 1\n");

		if(allow_edits)
		{
			x = x2;
			add_subwindow(lohi = new AssetEditByteOrderLOHI(this, asset->byte_order, x, y));
			x += 70;
			add_subwindow(hilo = new AssetEditByteOrderHILO(this, asset->byte_order ^ 1, x, y));
			y += vmargin;
		}
		else
		{
			x = x2;
			if(asset->byte_order)
				add_subwindow(new BC_Title(x, y, "Lo-Hi", MEDIUMFONT, YELLOW));
			else
				add_subwindow(new BC_Title(x, y, "Hi-Lo", MEDIUMFONT, YELLOW));
			y += vmargin;
		}

//printf("AssetEditWindow::create_objects 1\n");

		x = x1;
		if(allow_edits)
		{
//			add_subwindow(new BC_Title(x, y, "Values are signed:"));
			add_subwindow(new AssetEditSigned(this, asset->signed_, x, y));
		}
		else
		{
			if(!asset->signed_ && asset->bits == 8)
				add_subwindow(new BC_Title(x, y, "Values are unsigned"));
			else
				add_subwindow(new BC_Title(x, y, "Values are signed"));
		}
//printf("AssetEditWindow::create_objects 2\n");

		y += 30;
	}
//printf("AssetEditWindow::create_objects 3\n");

	x = x1;
	if(asset->video_data)
	{
		add_subwindow(new BC_Title(x, y, "Video:", LARGEFONT, RED));

		y += 30;
		x = x1;
		if(asset->vcodec[0])
		{
			add_subwindow(new BC_Title(x, y, "Compression:"));
			sprintf(string, "%c%c%c%c", 
				asset->vcodec[0], 
				asset->vcodec[1], 
				asset->vcodec[2], 
				asset->vcodec[3]);
			x = x2;
			add_subwindow(new BC_Title(x, y, string, MEDIUMFONT, YELLOW));
			y += vmargin;
			x = x1;
		}

		add_subwindow(new BC_Title(x, y, "Frame rate:"));
		x = x2;
		sprintf(string, "%.2f", asset->frame_rate);
		BC_TextBox *framerate;
		add_subwindow(framerate = new AssetEditFRate(this, string, x, y));
		x += 105;
		add_subwindow(new FrameRatePulldown(mwindow, framerate, x, y));
		
		y += 30;
		x = x1;
		add_subwindow(new BC_Title(x, y, "Width:"));
		x = x2;
		sprintf(string, "%d", asset->width);
		add_subwindow(new BC_Title(x, y, string, MEDIUMFONT, YELLOW));
		
		y += vmargin;
		x = x1;
		add_subwindow(new BC_Title(x, y, "Height:"));
		x = x2;
		sprintf(string, "%d", asset->height);
		add_subwindow(new BC_Title(x, y, string, MEDIUMFONT, YELLOW));
		y += 30;
	}

//printf("AssetEditWindow::create_objects 4\n");
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
//printf("AssetEditWindow::create_objects 5\n");
	show_window();
	flush();
	return 0;
}

AssetEditChannels::AssetEditChannels(AssetEditWindow *fwindow, 
	char *text, 
	int x,
	int y)
 : BC_TumbleTextBox(fwindow, 
		(long)atol(text),
		(long)1,
		(long)MAXCHANNELS,
		x, 
		y, 
		50)
{
	this->fwindow = fwindow;
}

int AssetEditChannels::handle_event()
{
	fwindow->asset->channels = atol(get_text());
	return 1;
}

AssetEditRate::AssetEditRate(AssetEditWindow *fwindow, char *text, int x, int y)
 : BC_TextBox(x, y, 100, 1, text)
{
	this->fwindow = fwindow;
}

int AssetEditRate::handle_event()
{
	fwindow->asset->sample_rate = atol(get_text());
	return 1;
}

AssetEditFRate::AssetEditFRate(AssetEditWindow *fwindow, char *text, int x, int y)
 : BC_TextBox(x, y, 100, 1, text)
{
	this->fwindow = fwindow;
}

int AssetEditFRate::handle_event()
{
	fwindow->asset->frame_rate = atof(get_text());
	return 1;
}

AssetEditHeader::AssetEditHeader(AssetEditWindow *fwindow, char *text, int x, int y)
 : BC_TextBox(x, y, 100, 1, text)
{
	this->fwindow = fwindow;
}

int AssetEditHeader::handle_event()
{
	fwindow->asset->header = atol(get_text());
	return 1;
}

AssetEditByteOrderLOHI::AssetEditByteOrderLOHI(AssetEditWindow *fwindow, 
	int value, 
	int x,
	int y)
 : BC_Radial(x, y, value, "Lo-Hi")
{
	this->fwindow = fwindow;
}

int AssetEditByteOrderLOHI::handle_event()
{
	fwindow->asset->byte_order = get_value();
	fwindow->hilo->update(get_value() ^ 1);
	return 1;
}

AssetEditByteOrderHILO::AssetEditByteOrderHILO(AssetEditWindow *fwindow, 
	int value, 
	int x, 
	int y)
 : BC_Radial(x, y, value, "Hi-Lo")
{
	this->fwindow = fwindow;
}

int AssetEditByteOrderHILO::handle_event()
{
	fwindow->asset->byte_order = get_value() ^ 1;
	fwindow->lohi->update(get_value() ^ 1);
	return 1;
}

AssetEditSigned::AssetEditSigned(AssetEditWindow *fwindow, 
	int value, 
	int x, 
	int y)
 : BC_CheckBox(x, y, value, "Values are signed")
{
	this->fwindow = fwindow;
}

int AssetEditSigned::handle_event()
{
	fwindow->asset->signed_ = get_value();
	return 1;
}







AssetEditPathText::AssetEditPathText(AssetEditWindow *fwindow, int y)
 : BC_TextBox(5, y, 300, 1, fwindow->asset->path) 
{
	this->fwindow = fwindow; 
}
AssetEditPathText::~AssetEditPathText() 
{
}
int AssetEditPathText::handle_event() 
{
	strcpy(fwindow->asset->path, get_text());
	return 1;
}

AssetEditPath::AssetEditPath(MWindow *mwindow, AssetEditWindow *fwindow, BC_TextBox *textbox, int y, char *text, char *window_title, char *window_caption)
 : BrowseButton(mwindow, fwindow, textbox, 310, y, text, window_title, window_caption, 0) 
{ 
	this->fwindow = fwindow; 
}
AssetEditPath::~AssetEditPath() {}






AssetEditFormat::AssetEditFormat(AssetEditWindow *fwindow, char* default_, int y)
 : FormatPopup(fwindow->mwindow->plugindb, 90, y)
{ 
	this->fwindow = fwindow; 
}
AssetEditFormat::~AssetEditFormat() 
{
}
int AssetEditFormat::handle_event()
{
	fwindow->asset->format = File::strtoformat(fwindow->mwindow->plugindb, get_selection(0, 0)->get_text());
	return 1;
}

