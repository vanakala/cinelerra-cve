// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2022 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef GUIELEMENTS_H
#define GUIELEMENTS_H

#include "bctoggle.h"
#include "bctextbox.h"
#include "guielements.inc"

class ValueTextBox : public BC_TextBox
{
public:
	ValueTextBox(int x, int y, int *value, int width = GUIELEM_VAL_W);

	int handle_event();
private:
	int *valueptr;
};

class DblValueTextBox : public BC_TextBox
{
public:
	DblValueTextBox(int x, int y, double *value, int width = GUIELEM_VAL_W,
		int precision = 4);

	int handle_event();
private:
	double *valueptr;
};


class TextBox : public BC_TextBox
{
public:
	TextBox(int x, int y, char *boxtext, int width = GUIELEM_TXT_w);

	int handle_event();
private:
	char *str;
};

class CheckBox : public BC_CheckBox
{
public:
	CheckBox(int x, int y, const char *text, int *value);

	int handle_event();
private:
	int *valueptr;
};

#endif
