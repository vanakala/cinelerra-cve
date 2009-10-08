
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

#include "filexml.h"
#include "mwindow.h"
#include "module.h"
#include "modules.h"
#include "mwindowgui.h"
#include "patch.h"
#include "patchbay.h"
#include "mainsession.h"
#include "theme.h"
#include <string.h>

Patch::Patch(MWindow *mwindow, PatchBay *patchbay, int data_type) : ListItem<Patch>()
{
	this->mwindow = mwindow;
	this->patches = patchbay;
	this->data_type = data_type;
	record = play = automate = draw = 1;
	title[0] = 0;
}

Patch::~Patch()
{
	if(mwindow->gui)
	{
		delete recordpatch;  
		delete playpatch;  
		delete autopatch;  
		delete drawpatch;
		delete title_text;  
	}
}

int Patch::create_objects(char *text, int pixel)
{
	int x, y;
	this->pixel = pixel;
	strcpy(title, text);
	
	if(mwindow->gui)
	{
		if(mwindow->session->tracks_vertical)
		{
			//x = patches->gui->w - pixel - mwindow->zoom_track;
			x = pixel + 3;
			y = 0;
		}
		else
		{
			x = 3;
			y = pixel;
		}

 		patches->add_subwindow(recordpatch = new RecordPatchOld(mwindow, this, x, y));
		patches->add_subwindow(playpatch = new PlayPatchOld(mwindow, this, x, y));
		patches->add_subwindow(title_text = new TitlePatchOld(mwindow, this, text, x, y));
		//patches->add_subwindow(autotitle = new BC_Title(x + PATCH_AUTO_TITLE, y + PATCH_ROW2, "A", SMALLFONT));
		//patches->add_subwindow(autopatch = new AutoPatchOld(mwindow, this, x, y));
		//patches->add_subwindow(drawtitle = new BC_Title(x + PATCH_DRAW_TITLE, y + PATCH_ROW2, "D", SMALLFONT));
		patches->add_subwindow(drawpatch = new DrawPatchOld(mwindow, this, x, y));
	}
	return 0;
}

int Patch::save(FileXML *xml)
{
	xml->tag.set_title("PATCH");
	xml->append_tag();

	if(play)
	{
		xml->tag.set_title("PLAY");
		xml->append_tag();
		xml->tag.set_title("/PLAY");
		xml->append_tag();
	}
	
	if(record)  
	{
		xml->tag.set_title("RECORD");
		xml->append_tag();
		xml->tag.set_title("/RECORD");
		xml->append_tag();
	}
	
	if(automate)  
	{
		xml->tag.set_title("AUTO");
		xml->append_tag();
		xml->tag.set_title("/AUTO");
		xml->append_tag();
	}
	
	if(draw)  
	{
		xml->tag.set_title("DRAW");
		xml->append_tag();
		xml->tag.set_title("/DRAW");
		xml->append_tag();
	}
	
	xml->tag.set_title("TITLE");
	xml->append_tag();
	xml->append_text(title);
	xml->tag.set_title("/TITLE");
	xml->append_tag();
	
	
	xml->tag.set_title("/PATCH");
	xml->append_tag();
	xml->append_newline();
	return 0;
}

int Patch::load(FileXML *xml)
{
	int result = 0;	
	play = record = automate = draw = 0;    // defaults
	
	do{
		result = xml->read_tag();
		
		if(!result)
		{
			if(xml->tag.title_is("/PATCH"))
			{
				result = 1;
			}
			else
			if(xml->tag.title_is("PLAY"))
			{
				play = 1;
			}
			else
			if(xml->tag.title_is("RECORD"))
			{
				record = 1;
			}
			else
			if(xml->tag.title_is("AUTO"))
			{
				automate = 1;
			}
			else
			if(xml->tag.title_is("DRAW"))
			{
				draw = 1;
			}
			else
			if(xml->tag.title_is("TITLE"))
			{
				strcpy(title, xml->read_text());
			}
		}
	}while(!result);
	
	if(mwindow->gui)
	{
		playpatch->update(play);
		recordpatch->update(record);
		autopatch->update(automate);
		drawpatch->update(draw);
		title_text->update(title);
	}
	return 0;
}

int Patch::set_pixel(int pixel)
{         // must be top of track for track zoom
	this->pixel = pixel;
	if(mwindow->gui)
	{
		if(mwindow->session->tracks_vertical)
		{
			pixel += 3;
			playpatch->reposition_window(pixel + PATCH_PLAY, playpatch->get_y());
			recordpatch->reposition_window(pixel + PATCH_REC, recordpatch->get_y());
			autopatch->reposition_window(pixel + PATCH_AUTO, autopatch->get_y());
			title_text->reposition_window(pixel, title_text->get_y());
			drawpatch->reposition_window(pixel + PATCH_DRAW, drawpatch->get_y());
		}
		else
		{
			playpatch->reposition_window(playpatch->get_x(), pixel + PATCH_ROW2);
			recordpatch->reposition_window(recordpatch->get_x(), pixel + PATCH_ROW2);
			autopatch->reposition_window(autopatch->get_x(), pixel + PATCH_ROW2);
			drawpatch->reposition_window(drawpatch->get_x(), pixel + PATCH_ROW2);
			title_text->reposition_window(title_text->get_x(), pixel + 3);
		}
	}
	return 0;
}

int Patch::set_title(char *new_title)
{
	strcpy(title, new_title);
	title_text->update(new_title);
	return 0;
}

int Patch::flip_vertical()
{
	if(mwindow->gui)
	{
		if(mwindow->session->tracks_vertical)
		{
			playpatch->reposition_window(playpatch->get_x(), PATCH_ROW2);
			recordpatch->reposition_window(recordpatch->get_x(), PATCH_ROW2);
			autopatch->reposition_window(autopatch->get_x(), PATCH_ROW2);
			drawpatch->reposition_window(drawpatch->get_x(), PATCH_ROW2);
			title_text->reposition_window(title_text->get_x(), 3);
		}
		else
		{
			playpatch->reposition_window(PATCH_PLAY, playpatch->get_y());
			recordpatch->reposition_window(PATCH_REC, recordpatch->get_y());
			autopatch->reposition_window(PATCH_AUTO, autopatch->get_y());
			drawpatch->reposition_window(PATCH_DRAW, drawpatch->get_y());
			title_text->reposition_window(PATCH_TITLE, title_text->get_y());
		}
		set_pixel(pixel);
	}
	return 0;
}


int Patch::pixelmovement(int distance)
{
	if(mwindow->gui)
	{
		pixel -= distance;
		set_pixel(pixel);
	}
	return 0;
}

Module* Patch::get_module()    // return corresponding module from console
{
//	return mwindow->console->modules->module_number(patches->number_of(this));
}

PlayPatchOld::PlayPatchOld(MWindow *mwindow, Patch *patch, int x, int y)
 : BC_Toggle(x + PATCH_PLAY, 
 	y + PATCH_ROW2, 
	mwindow->theme->playpatch_data,
	1,
	"",
	1,
	0,
	0)
{
	this->patch = patch; 
	patches = patch->patches; 
}


int PlayPatchOld::handle_event()
{
// get the total selected before this event
	if(shift_down())
	{
		int total_selected = patches->plays_selected();

		if(total_selected == 0)
		{
// nothing previously selected
			patches->select_all_play();
		}
		else
		if(total_selected == 1)
		{
			if(patch->play)
			{
// this patch was previously the only one on
				patches->select_all_play();
			}
			else
			{
// another patch was previously the only one on
				patches->deselect_all_play();
				patch->play = 1;
			}
		}
		else
		if(total_selected > 1)
		{
			patches->deselect_all_play();
			patch->play = 1;
		}
		
		update(patch->play);
	}
	else
	{
		patch->play = get_value();
	}
	patches->button_down = 1;
	patches->reconfigure_trigger = 1;
	patches->new_status = get_value();
	return 1;
}

int PlayPatchOld::button_release()
{
	return 0;
}

int PlayPatchOld::cursor_moved_over()
{
	if(patches->button_down && patches->new_status != get_value())
	{
		update(patches->new_status);
		patch->play = get_value();
		return 1;
	}
	return 0;
}

RecordPatchOld::RecordPatchOld(MWindow *mwindow, Patch *patch, int x, int y)
 : BC_Toggle(x + PATCH_REC, 
 	y + PATCH_ROW2, 
	mwindow->theme->recordpatch_data,
	1,
	"",
	1,
	0,
	0)
{ 
	this->patch = patch; 
	patches = patch->patches; 
}

int RecordPatchOld::handle_event()
{
// get the total selected before this event
	if(shift_down())
	{
		int total_selected = patches->records_selected();

		if(total_selected == 0)
		{
// nothing previously selected
			patches->select_all_record();
		}
		else
		if(total_selected == 1)
		{
			if(patch->record)
			{
// this patch was previously the only one on
				patches->select_all_record();
			}
			else
			{
// another patch was previously the only one on
				patches->deselect_all_record();
				patch->record = 1;
			}
		}
		else
		if(total_selected > 1)
		{
			patches->deselect_all_record();
			patch->record = 1;
		}
		
		update(patch->record);
	}
	else
	{
		patch->record = get_value();
	}
	patches->button_down = 1;
	patches->new_status = get_value();
	return 1;
}

int RecordPatchOld::button_release()
{
	//if(patches->button_down)
	//{
	//	patches->button_down = 0;
// restart the playback
		//patches->mwindow->start_reconfigure(1);
		//patches->mwindow->stop_reconfigure(1);
	//}
	return 0;
}

int RecordPatchOld::cursor_moved_over()
{
	if(patches->button_down && patches->new_status != get_value()) 
	{
		update(patches->new_status);
		patch->record = get_value();
		return 1;
	}
	return 0;
}

TitlePatchOld::TitlePatchOld(MWindow *mwindow, Patch *patch, char *text, int x, int y)
 : BC_TextBox(x, y + PATCH_TITLE, 124, 1, text, 0)
{ 
	this->patch = patch; 
	patches = patch->patches; 
	module = 0; 
}

int TitlePatchOld::handle_event()
{
	if(!module) module = patch->get_module();
	module->set_title(get_text());
	strcpy(patch->title, get_text());
	return 1;
}

DrawPatchOld::DrawPatchOld(MWindow *mwindow, Patch *patch, int x, int y)
 : BC_Toggle(x + PATCH_DRAW, 
 	y + PATCH_ROW2, 
	mwindow->theme->drawpatch_data,
	1,
	"",
 	1, 
	0, 
	0)
{ 
	this->patch = patch; 
	this->patches = patch->patches; 
}

int DrawPatchOld::handle_event()
{
// get the total selected before this event
	if(shift_down())
	{
		int total_selected = patches->draws_selected();

		if(total_selected == 0)
		{
// nothing previously selected
			patches->select_all_draw();
		}
		else
		if(total_selected == 1)
		{
			if(patch->draw)
			{
// this patch was previously the only one on
				patches->select_all_draw();
			}
			else
			{
// another patch was previously the only one on
				patches->deselect_all_draw();
				patch->draw = 1;
			}
		}
		else
		if(total_selected > 1)
		{
			patches->deselect_all_draw();
			patch->draw = 1;
		}
		
		update(patch->draw);
	}
	else
	{
		patch->draw = get_value();
	}
	patches->button_down = 1;
	patches->new_status = get_value();
	return 1;
}

int DrawPatchOld::cursor_moved_over()
{
	if(patches->button_down && patches->new_status != get_value())
	{
		update(patches->new_status);
		patch->draw = get_value();
	return 1;
	}
	return 0;
}
