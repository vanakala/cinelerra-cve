#ifndef RECORDLABELS_H
#define RECORDLABELS_H

#include "labels.inc"
#include "linklist.h"


class RecordLabel : public ListItem<RecordLabel>
{
public:
	RecordLabel() {};
	RecordLabel(double position);
	~RecordLabel();
	
	double position;
};


class RecordLabels : public List<RecordLabel>
{
public:
	RecordLabels();
	~RecordLabels();
	
	int delete_new_labels();
	int toggle_label(double position);
	double get_prev_label(double position);
	double get_next_label(double position);
	double goto_prev_label(double position);
	double goto_next_label(double position);
};

#endif
