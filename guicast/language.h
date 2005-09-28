#ifndef LANGUAGE_H
#define LANGUAGE_H



#include <libintl.h>


#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)



#endif







