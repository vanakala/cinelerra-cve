#ifndef FORMATCHECK_H
#define FORMATCHECK_H

#include "assets.inc"

class FormatCheck
{
public:
	FormatCheck(Asset *asset);
	~FormatCheck();

	int check_format();

private:
	Asset *asset;
};


#endif
