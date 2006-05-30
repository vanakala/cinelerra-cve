#ifndef PACKAGEDISPATCHER_H
#define PACKAGEDISPATCHER_H


#include "arraylist.h"
#include "assets.inc"
#include "edl.inc"
#include "mutex.inc"
#include "mwindow.inc"
#include "packagerenderer.inc"
#include "preferences.inc"


class PackagingEngine;

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
	void get_package_paths(ArrayList<char*> *path_list);

	int get_total_packages();
	int64_t get_progress_max();
	int packages_are_done();
	
private:
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

	PackagingEngine *packaging_engine;
};


// Classes used for different packaging strategies, which allow for customary splitting of packages
// Used for renderfarm jobs

class PackagingEngine
{
public:
	virtual int create_packages_single_farm(
		EDL *edl,
		Preferences *preferences,
		Asset *default_asset, 
		double total_start, 
		double total_end) = 0;
	virtual RenderPackage* get_package_single_farm(double frames_per_second, 
		int client_number,
		int use_local_rate) = 0;
	virtual int64_t get_progress_max() = 0;
	virtual void get_package_paths(ArrayList<char*> *path_list) = 0;
	virtual int packages_are_done() = 0;
};

class PackagingEngineDefault : public PackagingEngine
{
public:
	PackagingEngineDefault();
	~PackagingEngineDefault();
	int create_packages_single_farm(
		EDL *edl,
		Preferences *preferences,
		Asset *default_asset, 
		double total_start, 
		double total_end);
	RenderPackage* get_package_single_farm(double frames_per_second, 
		int client_number,
		int use_local_rate);
	int64_t get_progress_max();
	void get_package_paths(ArrayList<char*> *path_list);
	int packages_are_done();
private:
	RenderPackage **packages;
	int64_t total_allocated;  // Total packages to test the existence of
	int current_number;    // The number being injected into the filename.
	int number_start;      // Character in the filename path at which the number begins
	int total_digits;      // Total number of digits including padding the user specified.
	double package_len;    // Target length of a single package
	double min_package_len; // Minimum package length after load balancing
	int64_t total_packages;   // Total packages to base calculations on
	int64_t audio_position;
	int64_t video_position;
	int64_t audio_end;
	int64_t video_end;
	int current_package;
	Asset *default_asset;
	Preferences *preferences;
	double total_start;
	double total_end;
};

class PackagingEngineOgg : public PackagingEngine
{
public:
	PackagingEngineOgg();
	~PackagingEngineOgg();
	int create_packages_single_farm(
		EDL *edl,
		Preferences *preferences,
		Asset *default_asset, 
		double total_start, 
		double total_end);
	RenderPackage* get_package_single_farm(double frames_per_second, 
		int client_number,
		int use_local_rate);
	int64_t get_progress_max();
	void get_package_paths(ArrayList<char*> *path_list);
	int packages_are_done();

private:
	EDL *edl;

	RenderPackage **packages;
	int total_packages;
	double video_package_len;    // Target length of a single package

	Asset *default_asset;
	Preferences *preferences;
	int current_package;
	double total_start;
	double total_end;
	int64_t audio_position;
	int64_t video_position;
	int64_t audio_start;
	int64_t video_start;
	int64_t audio_end;
	int64_t video_end;

};


#endif


