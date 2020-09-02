//---------------------------------------------------------------------------

#define _EXPORTING
#include "..\..\tSIP\tSIP\phone\Phone.h"
#include "..\..\tSIP\tSIP\phone\PhoneSettings.h"
#include "..\..\tSIP\tSIP\phone\PhoneCapabilities.h"
#include "PhoneLocal.h"
#include "Log.h"
#include <assert.h>
#include <algorithm>	// needed by Utils::in_group
#include "Utils.h"
#include "Settings.h"
#include "Device.h"
#include "DeviceHandler.h"
#include <string>

Device device;
DeviceHandler deviceHandler(device);

//---------------------------------------------------------------------------

static const struct S_PHONE_DLL_INTERFACE dll_interface =
{DLL_INTERFACE_MAJOR_VERSION, DLL_INTERFACE_MINOR_VERSION};

// callback ptrs
CALLBACK_LOG lpLogFn = NULL;
CALLBACK_CONNECT lpConnectFn = NULL;
CALLBACK_KEY lpKeyFn = NULL;
CALLBACK_PAGING_TX lpPagingTxFn = NULL;
CALLBACK_CLEAR_DIAL lpClearDialFn = NULL;

CALLBACK_SET_VARIABLE lpSetVariableFn = NULL;
CALLBACK_CLEAR_VARIABLE lpClearVariableFn = NULL;

CALLBACK_QUEUE_PUSH lpQueuePushFn = NULL;
CALLBACK_QUEUE_CLEAR lpQueueClearFn = NULL;
CALLBACK_QUEUE_GET_SIZE lpQueueGetSizeFn = NULL;

void *callbackCookie;	///< used by upper class to distinguish library instances when receiving callbacks

void OnDeviceCmdReceive(std::string cmd, void* opaque) {
	(reinterpret_cast<DeviceHandler*>(opaque))->OnDeviceCmdRx(cmd);
}

void OnPeriodicPoll(void* opaque) {
	(reinterpret_cast<DeviceHandler*>(opaque))->OnPoll();
}

namespace {
	std::string configPath = Utils::ReplaceFileExtension(Utils::GetDllPath(), ".cfg");
}

void __stdcall GetPhoneInterfaceDescription(struct S_PHONE_DLL_INTERFACE* interf) {
    interf->majorVersion = dll_interface.majorVersion;
    interf->minorVersion = dll_interface.minorVersion;
}

void Log(char* txt) {
    if (lpLogFn)
        lpLogFn(callbackCookie, txt);
}

void Connect(int state, char *szMsgText) {
    if (lpConnectFn)
        lpConnectFn(callbackCookie, state, szMsgText);
}

void Key(int keyCode, int state) {
    if (lpKeyFn)
        lpKeyFn(callbackCookie, keyCode, state);
}

int PagingTx(const char* target, const char* filename, const char* codecname) {
	if (lpPagingTxFn) {
		return lpPagingTxFn(callbackCookie, target, filename, codecname);
	}
	return -1;
}

int ClearDial(void) {
	if (lpClearDialFn) {
		lpClearDialFn(callbackCookie);
		return 0;
	}
    return -1;
}

int SetVariable(const char* name, const char* value) {
	if (lpSetVariableFn) {
		//lpLogFn(callbackCookie, name);
		return lpSetVariableFn(callbackCookie, name, value);
	}
	return -1;
}

int ClearVariable(const char* name) {
	if (lpClearVariableFn) {
		return lpClearVariableFn(callbackCookie, name);
	}
	return -1;
}

int QueuePush(const char* name, const char* value) {
	if (lpQueuePushFn) {
		return lpQueuePushFn(callbackCookie, name, value);
	}
	return -1;
}

int QueueClear(const char* name) {
	if (lpQueueClearFn) {
		return lpQueueClearFn(callbackCookie, name);
	}
	return -1;
}

int QueueGetSize(const char* name) {
	if (lpQueueGetSizeFn) {
        return lpQueueGetSizeFn(callbackCookie, name);
    }
	return 0;
}

void __stdcall SetCallbacks(void *cookie, CALLBACK_LOG lpLog, CALLBACK_CONNECT lpConnect, CALLBACK_KEY lpKey) {
    assert(cookie && lpLog && lpConnect && lpKey);
    lpLogFn = lpLog;
    lpConnectFn = lpConnect;
    lpKeyFn = lpKey;
    callbackCookie = cookie;
	lpLogFn(callbackCookie, "Phone DLL for EX03 loaded\n");
}

void __stdcall GetPhoneCapabilities(struct S_PHONE_CAPABILITIES **caps) {
    static struct S_PHONE_CAPABILITIES capabilities = {
        0
    };
    *caps = &capabilities;
}

void __stdcall ShowSettings(HANDLE parent) {
    MessageBox((HWND)parent, "No additional settings.", "Device DLL", MB_ICONINFORMATION);
}

int __stdcall Connect(void) {
	appSettings.Read(configPath.c_str());
	LOG("Setting loaded");
#if 0
	if (frmMain == NULL) {
		frmMain = new TfrmMain(NULL);
		frmMain->Visible = true;
		frmMain->Show();
	}
#endif
	device.SetReceiveCmdCb(OnDeviceCmdReceive, &deviceHandler);
	device.SetPollCb(OnPeriodicPoll, &deviceHandler);
	device.Configure(appSettings.Device.asAddress.c_str(), appSettings.Device.port,
		appSettings.Device.initCmd.c_str(),
		appSettings.Device.keepalivePeriod,
		appSettings.Device.keepaliveCmd.c_str(),
		appSettings.Device.bHideKeepAlive);
	return device.Start();
}

int __stdcall Disconnect(void) {
	appSettings.Write(configPath.c_str());
#if 0
	if (frmMain) {
		frmMain->Break();
		while (frmMain->isScriptRunning()) {
			Application->ProcessMessages();
			Sleep(100);
		}
		delete frmMain;
		frmMain = NULL;
	}
#endif
	device.Disconnect();	
    return 0;
}

static bool bSettingsReaded = false;

static int GetDefaultSettings(struct S_PHONE_SETTINGS* settings) {
    settings->ring = 1;

    bSettingsReaded = true;
    return 0;
}

int __stdcall GetPhoneSettings(struct S_PHONE_SETTINGS* settings) {
    //settings->iTriggerSrcChannel = 0;

    std::string path = Utils::GetDllPath();
    path = Utils::ReplaceFileExtension(path, ".cfg");
    if (path == "")
        return GetDefaultSettings(settings);


    GetDefaultSettings(settings);

    settings->ring = true;//root.get("ring", settings->ring).asInt();


    bSettingsReaded = true;
    return 0;
}

int __stdcall SavePhoneSettings(struct S_PHONE_SETTINGS* settings) {

    return 0;
}

int __stdcall SetRegistrationState(int state) {
	//phoneState.registered = state;
    return 0;
}

int __stdcall SetCallState(int state, const char* display) {
	//phoneState.callState = state;
	//phoneState.display = display;
	return 0;
}


int __stdcall Ring(int state) {
    return 0;
}

void __stdcall SetPagingTxCallback(CALLBACK_PAGING_TX lpPagingTx) {
	lpPagingTxFn = lpPagingTx;
}

int __stdcall SetPagingTxState(int state) {
	//phoneState.paging = state;
	return 0;
}

void __stdcall SetClearDialCallback(CALLBACK_CLEAR_DIAL lpClearDial) {
	lpClearDialFn = lpClearDial;
}

void __stdcall SetSetVariableCallback(CALLBACK_SET_VARIABLE lpFn) {
	lpSetVariableFn = lpFn;
}

void __stdcall SetClearVariableCallback(CALLBACK_CLEAR_VARIABLE lpFn) {
	lpClearVariableFn = lpFn;
}

void __stdcall SetQueuePushCallback(CALLBACK_QUEUE_PUSH lpFn) {
	lpQueuePushFn = lpFn;
}

void __stdcall SetQueueClearCallback(CALLBACK_QUEUE_CLEAR lpFn) {
    lpQueueClearFn = lpFn;
}

void __stdcall SetQueueGetSizeCallback(CALLBACK_QUEUE_GET_SIZE lpFn) {
    lpQueueGetSizeFn = lpFn;
}

inline bool StartsWith(const char* &a, const char *b)
{
	if(strlen(a) < strlen(b)) {
		return false;
	}
	else {
		int len = strlen(b);
		bool result = !strnicmp(a,b,len); //case insensitive
		if(result)
			a += len;
		return result;
	}
}

int __stdcall SendMessageText(const char* text) {
	LOG("received message: %s", text);

	if (StartsWith(text, "SEND ")) {
		if (device.isConnected() == false) {
			return -1;
		}
		device.EnqueueCmd(text);
	}

	return 0;
}

