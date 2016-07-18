
/*
 * CINELERRA
 * Copyright (C) 2015 Einar Rünkaru <einarrunkaru@gmail dot com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef FILEAVLIBS_INC
#define FILEAVLIBS_INC

#define FILEAVLIBS_GLOBAL_CONFIG "avlibsconfig"
#define FILEAVLIBS_FORMAT_CONFIG "avlibsfmt:"
#define FILEAVLIBS_CODECS_CONFIG "avlibscdc:"
#define FILEAVLIBS_ACODEC_CONFIG "avlibsacodec:"
#define FILEAVLIBS_VCODEC_CONFIG "avlibsvcodec:"
#define FILEAVLIBS_APRIVT_CONFIG "avlibsacprivate:"
#define FILEAVLIBS_VPRIVT_CONFIG "avlibsvcprivate:"

#define FILEAVLIBS_CONFIG_EXT ".xml"

// Indexes of encoding parameters in asset
#define FILEAVLIBS_GLOBAL_IX 0
#define FILEAVLIBS_FORMAT_IX 1
#define FILEAVLIBS_CODECS_IX 2
#define FILEAVLIBS_ACODEC_IX 3
#define FILEAVLIBS_VCODEC_IX 4
#define FILEAVLIBS_APRIVT_IX 5
#define FILEAVLIBS_VPRIVT_IX 6

class FileAVlibs;

#endif
