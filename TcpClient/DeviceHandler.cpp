//---------------------------------------------------------------------------


#pragma hdrstop

#include "DeviceHandler.h"
#include "Device.h"
#include "PhoneLocal.h"
#include "Settings.h"
#include "Utils.h"
#include <stdio.h>

//---------------------------------------------------------------------------

#pragma package(smart_init)

#include "Log.h"

namespace {
	std::string varBaseName;
	std::string varConnName;
	std::string varConnLostName;
	std::string varInitializedName;

	std::string queueBaseName;
	std::string queueMsgRx;

	inline void SkipSpaces(const char* &a)
	{
		while (a[0] == ' ') {
			a++;
		}
	}

	inline bool StartsWith(const char* &a, const char *b)
	{
		int len = strlen(b);
		bool result = !strnicmp(a,b,len); //case insensitive
		if(result)
			a += len;
		SkipSpaces(a);
		return result;
	}

	inline bool SkipParam(const char* &a)
	{
		bool result = false;
		SkipSpaces(a);
		while (a[0] != ' ' && a[0] != '\0') {
			a++;
			result = true;
		}
		SkipSpaces(a);
		return result;
	}
}


DeviceHandler::DeviceHandler(Device &device):
	device(device),
	initialVarSet(false),
	connected(false),
	connectedInitialized(false)
{
	std::string path = Utils::GetDllPath();
	std::string dllName = Utils::ExtractFileNameWithoutExtension(path);
	varBaseName = dllName;
	#pragma warn -8091	// incorrectly issued by BDS2006
	std::transform(varBaseName.begin(), varBaseName.end(), varBaseName.begin(), tolower);
	varConnName = varBaseName + "Conn";
	varConnLostName = varBaseName + "ConnLost";
	varInitializedName = varBaseName + "Initialized";

	queueBaseName = dllName;
	queueMsgRx = queueBaseName + "MsgRx";
}

void DeviceHandler::OnDeviceCmdRx(std::string cmd)
{
	bool hideLog = false;
//	if (appSettings.Device.bHideKeepAlive && !strncmp(cmd.c_str(), "OK mtrstart ", strlen("OK mstrstart "))) {
//		hideLog = true;
//	}

	if (hideLog == false) {
		LOG("RX: %s", cmd.c_str());
	}
	const char *ptr = cmd.c_str();

	if (QueueGetSize(queueMsgRx.c_str()) < 2000) {	// preventing unlimited queue grow if second side is not receiving  
		QueuePush(queueMsgRx.c_str(), ptr);
	}
}

void DeviceHandler::OnPoll(void)
{
	if (!initialVarSet)
	{
		initialVarSet = true;
		SetVariable(varConnName.c_str(), connected?"1":"0");
		SetVariable(varInitializedName.c_str(), connectedInitialized?"1":"0");		
		SetVariable(varConnLostName.c_str(), "0");
	}
	if (connected != device.isConnected() || !connectedInitialized)
	{
		connected = device.isConnected();
		connectedInitialized = true;
		if (connected)
		{
			LOG("Connected\n");
			SetVariable(varConnName.c_str(), "1");
		}
		else
		{
			LOG("Disconnected\n");
			SetVariable(varConnName.c_str(), "0");
			SetVariable(varInitializedName.c_str(), "0");
			SetVariable(varConnLostName.c_str(), "1");			
		}
	}
	else if (device.connLost())
	{
		SetVariable(varInitializedName.c_str(), "0");
		SetVariable(varConnLostName.c_str(), "1");
	}
}
