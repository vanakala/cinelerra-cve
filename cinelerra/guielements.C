// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2022 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "guielements.h"

#include <string.h>

ValueTextBox::ValueTextBox(int x, int y, int *value, int width)
 : BC_TextBox(x, y, 70, 1, *value)
{
	valueptr = value;
}

int ValueTextBox::handle_event()
{
	*valueptr = atoi(get_text());
	return 1;
}


TextBox::TextBox(int x, int y, char *boxtext, int width)
 : BC_TextBox(x, y, 400, 1, boxtext)
{
	this->str = boxtext;
}

int TextBox::handle_event()
{
	strcpy(str, get_text());
	return 1;
}


CheckBox::CheckBox(int x, int y, const char *text, int *value)
 : BC_CheckBox(x, y, *value, text)
{
	valueptr = value;
}

int CheckBox::handle_event()
{
	*valueptr = get_value();
	return 1;
}
