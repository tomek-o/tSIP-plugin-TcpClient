//---------------------------------------------------------------------------

#ifndef DeviceHandlerH
#define DeviceHandlerH
//---------------------------------------------------------------------------

#include <string>

class Device;

class DeviceHandler
{
public:
	DeviceHandler(Device &device);
protected:
	Device &device;
	bool initialVarSet;
	bool connected;
	bool connectedInitialized;
	
	friend void OnDeviceCmdReceive(std::string cmd, void* opaque);
	void OnDeviceCmdRx(std::string cmd);

	friend void OnPeriodicPoll(void* opaque);
	void OnPoll(void);
};

#endif
