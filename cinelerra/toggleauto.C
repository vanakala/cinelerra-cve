#include "intauto.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

IntAuto::IntAuto(IntAutos *autos)
 : Auto((Autos*)autos)
{
}

IntAuto::~IntAuto()
{
}

int IntAuto::value_to_str(char *string, float value)
{
		if(value > 0) sprintf(string, _("ON"));
		else sprintf(string, _("OFF"));
}

void IntAuto::copy(int64_t start, int64_t end, FileXML *file, int default_auto)
{
	
}

void IntAuto::load(FileXML *file)
{
}
