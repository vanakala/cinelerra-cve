#ifndef PACKAGEDISPATCHER_H
#define PACKAGEDISPATCHER_H


#include "arraylist.h"
#include "assets.inc"
#include "edl.inc"
#include "mutex.inc"
#include "mwindow.inc"
#include "packagerenderer.inc"
#include "preferences.inc"



// Allocates fragments given a total start and total end.
// Checks the existence of every file.
// Adjusts package size for load.
class PackageDispatcher
{
public:
	PackageDispatcher();
	~PackageDispatcher();

	int create_packages(MWindow *mwindow,
		EDL *edl,
		Preferences *preferences,
		int strategy, 
		Asset *default_asset, 
		double total_start, 
		double total_end,
		int test_overwrite);
// Supply a frame rate of the calling node.  If the client number is -1
// the frame rate isn't added to the preferences table.
	RenderPackage* get_package(double frames_per_second, 
		int client_number,
		int use_local_rate);
	ArrayList<Asset*>* get_asset_list();
	RenderPackage* get_package(int number);
	int get_total_packages();

	EDL *edl;
	int64_t audio_position;
	int64_t video_position;
	int64_t audio_end;
	int64_t video_end;
	double total_start;
	double total_end;
	double total_len;
	int strategy;
	Asset *default_asset;
	Preferences *preferences;
	int current_number;    // The number being injected into the filename.
	int number_start;      // Character in the filename path at which the number begins
	int total_digits;      // Total number of digits including padding the user specified.
	double package_len;    // Target length of a single package
	double min_package_len; // Minimum package length after load balancing
	int64_t total_packages;   // Total packages to base calculations on
	int64_t total_allocated;  // Total packages to test the existence of
	int nodes;
	MWindow *mwindow;
	RenderPackage **packages;
	int current_package;
	Mutex *package_lock;
};


#endif


