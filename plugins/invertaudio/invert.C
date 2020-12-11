// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "invert.h"
#include "picon_png.h"

REGISTER_PLUGIN

InvertAudioEffect::InvertAudioEffect(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

InvertAudioEffect::~InvertAudioEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS


AFrame *InvertAudioEffect::process_tmpframe(AFrame *input)
{
	int size = input->get_length();

	for(int i = 0; i < size; i++)
		input->buffer[i] = -input->buffer[i];
	return input;
}
