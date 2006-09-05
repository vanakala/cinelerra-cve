#include "asset.h"
#include "bchash.h"
#include "clip.h"
#include "colormodels.h"
#include "file.h"
#include "filecr2.h"
#include "mutex.h"
#include <string.h>
#include <unistd.h>

// Only allow one instance of the decoder to run simultaneously.
static Mutex cr2_mutex("cr2_mutex");

extern "C"
{
extern char dcraw_info[1024];
extern float **dcraw_data;
extern int dcraw_alpha;
extern float dcraw_matrix[9];
int dcraw_main (int argc, char **argv);
}


FileCR2::FileCR2(Asset *asset, File *file)
 : FileBase(asset, file)
{
	reset();
	if(asset->format == FILE_UNKNOWN)
		asset->format = FILE_CR2;
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

// Want to disable interpolation if an interpolation plugin is on, but
// this is impractical because of the amount of caching.  The interpolation
// could not respond to a change in the plugin settings and it could not
// reload the frame after the plugin was added.  Also, since an 8 bit
// PBuffer would be required, it could never have enough resolution.
//	int interpolate = 0;
// 	if(!strcmp(frame->get_next_effect(), "Interpolate Pixels"))
// 		interpolate = 0;


// printf("FileCR2::read_frame %d\n", interpolate);
// frame->dump_stacks();
// output to stdout
	int argc = 0;
	char *argv[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	argv[argc++] = "dcraw";
// write to stdout
	argv[argc++] = "-c";
// Use camera white balance.  
// Before 2006, DCraw had no Canon white balance.
// In 2006 DCraw seems to support Canon white balance.
// Still no gamma support.
// Need to toggle this in preferences because it defeats dark frame subtraction.
	if(file->white_balance_raw)
		argv[argc++] = "-w";
	if(!file->interpolate_raw)
	{
// Trying to do everything but interpolate doesn't work because convert_to_rgb
// doesn't work with bayer patterns.
// Use document mode and hack dcraw to apply white balance in the write_ function.
		argv[argc++] = "-d";
	}

	argv[argc++] = asset->path;

	dcraw_data = (float**)frame->get_rows();

//Timer timer;
	int result = dcraw_main(argc, argv);

	char string[BCTEXTLEN];
	sprintf(string, 
		"%f %f %f %f %f %f %f %f %f\n",
		dcraw_matrix[0],
		dcraw_matrix[1],
		dcraw_matrix[2],
		dcraw_matrix[3],
		dcraw_matrix[4],
		dcraw_matrix[5],
		dcraw_matrix[6],
		dcraw_matrix[7],
		dcraw_matrix[8]);


	frame->get_params()->update("DCRAW_MATRIX", string);

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








