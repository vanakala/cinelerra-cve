#include "file.h"
#include "vdevicebase.h"
#include "videodevice.h"
#include "recordconfig.h"

VDeviceBase::VDeviceBase(VideoDevice *device)
{
	this->device = device;
}

VDeviceBase::~VDeviceBase()
{
}


int VDeviceBase::get_best_colormodel(Asset *asset)
{
	return File::get_best_colormodel(asset, device->in_config->driver);
}
