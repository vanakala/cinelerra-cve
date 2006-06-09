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

#define BC_ILACE_UNKNOWN_T      "Error!"

//Interlace Automatic fixing options
#define BC_ILACE_AUTOFIXOPTION_MANUAL  	0
#define BC_ILACE_AUTOFIXOPTION_MANUAL_T	"Manual compensation using selection"
#define BC_ILACE_AUTOFIXOPTION_AUTO    	1
#define BC_ILACE_AUTOFIXOPTION_AUTO_T  	"Automatic compensation using modes"
//Note: Do not change what the numbers mean as this will make backward-compatability have erroraneous settings.

//Interlace Modes
#define BC_ILACE_MODE_UNDETECTED         0
#define BC_ILACE_MODE_UNDETECTED_XMLT    "UNKNOWN"
#define BC_ILACE_MODE_UNDETECTED_T       "Unknown"
#define BC_ILACE_MODE_TOP_FIRST          1
#define BC_ILACE_MODE_TOP_FIRST_XMLT     "TOP_FIELD_FIRST"
#define BC_ILACE_MODE_TOP_FIRST_T        "Top Fields First"
#define BC_ILACE_MODE_BOTTOM_FIRST       2
#define BC_ILACE_MODE_BOTTOM_FIRST_XMLT  "BOTTOM_FIELD_FIRST"
#define BC_ILACE_MODE_BOTTOM_FIRST_T     "Bottom Fields First"
#define BC_ILACE_MODE_NOTINTERLACED      3
#define BC_ILACE_MODE_NOTINTERLACED_XMLT "NOTINTERLACED"
#define BC_ILACE_MODE_NOTINTERLACED_T    "Not Interlaced"

#define BC_ILACE_ASSET_MODEDEFAULT  	BC_ILACE_MODE_UNDETECTED
#define BC_ILACE_PROJECT_MODEDEFAULT	BC_ILACE_MODE_NOTINTERLACED_T
//Note: Do not change what the numbers mean as this will make backward-compatability have erroraneous settings.

//Interlace Compensation Methods
#define BC_ILACE_FIXMETHOD_NONE     	0
#define BC_ILACE_FIXMETHOD_NONE_XMLT   	"DO_NOTHING"
#define BC_ILACE_FIXMETHOD_NONE_T   	"Do Nothing"
#define BC_ILACE_FIXMETHOD_UPONE    	1
#define BC_ILACE_FIXMETHOD_UPONE_XMLT  	"SHIFT_UPONE"
#define BC_ILACE_FIXMETHOD_UPONE_T  	"Shift Up 1 pixel"
#define BC_ILACE_FIXMETHOD_DOWNONE  	2
#define BC_ILACE_FIXMETHOD_DOWNONE_XMLT	"SHIFT_DOWNONE"
#define BC_ILACE_FIXMETHOD_DOWNONE_T	"Shift Down 1 pixel"

// the following is for project/asset having odd/even, or even/odd  
#define BC_ILACE_FIXDEFAULT         	BC_ILACE_FIXMETHOD_UPONE
//Note: Do not change what the numbers mean as this will make backward-compatability have erroraneous settings.

// Refer to <mjpegtools/yuv4mpeg.h> (descriptions were cut-and-pasted!)
#define BC_ILACE_Y4M_UKNOWN_T         "unknown"
#define BC_ILACE_Y4M_NONE_T           "non-interlaced, progressive frame"
#define BC_ILACE_Y4M_TOP_FIRST_T      "interlaced, top-field first"
#define BC_ILACE_Y4M_BOTTOM_FIRST_T   "interlaced, bottom-field first"
#define BC_ILACE_Y4M_MIXED_T          "mixed, \"refer to frame header\""

#ifdef __cplusplus
extern "C" {
#endif

void ilaceautofixoption_to_text(char *string, int autofixoption);
int  ilaceautofixoption_from_text(char *text, int thedefault);

void ilacemode_to_text(char *string, int ilacemode);
int  ilacemode_from_text(char *text, int thedefault);
void ilacemode_to_xmltext(char *string, int ilacemode);
int  ilacemode_from_xmltext(char *text, int thedefault);

void ilacefixmethod_to_text(char *string, int fixmethod);
int  ilacefixmethod_from_text(char *text, int thedefault);
void ilacefixmethod_to_xmltext(char *string, int fixmethod);
int  ilacefixmethod_from_xmltext(char *text, int thedefault);


int  ilaceautofixmethod(int projectilacemode, int assetilacemode);
int  ilaceautofixmethod2(int projectilacemode, int assetautofixoption, int assetilacemode, int assetfixmethod);

int ilace_bc_to_yuv4mpeg(int ilacemode);
int ilace_yuv4mpeg_to_bc(int ilacemode);

void ilace_yuv4mpeg_mode_to_text(char *string, int ilacemode);

#ifdef __cplusplus
}
#endif

#endif // INTERLACEMODES_H
