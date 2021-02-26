// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "clip.h"
#include "edl.h"
#include "filexml.h"
#include "labels.h"
#include <string.h>


Labels::Labels(EDL *edl, const char *xml_tag)
 : List<Label>()
{
	this->edl = edl;
	this->xml_tag = xml_tag;
}

Labels::~Labels()
{
	delete_all();
}

void Labels::reset_instance()
{
	delete_all();
}

void Labels::dump(int indent)
{
	printf("%*sLabels %p dump(%d):\n", indent, "", this, total());
	indent += 2;

	for(Label *current = first; current; current = NEXT)
		current->dump(indent);
}

void Labels::toggle_label(ptstime start, ptstime end)
{
	Label *current;

// handle selection start
// find label the selectionstart is after
	for(current = first; 
		current && current->position < start && !edl->equivalent(current->position, start); 
		current = NEXT);

	if(current)
	{
		if(edl->equivalent(current->position, start))
		{        // remove it
			remove(current);
		}
		else
		{        // insert before it
			current = insert_before(current, new Label(start));
		}
	}
	else
	{           // insert after last
		current = append(new Label(start));
	}

// handle selection end
	if(!EQUIV(start, end))
	{
		for(current = first; 
			current && current->position < end && !edl->equivalent(current->position, end); 
			current = NEXT);

		if(current)
		{
			if(edl->equivalent(current->position, end))
			{
				remove(current);
			}
			else
			{
				current = insert_before(current, new Label(end));
			}
		}
		else
		{
			current = append(new Label(end));
		}
	}
}

void Labels::delete_all()
{
	while(last)
		remove(last);
}

void Labels::save_xml(FileXML *xml)
{
	char string[BCTEXTLEN];

	if(!total())
		return;

	xml->tag.set_title(xml_tag);
	xml->append_tag();
	xml->append_newline();

	Label *current;
	sprintf(string, "/%s", xml_tag);
	string[strlen(string) - 1] = 0; // remove trailing "S" on "LABELS" giving "LABEL"
	for(current = first; current; current = NEXT)
	{
		xml->tag.set_title(string + 1); // skip the "/" for opening tag
		xml->tag.set_property("TIME", current->position);
		xml->tag.set_property("TEXTSTR", current->textstr);
		xml->append_tag();
		xml->tag.set_title(string); // closing tag
		xml->append_tag();
		xml->append_newline();
	}

	sprintf(string, "/%s", xml_tag);
	xml->tag.set_title(string);
	xml->append_tag();
	xml->append_newline();
}

void Labels::copy(Labels *that, ptstime start, ptstime end)
{
	while(last)
		delete(last);

	for(Label *current = that->first; current; current = NEXT)
	{
		if(current->position < start + EPSILON || current->position > end - EPSILON)
			continue;
		append(new Label(current->position - start, current->textstr));
	}
}

void Labels::copy_from(Labels *labels)
{
	while(last) delete last;

	for(Label *current = labels->first; current; current = NEXT)
	{
		append(new Label(current->position, current->textstr));
	}
}


Labels& Labels::operator=(Labels &that)
{
	copy_from(&that);
	return *this;
}

void Labels::load(FileXML *xml)
{
	int result = 0;
	char string1[BCTEXTLEN], string2[BCTEXTLEN];

	sprintf(string1, "/%s", xml_tag);
	strcpy(string2, xml_tag);
	string2[strlen(string2) - 1] = 0;

	do{
		result = xml->read_tag();
		if(!result)
		{
			if(xml->tag.title_is(string1))
			{
				result = 1;
			}
			else
			if(xml->tag.title_is(string2))
			{
				double position = xml->tag.get_property("TIME", (double)-1);
				if(position < 0)
					position = xml->tag.get_property("SAMPLE", (double)-1);
				if(position > -1)
				{
					Label *current = label_of(position);
					current = insert_before(current, new Label(position));
					xml->tag.get_property("TEXTSTR", current->textstr);
				}
			}
			else
			if(xml->tag.title_is("INPOINT"))
			{
				double position = xml->tag.get_property("TIME", (double)-1);
				if(position < 0)
					position = xml->tag.get_property("SAMPLE", (double)-1);
			}
			else
			if(xml->tag.title_is("OUTPOINT"))
			{
				double position = xml->tag.get_property("TIME", (double)-1);
				if(position < 0)
					position = xml->tag.get_property("SAMPLE", (double)-1);
			}
		}
	}while(!result);
}

void Labels::clear(ptstime start, ptstime end, int follow)
{
	Label *current;
	Label *next;

	current = label_of(start);

// remove selected labels
	while(current && current->position < end)
	{
		next = NEXT;
		delete current;
		current = next;
	}

// Shift later labels
	if(follow)
	{
		while(current)
		{
			current->position -= end - start;   // shift labels forward
			current = NEXT;
		}
		optimize();
	}
}

Label* Labels::prev_label(ptstime position)
{
	Label *current;

// Test for label under cursor position
	for(current = first; 
		current && !edl->equivalent(current->position, position); 
		current = NEXT);

// Test for label after cursor position
	if(!current)
		for(current = first;
			current && current->position < position;
			current = NEXT);

// Test for label before cursor position
	if(!current) 
		current = last;
	else
// Get previous label
		current = PREVIOUS;

	return current;
}

Label* Labels::next_label(ptstime position)
{
	Label *current;

// Test for label under cursor position
	for(current = first; 
		current && !edl->equivalent(current->position, position); 
		current = NEXT);

// Test for label before cursor position
	if(!current)
		for(current = last;
			current && current->position > position;
			current = PREVIOUS);

// Test for label after cursor position
	if(!current)
		current = first;
	else
// Get next label
		current = NEXT;

	return current;
}

void Labels::insert(ptstime start, ptstime length)
{
// shift every label including the first one back
	Label *current;

	for(current = label_of(start); current; current = NEXT)
	{
		current->position += length;
	}
}

void Labels::paste_silence(ptstime start, ptstime end)
{
	insert(start, end - start);
	optimize();
}

void Labels::modify_handles(ptstime oldposition,
	ptstime newposition,
	int currentend, 
	int handle_mode,
	int edit_labels)
{
	if(edit_labels &&
		handle_mode == MOVE_ALL_EDITS)
	{
		if(currentend == HANDLE_LEFT)          // left handle
		{
			if(newposition < oldposition)
			{
				insert(oldposition, oldposition - newposition);    // shift all labels right
			}
			else
			{
				clear(oldposition, newposition);   // clear selection
			}
		}
		else
		if(currentend == HANDLE_RIGHT)  // right handle
		{
			if(newposition < oldposition)
			{
				clear(newposition, oldposition);
			}
			else
			{
				insert(oldposition, newposition - oldposition);
			}
		}
	}
}

void Labels::optimize()
{
	int result = 1;
	Label *current;

	while(result)
	{
		result = 0;

		for(current = first; current && NEXT && !result;)
		{
			Label *next = NEXT;
			if(EQUIV(current->position, next->position))
			{
				delete current;
				result  = 1;
			}
			current = next;
		}
	}
}

Label* Labels::label_of(ptstime position)
{
	Label *current;

	for(current = first; current; current = NEXT)
	{
		if(current->position >= position) return current;
	}
	return 0;
}

size_t Labels::get_size()
{
	size_t size = sizeof(*this);

	for(Label *current = first; current; current = NEXT)
		size += current->get_size();
	return size;
}

Label::Label(ptstime position, const char *textstr)
 : ListItem<Label>()
{
	this->position = position;
	if(textstr)
		strcpy(this->textstr, textstr);
	else
		this->textstr[0] = 0;
	is_edited = 0;
}

void Label::dump(int indent)
{
	printf("%*sLabel: %p %.2f edited %d '%s'\n", indent, "", this,
		 position, is_edited, textstr);
}

size_t Label::get_size()
{
	return sizeof(*this);
}
