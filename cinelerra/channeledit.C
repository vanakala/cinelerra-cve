#include "batch.h"
#include "channel.h"
#include "channeldb.h"
#include "channeledit.h"
#include "channelpicker.h"
#include "chantables.h"
#include "condition.h"
#include "language.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "picture.h"
#include "record.h"
#include "recordgui.h"
#include "theme.h"
#include "videodevice.h"

#include <string.h>

ChannelEditThread::ChannelEditThread(MWindow *mwindow, 
	ChannelPicker *channel_picker,
	ChannelDB *channeldb,
	Record *record)
 : Thread()
{
	this->channel_picker = channel_picker;
	this->mwindow = mwindow;
	this->channeldb = channeldb;
	this->record = record;
	in_progress = 0;
	this->window = 0;
	new_channels = new ChannelDB;
	completion = new Condition(1, "ChannelEditThread::completion");
}
ChannelEditThread::~ChannelEditThread()
{
	delete new_channels;
	delete completion;
}

void ChannelEditThread::run()
{
	int i;

	if(in_progress) 
	{
		if(window)
		{
			window->lock_window("ChannelEditThread::run");
			window->raise_window(1);
			window->unlock_window();
		}
		return;
	}
	in_progress = 1;
	completion->lock("ChannelEditThread::run");

// Copy master channel list to temporary.
	new_channels->copy_from(channel_picker->channeldb);
	current_channel = channel_picker->get_current_channel_number();
//printf("ChannelEditThread::run 1 %d\n", current_channel);

// Run the channel list window using the temporary list.
	ChannelEditWindow window(mwindow, this, channel_picker);
	window.create_objects();
	this->window = &window;
	int result = window.run_window();
	this->window = 0;

	if(!result)
	{
// Copy new channels to master list
		channel_picker->channeldb->clear();
		
		channel_picker->channeldb->copy_from(new_channels);
		channel_picker->update_channel_list();

		if(record)
		{
			record->record_gui->lock_window("ChannelEditThread::run");
			record->record_gui->update_batch_sources();

//printf("ChannelEditThread::run 10 %d\n", current_channel);
			record->set_channel(current_channel);
			record->record_gui->unlock_window();
			record->save_defaults();
			record->channeldb->save(record->get_channeldb_prefix());
		}
		mwindow->save_defaults();
	}
	else
	{
// Rejected.
		if(record)
		{
			record->set_channel(record->get_editing_batch()->channel);
		}
	}

	window.edit_thread->close_threads();
	window.picture_thread->close_threads();

	completion->unlock();
	in_progress = 0;

}

int ChannelEditThread::close_threads()
{
	if(in_progress && window)
	{
		window->edit_thread->close_threads();
		window->picture_thread->close_threads();
		window->set_done(1);
		completion->lock("ChannelEditThread::close_threads");
		completion->unlock();
	}
}


ChannelEditWindow::ChannelEditWindow(MWindow *mwindow, 
	ChannelEditThread *thread, 
	ChannelPicker *channel_picker)
 : BC_Window(PROGRAM_NAME ": Channels", 
 	mwindow->gui->get_abs_cursor_x(1) - 330, 
	mwindow->gui->get_abs_cursor_y(1), 
	330, 
	330, 
	330, 
	330,
	0,
	0,
	1)
{
	this->thread = thread;
	this->channel_picker = channel_picker;
	this->mwindow = mwindow;
}
ChannelEditWindow::~ChannelEditWindow()
{
	int i;
	for(i = 0; i < channel_list.total; i++)
	{
		delete channel_list.values[i];
	}
	channel_list.remove_all();
	delete edit_thread;
	delete picture_thread;
}

int ChannelEditWindow::create_objects()
{
	int x = 10, y = 10, i;
	char string[1024];

// Create channel list
	for(i = 0; i < thread->new_channels->size(); i++)
	{
		channel_list.append(new BC_ListBoxItem(thread->new_channels->get(i)->title));
	}

	add_subwindow(list_box = new ChannelEditList(mwindow, this, x, y));
	x += 200;
	if(thread->record)
	{
		add_subwindow(new ChannelEditSelect(mwindow, this, x, y));
		y += 30;
	}
	add_subwindow(new ChannelEditAdd(mwindow, this, x, y));
	y += 30;
	add_subwindow(new ChannelEdit(mwindow, this, x, y));
	y += 30;
	add_subwindow(new ChannelEditMoveUp(mwindow, this, x, y));
	y += 30;
	add_subwindow(new ChannelEditMoveDown(mwindow, this, x, y));
	y += 30;
	add_subwindow(new ChannelEditDel(mwindow, this, x, y));
	y += 30;
	add_subwindow(new ChannelEditPicture(mwindow, this, x, y));
	y += 100;
	x = 10;
	add_subwindow(new BC_OKButton(this));
	x += 150;
	add_subwindow(new BC_CancelButton(this));


	edit_thread = new ChannelEditEditThread(this, 
		channel_picker, 
		thread->record);
	picture_thread = new ChannelEditPictureThread(channel_picker, this);
	show_window();
	return 0;
}

int ChannelEditWindow::close_event()
{
	set_done(0);
}

int ChannelEditWindow::add_channel()
{
	Channel *new_channel;
	Channel *prev_channel = 0;

// Create new channel
	new_channel = new Channel;

// Reuse parameters from previous channel
	if(thread->new_channels->size()) 
	{
		prev_channel = thread->new_channels->get(
				thread->new_channels->size() - 1);
		new_channel->copy_settings(prev_channel);
	}
	else
// Use default channel parameters
	if(thread->record)
	{
		new_channel->copy_settings(thread->record->master_channel);
	}

// Copy device usage.  Need the same thing for playback.
	if(thread->record)
	{
		new_channel->copy_usage(thread->record->master_channel);
	}

// Add to channel table
	channel_list.append(new BC_ListBoxItem(new_channel->title));
	thread->new_channels->append(new_channel);
	update_list();

// Start common routing
	edit_thread->edit_channel(new_channel, 0);
	return 0;
}

int ChannelEditWindow::update_list()
{
	list_box->update(&channel_list, 0, 0, 1, list_box->get_yposition());
}

int ChannelEditWindow::update_list(Channel *channel)
{
	int i;
	for(i = 0; i < thread->new_channels->size(); i++)
		if(thread->new_channels->get(i) == channel) break;

	if(i < thread->new_channels->size())
	{
		channel_list.values[i]->set_text(channel->title);
	}

	update_list();
}


int ChannelEditWindow::edit_channel()
{
	if(list_box->get_selection_number(0, 0) > -1)
	{
		thread->current_channel = list_box->get_selection_number(0, 0);
		edit_thread->edit_channel(
			thread->new_channels->get(
				list_box->get_selection_number(0, 0)), 
			1);
	}
}

int ChannelEditWindow::edit_picture()
{
	picture_thread->edit_picture();
}

int ChannelEditWindow::delete_channel(int number)
{
	delete thread->new_channels->get(number);
	channel_list.remove_number(number);
	thread->new_channels->remove_number(number);
	update_list();
}

int ChannelEditWindow::delete_channel(Channel *channel)
{
	int i;
	for(i = 0; i < thread->new_channels->size(); i++)
	{
		if(thread->new_channels->get(i) == channel)
		{
			break;
		}
	}
	if(i < thread->new_channels->size()) delete_channel(i);
	return 0;
}

int ChannelEditWindow::move_channel_up()
{
	if(list_box->get_selection_number(0, 0) > -1)
	{
		int number2 = list_box->get_selection_number(0, 0);
		int number1 = number2 - 1;
		Channel *temp;
		BC_ListBoxItem *temp_text;

		if(number1 < 0) number1 = thread->new_channels->size() - 1;

		temp = thread->new_channels->get(number1);
		thread->new_channels->set(number1, thread->new_channels->get(number2));
		thread->new_channels->set(number2, temp);

		temp_text = channel_list.values[number1];
		channel_list.values[number1] = channel_list.values[number2];
		channel_list.values[number2] = temp_text;
		list_box->update(&channel_list, 
			0, 
			0, 
			1, 
			list_box->get_xposition(), 
			list_box->get_yposition(), 
			number1,
			1);
	}
	return 0;
}

int ChannelEditWindow::move_channel_down()
{
	if(list_box->get_selection_number(0, 0) > -1)
	{
		int number2 = list_box->get_selection_number(0, 0);
		int number1 = number2 + 1;
		Channel *temp;
		BC_ListBoxItem *temp_text;

		if(number1 > thread->new_channels->size() - 1) number1 = 0;

		temp = thread->new_channels->get(number1);
		thread->new_channels->set(number1, thread->new_channels->get(number2));
		thread->new_channels->set(number2, temp);
		temp_text = channel_list.values[number1];
		channel_list.values[number1] = channel_list.values[number2];
		channel_list.values[number2] = temp_text;
		list_box->update(&channel_list, 
			0, 
			0, 
			1, 
			list_box->get_xposition(), 
			list_box->get_yposition(), 
			number1,
			1);
	}
	return 0;
}

int ChannelEditWindow::change_channel_from_list(int channel_number)
{
	Channel *channel;
	if(channel_number > -1 && channel_number < thread->new_channels->size())
	{
		thread->current_channel = channel_number;
		channel_picker->set_channel(thread->new_channels->get(channel_number));
	}
}

ChannelEditSelect::ChannelEditSelect(MWindow *mwindow, ChannelEditWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Select"))
{
	this->window = window;
}
ChannelEditSelect::~ChannelEditSelect()
{
}
int ChannelEditSelect::handle_event()
{
	window->change_channel_from_list(
		window->list_box->get_selection_number(0, 0));
}

ChannelEditAdd::ChannelEditAdd(MWindow *mwindow, ChannelEditWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Add..."))
{
	this->window = window;
}
ChannelEditAdd::~ChannelEditAdd()
{
}
int ChannelEditAdd::handle_event()
{
	window->add_channel();
}

ChannelEditList::ChannelEditList(MWindow *mwindow, ChannelEditWindow *window, int x, int y)
 : BC_ListBox(x, 
 			y, 
			185, 
			250, 
			LISTBOX_TEXT, 
			&(window->channel_list))
{
	this->window = window;
}
ChannelEditList::~ChannelEditList()
{
}
int ChannelEditList::handle_event()
{
	window->edit_channel();
}

ChannelEditMoveUp::ChannelEditMoveUp(MWindow *mwindow, ChannelEditWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Move up"))
{
	this->window = window;
}
ChannelEditMoveUp::~ChannelEditMoveUp()
{
}
int ChannelEditMoveUp::handle_event()
{
	lock_window("ChannelEditMoveUp::handle_event");
	window->move_channel_up();
	unlock_window();
}

ChannelEditMoveDown::ChannelEditMoveDown(MWindow *mwindow, ChannelEditWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Move down"))
{
	this->window = window;
}
ChannelEditMoveDown::~ChannelEditMoveDown()
{
}
int ChannelEditMoveDown::handle_event()
{
	lock_window("ChannelEditMoveDown::handle_event");
	window->move_channel_down();
	unlock_window();
}

ChannelEditDel::ChannelEditDel(MWindow *mwindow, ChannelEditWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Delete"))
{
	this->window = window;
}
ChannelEditDel::~ChannelEditDel()
{
}
int ChannelEditDel::handle_event()
{
	if(window->list_box->get_selection_number(0, 0) > -1) window->delete_channel(window->list_box->get_selection_number(0, 0));
}

ChannelEdit::ChannelEdit(MWindow *mwindow, ChannelEditWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Edit..."))
{
	this->window = window;
}
ChannelEdit::~ChannelEdit()
{
}
int ChannelEdit::handle_event()
{
	window->edit_channel();
}

ChannelEditPicture::ChannelEditPicture(MWindow *mwindow, ChannelEditWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Picture..."))
{
	this->window = window;
}
ChannelEditPicture::~ChannelEditPicture()
{
}
int ChannelEditPicture::handle_event()
{
	window->edit_picture();
}



// ================================= Edit a single channel



ChannelEditEditThread::ChannelEditEditThread(ChannelEditWindow *window, 
	ChannelPicker *channel_picker,
	Record *record)
 : Thread()
{
	this->window = window;
	this->channel_picker = channel_picker;
	this->record = record;
	in_progress = 0;
	edit_window = 0;
	editing = 0;
	completion = new Condition(1, "ChannelEditEditThread::completion");
}

ChannelEditEditThread::~ChannelEditEditThread()
{
	delete completion;
}

int ChannelEditEditThread::close_threads()
{
	if(edit_window)
	{
		edit_window->set_done(1);
		completion->lock("ChannelEditEditThread::close_threads");
		completion->unlock();
	}
}

int ChannelEditEditThread::edit_channel(Channel *channel, int editing)
{
	if(in_progress) 
	{
		edit_window->lock_window("ChannelEditEditThread::edit_channel");
		edit_window->raise_window(1);
		edit_window->unlock_window();
		return 1;
	}
	in_progress = 1;

// Copy the channel to edit into a temporary
	completion->lock("ChannelEditEditThread::edit_channel");
	this->editing = editing;
	this->output_channel = channel;
	new_channel.copy_settings(output_channel);
	new_channel.copy_usage(output_channel);

	if(editing && new_channel.title[0])
		user_title = 1;
	else
		user_title = 0;
	set_synchronous(0);
	Thread::start();
}

char *ChannelEditEditThread::value_to_freqtable(int value)
{
	switch(value)
	{
		case NTSC_BCAST:
			return _("NTSC_BCAST");
			break;
		case NTSC_CABLE:
			return _("NTSC_CABLE");
			break;
		case NTSC_HRC:
			return _("NTSC_HRC");
			break;
		case NTSC_BCAST_JP:
			return _("NTSC_BCAST_JP");
			break;
		case NTSC_CABLE_JP:
			return _("NTSC_CABLE_JP");
			break;
		case PAL_AUSTRALIA:
			return _("PAL_AUSTRALIA");
			break;
		case PAL_EUROPE:
			return _("PAL_EUROPE");
			break;
		case PAL_E_EUROPE:
			return _("PAL_E_EUROPE");
			break;
		case PAL_ITALY:
			return _("PAL_ITALY");
			break;
		case PAL_IRELAND:
			return _("PAL_IRELAND");
			break;
		case PAL_NEWZEALAND:
			return _("PAL_NEWZEALAND");
			break;
	}
}

char* ChannelEditEditThread::value_to_norm(int value)
{
	switch(value)
	{
		case NTSC:
			return _("NTSC");
			break;
		case PAL:
			return _("PAL");
			break;
		case SECAM:
			return _("SECAM");
			break;
	}
}

char* ChannelEditEditThread::value_to_input(int value)
{
	if(channel_picker->get_video_inputs()->total > value)
		return channel_picker->get_video_inputs()->values[value]->device_name;
	else
		return _("None");
}

void ChannelEditEditThread::set_device()
{
	channel_picker->set_channel(&new_channel);
}

int ChannelEditEditThread::change_source(char *source_name)
{
	int i, result;
	for(i = 0; i < chanlists[new_channel.freqtable].count; i++)
	{
		if(!strcasecmp(chanlists[new_channel.freqtable].list[i].name, source_name))
		{
			new_channel.entry = i;
			i = chanlists[new_channel.freqtable].count;
			set_device();
		}
	}
	if(!user_title)
	{
		strcpy(new_channel.title, source_name);
		if(edit_window->title_text)
		{
			edit_window->title_text->update(source_name);
		}
	}
}

int ChannelEditEditThread::source_up()
{
	new_channel.entry++;
	if(new_channel.entry > chanlists[new_channel.freqtable].count - 1) new_channel.entry = 0;
	source_text->update(chanlists[new_channel.freqtable].list[new_channel.entry].name);
	set_device();
}

int ChannelEditEditThread::source_down()
{
	new_channel.entry--;
	if(new_channel.entry < 0) new_channel.entry = chanlists[new_channel.freqtable].count - 1;
	source_text->update(chanlists[new_channel.freqtable].list[new_channel.entry].name);
	set_device();
}

int ChannelEditEditThread::set_input(int value)
{
	new_channel.input = value;
	set_device();
}

int ChannelEditEditThread::set_norm(int value)
{
	new_channel.norm = value;
	set_device();
}

int ChannelEditEditThread::set_freqtable(int value)
{
	new_channel.freqtable = value;
	if(new_channel.entry > chanlists[new_channel.freqtable].count - 1) new_channel.entry = 0;
	source_text->update(chanlists[new_channel.freqtable].list[new_channel.entry].name);
	set_device();
}

void ChannelEditEditThread::run()
{
	ChannelEditEditWindow edit_window(this, window, channel_picker);
	edit_window.create_objects(&new_channel);
	this->edit_window = &edit_window;
	int result = edit_window.run_window();
	this->edit_window = 0;

// Done editing channel.  Keep channel.
	if(!result)
	{
		output_channel->copy_settings(&new_channel);
		window->lock_window();
		window->update_list(output_channel);
		window->unlock_window();
	}
	else
	{
// Discard channel.
		if(!editing)
		{
			window->lock_window();
			window->delete_channel(output_channel);
			window->unlock_window();
		}
	}
	editing = 0;
	completion->unlock();
	in_progress = 0;
}

ChannelEditEditWindow::ChannelEditEditWindow(ChannelEditEditThread *thread, 
	ChannelEditWindow *window,
	ChannelPicker *channel_picker)
 : BC_Window(PROGRAM_NAME ": Edit Channel", 
 	channel_picker->mwindow->gui->get_abs_cursor_x(1), 
	channel_picker->mwindow->gui->get_abs_cursor_y(1), 
 	390, 
	270, 
	390, 
	270,
	0,
	0,
	1)
{
	this->channel_picker = channel_picker;
	this->window = window;
	this->thread = thread;
}
ChannelEditEditWindow::~ChannelEditEditWindow()
{
}
int ChannelEditEditWindow::create_objects(Channel *channel)
{
	this->new_channel = channel;
	Channel *channel_usage = channel_picker->get_channel_usage();
	title_text = 0;

	int x = 10, y = 10;
	if(!channel_usage ||
		(!channel_usage->use_frequency && 
		!channel_usage->use_fine && 
		!channel_usage->use_norm && 
		!channel_usage->use_input))
	{
		add_subwindow(new BC_Title(x, y, "Device has no input selection."));
		y += 30;
	}
	else
	{
		add_subwindow(new BC_Title(x, y, _("Title:")));
		add_subwindow(title_text = new ChannelEditEditTitle(x, y + 20, thread));
		y += 50;
	}

	if(channel_usage->use_frequency)
	{
		add_subwindow(new BC_Title(x, y, _("Channel:")));
		y += 20;
		add_subwindow(thread->source_text = new ChannelEditEditSource(x, y, thread));
		add_subwindow(new ChannelEditEditSourceTumbler(x + 160, y, thread));
		y += 40;

		add_subwindow(new BC_Title(x, y, _("Frequency table:")));
		ChannelEditEditFreqtable *table;
		add_subwindow(table = new ChannelEditEditFreqtable(x + 130, y, thread));
		table->add_items();
		y += 30;
	}

	if(channel_usage->use_fine)
	{
		add_subwindow(new BC_Title(x, y, _("Fine:")));
		add_subwindow(new ChannelEditEditFine(x + 130, y, thread));
		y += 30;
	}

	if(channel_usage->use_norm)
	{
		add_subwindow(new BC_Title(x, y, _("Norm:")));
		ChannelEditEditNorm *norm;
		add_subwindow(norm = new ChannelEditEditNorm(x + 130, y, thread));
		norm->add_items();
		y += 30;
	}

	if(channel_usage->use_input)
	{
		add_subwindow(new BC_Title(x, y, _("Input:")));
		ChannelEditEditInput *input;
		add_subwindow(input = new ChannelEditEditInput(x + 130, y, thread, thread->record));
		input->add_items();
		y += 30;
	}

	add_subwindow(new BC_OKButton(x, y));
	x += 200;
	add_subwindow(new BC_CancelButton(x, y));
	show_window();
	return 0;
}

ChannelEditEditTitle::ChannelEditEditTitle(int x, 
	int y, 
	ChannelEditEditThread *thread)
 : BC_TextBox(x, y, 150, 1, thread->new_channel.title)
{
	this->thread = thread;
}
ChannelEditEditTitle::~ChannelEditEditTitle()
{
}
int ChannelEditEditTitle::handle_event()
{
	if(strlen(get_text()) < 1024)
	{
		strcpy(thread->new_channel.title, get_text());
	}
	if(get_text()[0]) 
		thread->user_title = 1;
	else
		thread->user_title = 0;
	return 1;
}


ChannelEditEditSource::ChannelEditEditSource(int x, int y, ChannelEditEditThread *thread)
 : BC_TextBox(x, y, 150, 1, chanlists[thread->new_channel.freqtable].list[thread->new_channel.entry].name)
{
	this->thread = thread;
}

ChannelEditEditSource::~ChannelEditEditSource()
{
}
int ChannelEditEditSource::handle_event()
{
	thread->change_source(get_text());
}


ChannelEditEditSourceTumbler::ChannelEditEditSourceTumbler(int x, int y, ChannelEditEditThread *thread)
 : BC_Tumbler(x, y)
{
	this->thread = thread;
}
ChannelEditEditSourceTumbler::~ChannelEditEditSourceTumbler()
{
}
int ChannelEditEditSourceTumbler::handle_up_event()
{
	thread->source_up();
}
int ChannelEditEditSourceTumbler::handle_down_event()
{
	thread->source_down();
}

ChannelEditEditInput::ChannelEditEditInput(int x, int y, ChannelEditEditThread *thread, Record *record)
 : BC_PopupMenu(x, y, 150, thread->value_to_input(thread->new_channel.input))
{
	this->thread = thread;
	this->record = record;
}
ChannelEditEditInput::~ChannelEditEditInput()
{
}
int ChannelEditEditInput::add_items()
{
	ArrayList<Channel*> *inputs;
	inputs = thread->channel_picker->get_video_inputs();
	
	if(inputs)
		for(int i = 0; i < inputs->total; i++)
		{
			add_item(new ChannelEditEditInputItem(thread, inputs->values[i]->device_name, i));
		}
}
int ChannelEditEditInput::handle_event()
{
	return 0;
}

ChannelEditEditInputItem::ChannelEditEditInputItem(ChannelEditEditThread *thread, char *text, int value)
 : BC_MenuItem(text)
{
	this->thread = thread;
	this->value = value;
}
ChannelEditEditInputItem::~ChannelEditEditInputItem()
{
}
int ChannelEditEditInputItem::handle_event()
{
	get_popup_menu()->set_text(get_text());
	if(!thread->user_title)
	{
		strcpy(thread->new_channel.title, get_text());
		if(thread->edit_window->title_text)
		{
			thread->edit_window->title_text->update(get_text());
		}
	}
	thread->set_input(value);
}

ChannelEditEditNorm::ChannelEditEditNorm(int x, int y, ChannelEditEditThread *thread)
 : BC_PopupMenu(x, y, 100, thread->value_to_norm(thread->new_channel.norm))
{
	this->thread = thread;
}
ChannelEditEditNorm::~ChannelEditEditNorm()
{
}
int ChannelEditEditNorm::add_items()
{
	add_item(new ChannelEditEditNormItem(thread, thread->value_to_norm(NTSC), NTSC));
	add_item(new ChannelEditEditNormItem(thread, thread->value_to_norm(PAL), PAL));
	add_item(new ChannelEditEditNormItem(thread, thread->value_to_norm(SECAM), SECAM));
	return 0;
}


ChannelEditEditNormItem::ChannelEditEditNormItem(ChannelEditEditThread *thread, char *text, int value)
 : BC_MenuItem(text)
{
	this->value = value;
	this->thread = thread;
}
ChannelEditEditNormItem::~ChannelEditEditNormItem()
{
}
int ChannelEditEditNormItem::handle_event()
{
	get_popup_menu()->set_text(get_text());
	thread->set_norm(value);
}


ChannelEditEditFreqtable::ChannelEditEditFreqtable(int x, int y, ChannelEditEditThread *thread)
 : BC_PopupMenu(x, y, 150, thread->value_to_freqtable(thread->new_channel.freqtable))
{
	this->thread = thread;
}
ChannelEditEditFreqtable::~ChannelEditEditFreqtable()
{
}
int ChannelEditEditFreqtable::add_items()
{
	add_item(new ChannelEditEditFreqItem(thread, thread->value_to_freqtable(NTSC_BCAST), NTSC_BCAST));
	add_item(new ChannelEditEditFreqItem(thread, thread->value_to_freqtable(NTSC_CABLE), NTSC_CABLE));
	add_item(new ChannelEditEditFreqItem(thread, thread->value_to_freqtable(NTSC_HRC), NTSC_HRC));
	add_item(new ChannelEditEditFreqItem(thread, thread->value_to_freqtable(NTSC_BCAST_JP), NTSC_BCAST_JP));
	add_item(new ChannelEditEditFreqItem(thread, thread->value_to_freqtable(NTSC_CABLE_JP), NTSC_CABLE_JP));
	add_item(new ChannelEditEditFreqItem(thread, thread->value_to_freqtable(PAL_AUSTRALIA), PAL_AUSTRALIA));
	add_item(new ChannelEditEditFreqItem(thread, thread->value_to_freqtable(PAL_EUROPE), PAL_EUROPE));
	add_item(new ChannelEditEditFreqItem(thread, thread->value_to_freqtable(PAL_E_EUROPE), PAL_E_EUROPE));
	add_item(new ChannelEditEditFreqItem(thread, thread->value_to_freqtable(PAL_ITALY), PAL_ITALY));
	add_item(new ChannelEditEditFreqItem(thread, thread->value_to_freqtable(PAL_IRELAND), PAL_IRELAND));
	add_item(new ChannelEditEditFreqItem(thread, thread->value_to_freqtable(PAL_NEWZEALAND), PAL_NEWZEALAND));
	return 0;
}

ChannelEditEditFreqItem::ChannelEditEditFreqItem(ChannelEditEditThread *thread, char *text, int value)
 : BC_MenuItem(text)
{
	this->value = value;
	this->thread = thread;
}
ChannelEditEditFreqItem::~ChannelEditEditFreqItem()
{
}
int ChannelEditEditFreqItem::handle_event()
{
	get_popup_menu()->set_text(get_text());
	thread->set_freqtable(value);
}



ChannelEditEditFine::ChannelEditEditFine(int x, int y, ChannelEditEditThread *thread)
 : BC_ISlider(x, 
 		y, 
		0, 
		240, 
		240, 
		-100, 
		100, 
		thread->new_channel.fine_tune)
{
	this->thread = thread;
}
ChannelEditEditFine::~ChannelEditEditFine()
{
}
int ChannelEditEditFine::handle_event()
{
	thread->new_channel.fine_tune = get_value();
	thread->set_device();
}


// ========================== picture quality

ChannelEditPictureThread::ChannelEditPictureThread(ChannelPicker *channel_picker, ChannelEditWindow *window)
 : Thread()
{
	this->channel_picker = channel_picker;
	this->window = window;
	in_progress = 0;
	edit_window = 0;
	completion = new Condition(1, "ChannelEditPictureThread::completion");
}
ChannelEditPictureThread::~ChannelEditPictureThread()
{
	delete completion;
}

int ChannelEditPictureThread::edit_picture()
{
	if(in_progress) 
	{
		edit_window->lock_window("ChannelEditPictureThread::edit_picture");
		edit_window->raise_window(1);
		edit_window->unlock_window();
		return 1;
	}
	in_progress = 1;
	completion->lock("ChannelEditPictureThread::edit_picture");
	set_synchronous(0);
	Thread::start();
}

void ChannelEditPictureThread::run()
{
	ChannelEditPictureWindow edit_window(this, channel_picker);
	edit_window.create_objects();
	this->edit_window = &edit_window;
	int result = edit_window.run_window();
	this->edit_window = 0;
	completion->unlock();
	in_progress = 0;
}

int ChannelEditPictureThread::close_threads()
{
	if(edit_window)
	{
		edit_window->set_done(1);
		completion->lock("ChannelEditPictureThread::close_threads");
		completion->unlock();
	}
}


ChannelEditPictureWindow::ChannelEditPictureWindow(ChannelEditPictureThread *thread, ChannelPicker *channel_picker)
 : BC_Window(PROGRAM_NAME ": Picture", 
 	channel_picker->mwindow->gui->get_abs_cursor_x(1) - 200, 
	channel_picker->mwindow->gui->get_abs_cursor_y(1) - 220, 
 	200, 
	250, 
	200, 
	250)
{
	this->thread = thread;
	this->channel_picker = channel_picker;
}
ChannelEditPictureWindow::~ChannelEditPictureWindow()
{
}
int ChannelEditPictureWindow::create_objects()
{
	int x = 10, y = 10;
	int x1 = 110, x2 = 145;
#define SWAP_X x1 ^= x2; x2 ^= x1; x1 ^= x2;

	Picture *picture_usage = channel_picker->get_picture_usage();

	if(!picture_usage ||
		(!picture_usage->use_brightness &&
		!picture_usage->use_contrast &&
		!picture_usage->use_color &&
		!picture_usage->use_hue &&
		!picture_usage->use_whiteness &&
		!channel_picker->get_controls()))
	{
		add_subwindow(new BC_Title(x, y, "Device has no picture controls."));
		y += 50;
	}

	if(picture_usage->use_brightness)
	{
		add_subwindow(new BC_Title(x, y + 10, _("Brightness:")));
		add_subwindow(new ChannelEditBright(x1, y, channel_picker, channel_picker->get_brightness()));
		y += 30;
		SWAP_X
		
	}

	if(picture_usage->use_contrast)
	{
		add_subwindow(new BC_Title(x, y + 10, _("Contrast:")));
		add_subwindow(new ChannelEditContrast(x1, y, channel_picker, channel_picker->get_contrast()));
		y += 30;
		SWAP_X
	}

	if(picture_usage->use_color)
	{
		add_subwindow(new BC_Title(x, y + 10, _("Color:")));
		add_subwindow(new ChannelEditColor(x1, y, channel_picker, channel_picker->get_color()));
		y += 30;
		SWAP_X
	}

	if(picture_usage->use_hue)
	{
		add_subwindow(new BC_Title(x, y + 10, _("Hue:")));
		add_subwindow(new ChannelEditHue(x1, y, channel_picker, channel_picker->get_hue()));
		y += 30;
		SWAP_X
	}

	if(picture_usage->use_whiteness)
	{
		add_subwindow(new BC_Title(x, y + 10, _("Whiteness:")));
		add_subwindow(new ChannelEditWhiteness(x1, y, channel_picker, channel_picker->get_whiteness()));
		y += 30;
		SWAP_X
	}

	for(int i = 0; i < channel_picker->get_controls(); i++)
	{
		add_subwindow(new BC_Title(x, 
			y + 10, 
			_(channel_picker->get_control(i)->name)));
		add_subwindow(new ChannelEditCommon(x1, 
			y, 
			channel_picker,
			channel_picker->get_control(i)));
		y += 30;
		SWAP_X
	}


	y += 20;
	add_subwindow(new BC_OKButton(x + 70, y));
	return 0;
}



ChannelEditBright::ChannelEditBright(int x, int y, ChannelPicker *channel_picker, int value)
 : BC_IPot(x, 
 		y, 
		value, 
		-100, 
		100)
{
	this->channel_picker = channel_picker;
}
ChannelEditBright::~ChannelEditBright() {}
int ChannelEditBright::handle_event()
{
	return 1;
}
int ChannelEditBright::button_release_event()
{
	if(BC_Pot::button_release_event())
	{
		channel_picker->set_brightness(get_value());
		return 1;
	}
	return 0;
}

ChannelEditContrast::ChannelEditContrast(int x, int y, ChannelPicker *channel_picker, int value)
 : BC_IPot(x, 
 		y, 
		value, 
		-100, 
		100)
{
	this->channel_picker = channel_picker;
}
ChannelEditContrast::~ChannelEditContrast() {}
int ChannelEditContrast::handle_event()
{
	return 1;
}
int ChannelEditContrast::button_release_event()
{
	if(BC_Pot::button_release_event())
	{
		channel_picker->set_contrast(get_value());
		return 1;
	}
	return 0;
}


ChannelEditColor::ChannelEditColor(int x, int y, ChannelPicker *channel_picker, int value)
 : BC_IPot(x, 
 		y, 
		value, 
		-100, 
		100)
{
	this->channel_picker = channel_picker;
}
ChannelEditColor::~ChannelEditColor() {}
int ChannelEditColor::handle_event()
{
	return 1;
}
int ChannelEditColor::button_release_event()
{
	if(BC_Pot::button_release_event())
	{
		channel_picker->set_color(get_value());
		return 1;
	}
	return 0;
}

ChannelEditHue::ChannelEditHue(int x, int y, ChannelPicker *channel_picker, int value)
 : BC_IPot(x, 
 		y, 
		value, 
		-100, 
		100)
{
	this->channel_picker = channel_picker;
}
ChannelEditHue::~ChannelEditHue() {}
int ChannelEditHue::handle_event()
{
	return 1;
}
int ChannelEditHue::button_release_event()
{
	if(BC_Pot::button_release_event())
	{
		channel_picker->set_hue(get_value());
		return 1;
	}
	return 0;
}

ChannelEditWhiteness::ChannelEditWhiteness(int x, int y, ChannelPicker *channel_picker, int value)
 : BC_IPot(x, 
 		y, 
		value, 
		-100, 
		100)
{
	this->channel_picker = channel_picker;
}
ChannelEditWhiteness::~ChannelEditWhiteness() 
{
}
int ChannelEditWhiteness::handle_event()
{
	return 1;
}
int ChannelEditWhiteness::button_release_event()
{
	if(BC_Pot::button_release_event())
	{
		channel_picker->set_whiteness(get_value());
		return 1;
	}
	return 0;
}



ChannelEditCommon::ChannelEditCommon(int x, 
	int y, 
	ChannelPicker *channel_picker,
	PictureItem *item)
 : BC_IPot(x, 
 		y, 
		item->value, 
		item->min, 
		item->max)
{
	this->channel_picker = channel_picker;
	this->device_id = item->device_id;
}

ChannelEditCommon::~ChannelEditCommon() 
{
}

int ChannelEditCommon::handle_event()
{
	return 1;
}

int ChannelEditCommon::button_release_event()
{
	if(BC_Pot::button_release_event())
	{
		channel_picker->set_picture(device_id, get_value());
		return 1;
	}
	return 0;
}



