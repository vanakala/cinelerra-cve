// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "undostackitem.h"
#include <string.h>

UndoStackItem::UndoStackItem() : ListItem<UndoStackItem>()
{
	description[0] = 0;
	creator = 0;
}

void UndoStackItem::set_description(const char *description)
{
	strncpy(this->description, description, UNDO_DESCLEN);
	this->description[UNDO_DESCLEN - 1] = 0;
}

void UndoStackItem::set_creator(void *creator)
{
	this->creator = creator;
}
