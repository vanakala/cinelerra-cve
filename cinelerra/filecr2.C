#include "asset.h"
#include "colormodels.h"
#include "filecr2.h"
#include "mutex.h"
#include <string.h>
#include <unistd.h>

// Only allow one instance of the decoder to run simultaneously.
static Mutex cr2_mutex;


FileCR2::FileCR2(Asset *asset, File *file)
 : FileBase(asset, file)
{
	reset();
}

FileCR2::~FileCR2()
{
	close_file();
}


void FileCR2::reset()
{
}

int FileCR2::check_sig(Asset *asset)
{
	cr2_mutex.lock("FileCR2::check_sig");
	char string[BCTEXTLEN];
	int argc = 3;

	strcpy(string, asset->path);

	char *argv[4];
	argv[0] = "dcraw";
	argv[1] = "-i";
	argv[2] = string;
	argv[3] = 0;

	int result = dcraw_main(argc, argv);

	cr2_mutex.unlock();

	return !result;
}

int FileCR2::open_file(int rd, int wr)
{
	cr2_mutex.lock("FileCR2::check_sig");

	int argc = 3;
	char *argv[3] = 
	{
		"dcraw",
		"-i",
		asset->path
	};

	int result = dcraw_main(argc, argv);
	if(!result) format_to_asset();

	cr2_mutex.unlock();
	return result;
}


int FileCR2::close_file()
{
	return 0;
}

void FileCR2::format_to_asset()
{
	asset->video_data = 1;
	asset->layers = 1;
	sscanf(dcraw_info, "%d %d", &asset->width, &asset->height);
	if(!asset->frame_rate) asset->frame_rate = 1;
	asset->video_length = -1;
}


int FileCR2::read_frame(VFrame *frame)
{
	cr2_mutex.lock("FileCR2::read_frame");
	if(frame->get_color_model() == BC_RGBA_FLOAT)
		dcraw_alpha = 1;
	else
		dcraw_alpha = 0;



// printf("FileCR2::read_frame %d\n", interpolate);
// frame->dump_stacks();
// output to stdout
	int argc = 3;
	char *argv[3] = 
	{
		"dcraw",
		"-c",
		asset->path
	};
	dcraw_data = (float**)frame->get_rows();

	int result = dcraw_main(argc, argv);

// printf("FileCR2::read_frame\n");
// frame->dump_params();

	cr2_mutex.unlock();
	return 0;
}

int FileCR2::colormodel_supported(int colormodel)
{
	if(colormodel == BC_RGB_FLOAT ||
		colormodel == BC_RGBA_FLOAT)
		return colormodel;
	return BC_RGB_FLOAT;
}








