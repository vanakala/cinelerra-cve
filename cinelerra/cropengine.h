// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef CROPENGINE_H
#define CROPENGINE_H

#include "cropengine.inc"
#include "cropautos.inc"
#include "vframe.inc"

class CropEngine
{
public:
	CropEngine();

	VFrame *do_crop(Autos *autos, VFrame *frame, int before);
};

#endif
