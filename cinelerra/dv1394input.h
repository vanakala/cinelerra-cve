#ifndef DV1394INPUT_H
#define DV1394INPUT_H




class DV1394Input : public Thread
{
public:
	DV1394Input(VideoDevice *device);
	~DV1394Input();

	void start();
	void run();

	int done;
	VideoDevice *device;
	int fd;
};







#endif
