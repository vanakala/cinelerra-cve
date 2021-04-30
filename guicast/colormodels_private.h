// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2021 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef COLORMODELS_PRIVATE_H
#define COLORMODELS_PRIVATE_H

extern "C"
{
#include <libavutil/pixdesc.h>
}
AVPixelFormat color_model_to_pix_fmt(int color_model);

#endif
