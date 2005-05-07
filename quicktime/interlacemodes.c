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
#include "interlacemodes.h"
#include <stdlib.h>

void ilaceautofixoption_to_text(char *string, int autofixoption)
{
	switch(autofixoption)
	{
		case BC_ILACE_AUTOFIXOPTION_AUTO:  	strcpy(string, BC_ILACE_AUTOFIXOPTION_AUTO_T);  	break;
		case BC_ILACE_AUTOFIXOPTION_MANUAL:	strcpy(string, BC_ILACE_AUTOFIXOPTION_MANUAL_T);	break;
		default: strcpy(string, BC_ILACE_UNKNOWN_T);	break;
	}
}

int ilaceautofixoption_from_text(char *text, int thedefault)
{
	if(!strcasecmp(text, BC_ILACE_AUTOFIXOPTION_AUTO_T))  	return BC_ILACE_AUTOFIXOPTION_AUTO;
	if(!strcasecmp(text, BC_ILACE_AUTOFIXOPTION_MANUAL_T))	return BC_ILACE_AUTOFIXOPTION_MANUAL;
	return thedefault;
}

void ilacemode_to_text(char *string, int ilacemode)
{
	switch(ilacemode)
	{
		case BC_ILACE_MODE_UNDETECTED:   	strcpy(string, BC_ILACE_MODE_UNDETECTED_T);   	break;
		case BC_ILACE_MODE_ODDLEADS:     	strcpy(string, BC_ILACE_MODE_ODDLEADS_T);     	break;
		case BC_ILACE_MODE_EVENLEADS:    	strcpy(string, BC_ILACE_MODE_EVENLEADS_T);    	break;
		case BC_ILACE_MODE_NOTINTERLACED:	strcpy(string, BC_ILACE_MODE_NOTINTERLACED_T);	break;
		default: strcpy(string, BC_ILACE_UNKNOWN_T); break;
	}
}

int ilacemode_from_text(char *text, int thedefault)
{
	if(!strcasecmp(text, BC_ILACE_MODE_UNDETECTED_T))   	return BC_ILACE_MODE_UNDETECTED;
	if(!strcasecmp(text, BC_ILACE_MODE_ODDLEADS_T))     	return BC_ILACE_MODE_ODDLEADS;
	if(!strcasecmp(text, BC_ILACE_MODE_EVENLEADS_T))    	return BC_ILACE_MODE_EVENLEADS;
	if(!strcasecmp(text, BC_ILACE_MODE_NOTINTERLACED_T))	return BC_ILACE_MODE_NOTINTERLACED;
	return thedefault;
}

void ilacefixmethod_to_text(char *string, int fixmethod)
{
	switch(fixmethod)
	{
		case BC_ILACE_FIXMETHOD_NONE:   	strcpy(string, BC_ILACE_FIXMETHOD_NONE_T);   	break;
		case BC_ILACE_FIXMETHOD_UPONE:  	strcpy(string, BC_ILACE_FIXMETHOD_UPONE_T);  	break;
		case BC_ILACE_FIXMETHOD_DOWNONE:	strcpy(string, BC_ILACE_FIXMETHOD_DOWNONE_T);	break;
		default: strcpy(string, BC_ILACE_UNKNOWN_T);	break;
	}
}

int ilacefixmethod_from_text(char *text, int thedefault)
{
	if(!strcasecmp(text, BC_ILACE_FIXMETHOD_NONE_T))   	return BC_ILACE_FIXMETHOD_NONE;
	if(!strcasecmp(text, BC_ILACE_FIXMETHOD_UPONE_T))  	return BC_ILACE_FIXMETHOD_UPONE;
	if(!strcasecmp(text, BC_ILACE_FIXMETHOD_DOWNONE_T))	return BC_ILACE_FIXMETHOD_DOWNONE;
	return thedefault; 
}

int  ilaceautofixmethod(int projectmode, int assetmode) 
{
  if (projectmode == assetmode)
    return BC_ILACE_FIXMETHOD_NONE;
      
  if (
      (projectmode == BC_ILACE_MODE_EVENLEADS && assetmode == BC_ILACE_MODE_ODDLEADS )
      ||
      (projectmode == BC_ILACE_MODE_ODDLEADS  && assetmode == BC_ILACE_MODE_EVENLEADS)
      )
    return BC_ILACE_FIXDEFAULT;

  // still to implement anything else...
   return BC_ILACE_FIXMETHOD_NONE;
}

int  ilaceautofixmethod2(int projectilacemode, int assetautofixoption, int assetilacemode, int assetfixmethod)
{
	if (assetautofixoption == BC_ILACE_AUTOFIXOPTION_AUTO)
		return (ilaceautofixmethod(projectilacemode, assetilacemode));
	else
		return (assetfixmethod);
}
