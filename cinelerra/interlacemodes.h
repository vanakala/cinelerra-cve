/*
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 
 * USA
 */
 
#ifndef INTERLACEMODES_H
#define INTERLACEMODES_H

//Interlace Automatic fixing options
#define BC_ILACE_AUTOFIXOPTION_MANUAL   0
#define BC_ILACE_AUTOFIXOPTION_AUTO     1

//Interlace Modes
#define BC_ILACE_MODE_UNDETECTED         0
#define BC_ILACE_MODE_TOP_FIRST          1
#define BC_ILACE_MODE_BOTTOM_FIRST       2
#define BC_ILACE_MODE_NOTINTERLACED      3

//Note: Do not change what the numbers mean as this will make backward-compatability have erroraneous settings.

//Interlace Compensation Methods
#define BC_ILACE_FIXMETHOD_NONE         0
#define BC_ILACE_FIXMETHOD_UPONE        1
#define BC_ILACE_FIXMETHOD_DOWNONE      2

#endif // INTERLACEMODES_H
