#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "picon_png.h"
#include "units.h"
#include "vframe.h"

#include <math.h>
#include <string.h>




#define WINDOW_SIZE 16384
#define MAXMAGNITUDE 15





class GraphicEQ : public PluginAClient
{
public:
	GraphicEQ(PluginServer *server);
	~GraphicEQ();

	VFrame* new_picon();
	char* plugin_title();
	int is_realtime();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_realtime(long size, double *input_ptr, double *output_ptr);
	int show_gui();
	void raise_window();
	int set_string();





	int load_defaults();
	int save_defaults();
	void load_configuration();
	void reset();
	void reconfigure();
	void update_gui();

	double calculate_envelope();
	double gauss(double sigma, double a, double x);

	double envelope[WINDOW_SIZE / 2];
	int need_reconfigure;
	BC_Hash *defaults;
	GraphicThread *thread;
	GraphicFFT *fft;
	GraphicConfig config;
};











PluginClient* new_plugin(PluginServer *server)
{
	return new GraphicEQ(server);
}


