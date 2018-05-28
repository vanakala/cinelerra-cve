
/*
 * CINELERRA
 * Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#ifndef PARAMLIST_INC
#define PARAMLIST_INC

class Param;
class Paramlist;

#define PARAM_NAMELEN 40

#define PARAMTYPE_STR 1
#define PARAMTYPE_INT 2
#define PARAMTYPE_LNG 4
#define PARAMTYPE_DBL 8

#define PARAMTYPE_MASK 0xf

#define PARAMTYPE_BITS 0x100
#define PARAMTYPE_BOOL 0x200
#define PARAMTYPE_HIDN 0x400
#define PARAMTYPE_CODK 0x800
#define PARAMTYPE_CHNG 0x8000

#endif
