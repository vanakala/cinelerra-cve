// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2015 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef FILEAVLIBS_INC
#define FILEAVLIBS_INC

#define FILEAVLIBS_FORMAT_CONFIG "avlibsfmt:"
#define FILEAVLIBS_CODECS_CONFIG "avlibscdc:"
#define FILEAVLIBS_ACODEC_CONFIG "avlibsacodec:"
#define FILEAVLIBS_VCODEC_CONFIG "avlibsvcodec:"
#define FILEAVLIBS_APRIVT_CONFIG "avlibsacprivate:"
#define FILEAVLIBS_VPRIVT_CONFIG "avlibsvcprivate:"

#define FILEAVLIBS_CONFIG_EXT ".xml"

// Indexes of encoding parameters in asset
#define FILEAVLIBS_FORMAT_IX 0
#define FILEAVLIBS_CODECS_IX 1
#define FILEAVLIBS_ACODEC_IX 2
#define FILEAVLIBS_VCODEC_IX 3
#define FILEAVLIBS_APRIVT_IX 4
#define FILEAVLIBS_VPRIVT_IX 5
#define FILEAVLIBS_METADT_IX 6

// Indexes of decoding parameters in asset
#define FILEAVLIBS_DFORMAT_IX 0

// Current codecs list in asset
#define AVL_PARAM_CODEC_AUDIO "audio"
#define AVL_PARAM_CODEC_VIDEO "video"

// Some more encoder parameters
#define ENC_PIX_FMTS     0
#define ENC_FRAMERATES   1
#define ENC_SAMPLERATES  2
#define ENC_SAMPLE_FMTS  3
#define ENC_LAYOUTS      4

class FileAVlibs;

#endif
