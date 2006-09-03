#ifndef FILECR2_H
#define FILECR2_H


#include "filebase.h"


// This uses a program from a guy called Coffin to do the decoding.
// Changes to the dcraw.c file were commented with // Cinelerra

// The expected behavior for the dcraw function:
// -i <path>
// When the file is recognized, return 0.
// When the file is unknown, return 1.

// <path>
// Decode the file.

class FileCR2 : public FileBase
{
public:
	FileCR2(Asset *asset, File *file);
	~FileCR2();

	void reset();
	static int check_sig(Asset *asset);

// Open file and set asset properties but don't decode.
	int open_file(int rd, int wr);
	int close_file();
// Open file and decode.
	int read_frame(VFrame *frame);
// Get best colormodel for decoding.
	int colormodel_supported(int colormodel);

private:
	void format_to_asset();
};


#endif
