#include "picon_png.h"
#include "pluginaclient.h"
#include "vframe.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

class InvertAudioEffect : public PluginAClient
{
public:
	InvertAudioEffect(PluginServer *server)
	 : PluginAClient(server)
	{
	};
	~InvertAudioEffect()
	{
	};

	VFrame* new_picon()
	{
		return new VFrame(picon_png);
	};
	char* plugin_title()
	{
		return _("Invert Audio");
	};
	int is_realtime()
	{
		return 1;
	};
	int process_realtime(int64_t size, double *input_ptr, double *output_ptr)
	{
		for(int i = 0; i < size; i++)
			output_ptr[i] = -input_ptr[i];
		return 0;
	};
};




REGISTER_PLUGIN(InvertAudioEffect)

