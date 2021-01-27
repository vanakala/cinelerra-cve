// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef PARAMLIST_INC
#define PARAMLIST_INC

class Param;
class Paramlist;
struct paramlist_defaults;

#define PARAM_NAMELEN 40

#define PARAMTYPE_STR 1
#define PARAMTYPE_INT 2
#define PARAMTYPE_LNG 4
#define PARAMTYPE_DBL 8

#define PARAMTYPE_MASK 0xf

#define PARAMTYPE_SUBS 0x80
#define PARAMTYPE_BITS 0x100
#define PARAMTYPE_BOOL 0x200
#define PARAMTYPE_HIDN 0x400
#define PARAMTYPE_CODK 0x800
#define PARAMTYPE_CHNG 0x8000

// Extension of the onfiguration file
#define XML_CONFIG_EXT ".xml"

#endif
