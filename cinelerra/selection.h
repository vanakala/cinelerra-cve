// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2014 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef SELECTION_H
#define SELECTION_H

#include "selection.inc"
#include "bcbutton.h"
#include "bcmenuitem.h"
#include "bcpopupmenu.inc"
#include "bctextbox.h"
#include "bcwindow.inc"
#include "mwindow.inc"

struct selection_int
{
	const char *text;
	int value;
};

struct selection_double
{
	const char *text;
	double value;
};

struct selection_2int
{
	const char *text;
	int value1;
	int value2;
};

struct selection_2double
{
	const char *text;
	double value1;
	double value2;
};

class SelectionLeftBox;
class SelectionButton;

class Selection : public BC_TextBox
{
public:
	Selection(int x, int y, BC_WindowBase *base,
		const struct selection_int items[], int *value, int options = 0);
	Selection(int x, int y, BC_WindowBase *base,
		const struct selection_double items[], double *value);
	Selection(int x1, int y1, BC_WindowBase *base,
		const struct selection_2int items[], int *value1, int options = 0);
	Selection(int x1, int y1, int x2, int y2, BC_WindowBase *base,
		const struct selection_2int items[],
		int *value1, int *value2, int separator = 0);
	Selection(int x1, int y1, int x2, int y2, BC_WindowBase *base,
		const struct selection_2double items[],
		double *value1, double *value2, int separator);

	int calculate_width();
// option == 1 - textbox is bright, but not modifable
	void disable(int option = 0);
	void enable(int option = 0);
	void reposition_window(int x, int y);
	virtual int handle_event();
	void delete_subwindows();
	virtual void update_auto(double value1, double value2) {};

	const struct selection_int *current_int;
	const struct selection_2int *current_2int;
	const struct selection_double *current_double;
	const struct selection_2double *current_2double;

protected:
	SelectionLeftBox *firstbox;
	int *intvalue;
	int *intvalue2;
	double *doublevalue;
	double *doublevalue2;
private:
	BC_PopupMenu *init_objects(int x, int y, BC_WindowBase *base);
	SelectionButton *button;
	BC_PopupMenu *popupmenu;
};

class SelectionButton : public BC_Button
{
public:
	SelectionButton(int x, int y, BC_PopupMenu *popupmenu, VFrame **images);

	int handle_event();
private:
	BC_PopupMenu *popupmenu;
};

class SelectionItem : public BC_MenuItem
{
public:
	SelectionItem(const struct selection_int *item, Selection *output);
	SelectionItem(const struct selection_double *item, Selection *output);
	SelectionItem(const struct selection_2int *item,
		BC_TextBox *output2, Selection *output1);
	SelectionItem(const struct selection_2double *item,
		BC_TextBox *output2, Selection *output1);

	int handle_event();
private:
	const struct selection_int *intitem;
	const struct selection_double *doubleitem;
	const struct selection_2int *int2item;
	const struct selection_2double *double2item;

	Selection *output;
	BC_TextBox *output2;
};


class SampleRateSelection : public Selection
{
public:
	SampleRateSelection(int x, int y, BC_WindowBase *base, int *value);

	static int limits(int *rate);
private:
	static const struct selection_int sample_rates[];
};


class FrameRateSelection : public Selection
{
public:
	FrameRateSelection(int x, int y, BC_WindowBase *base, double *value);

	static int limits(double *rate);
private:
	static const struct selection_double frame_rates[];
};

class FrameSizeSelection : public Selection
{
public:
	FrameSizeSelection(int x1, int y1, int x2, int y2,
		BC_WindowBase *base, int *value1, int *value2, int swapvalues = 1);

	void update(int value1, int value2);
	void handle_swapvalues(int value1, int value2);
	static int limits(int *width, int *height);

private:
	static const struct selection_2int frame_sizes[];
};

class AspectRatioSelection : public Selection
{
public:
	AspectRatioSelection(int x1, int y1, int x2, int y2,
		BC_WindowBase *base, double *aspect_ratio,
		int *frame_w, int *frame_h);

	void update_auto(double value1, double value2);
	void update_sar(double sample_aspect_ratio);
	static void auto_aspect_ratio(double *aspect_w, double *aspect_h,
		int width, int height);
	static void aspect_to_wh(double *aspect_w, double *aspect_h,
		double sample_aspect_ratio, int frame_w, int frame_h);
	static int defined_aspect(double *aw, double *ah);
	static int limits(double *aspect);

private:
	int *frame_w;
	int *frame_h;
	double *aspect;
	double waspect;
	double haspect;
	static const struct selection_2double aspect_ratios[];
};


class SwapValues : public BC_Button
{
public:
	SwapValues(int x, int y, FrameSizeSelection *output, int *value1, int *value2);

	int handle_event();

private:
	int *value1;
	int *value2;
	FrameSizeSelection *output;
};


class SelectionLeftBox : public BC_TextBox
{
public:
	SelectionLeftBox(int x, int y, Selection *selection);

	int handle_event();
private:
	Selection *selection;
};


class SampleBitsSelection : public Selection
{
public:
	SampleBitsSelection(int x, int y, BC_WindowBase *base, int *value, int bits);

	int handle_event();
	void update_size(int size);

// Convert samplesize flag to sample bits
	static int samlpesize(int flag);
	static int sampleflag(int size);
private:
	static const struct selection_2int sample_bits[];
};

class OutputDepthSelection : public Selection
{
public:
	OutputDepthSelection(int x, int y, BC_WindowBase *base, int *value);

	static int limits(int *depth);
	void update(int depth);
private:
	static const struct selection_int output_depths[];
};

#endif
