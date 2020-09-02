#ifndef DeviceH
#define DeviceH

#include <string>
#include <deque>
#include <winsock2.h>
#include "Mutex.h"

class Device {
public:
	typedef void (*pfcnReceiveCmd)(std::string cmd, void* opaque);
	typedef void (*pfcnPoll)(void* opaque);
private:
	std::string sAddress;
	int port;
	bool connectRequest;
	bool connected;
	bool connectionLost;

	SOCKET socketHandle;
	HANDLE workerThread;
	HANDLE keepaliveThread;
	bool terminate;
	volatile bool workerTerminated;
	friend DWORD WINAPI DeviceThreadProc(LPVOID lpParameter);
	void WorkerProc(void);

	volatile bool keepaliveTerminated;
	friend DWORD WINAPI ConnKeepaliveThreadProc(LPVOID lpParameter);
	void ConnKeepaliveProc(void);

	Mutex mutex;

	std::string cmd;
	void ProcessCmd(std::string cmd);
	int SendCmd(std::string cmd, bool hideLog);
	int ConnClose(void);
	int waitForReply;

	std::deque<std::string> cmdQueue;
	Mutex mutexQueue;
	pfcnReceiveCmd onReceiveCb;
	void* onReceiveOpaque;

	pfcnPoll onPoll;
	void* onPollOpaque;

	int keepAlivePeriod;	//ms;	 no keepalive if <= 0
	std::string keepAliveString;
	std::string initString;
	bool hideKeepAliveLog;

	int Connect(void);

public:
	Device(void);
	void SetReceiveCmdCb(pfcnReceiveCmd onReceiveCb, void* onReceiveOpaque) {
		this->onReceiveCb = onReceiveCb;
		this->onReceiveOpaque = onReceiveOpaque;
	}
	void SetPollCb(pfcnPoll onPollCb, void* onPollOpaque) {
		this->onPoll = onPollCb;
		this->onPollOpaque = onPollOpaque;
	}
	int Configure(std::string sAddress, int port, std::string initString, int keepAlivePeriod, std::string keepAliveString, bool hideKeepAliveLog);
	int Start(void);
	int Disconnect(void);
	bool isConnected(void) const {
		return connected;
	}
	bool connLost(void) {
		bool val = connectionLost;
		connectionLost = false;
		return val;
	}
	int EnqueueCmd(std::string cmd);
};

#endif
