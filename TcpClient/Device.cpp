#include "Device.h"
#include "Log.h"
#include "ScopedLock.h"
#include "Utils.h"
#include <assert.h>

#define TRACE LOG("%s/%04d", __FILE__, __LINE__);

DWORD WINAPI DeviceThreadProc(LPVOID lpParameter)
{
	(reinterpret_cast<Device*>(lpParameter))->WorkerProc();
	return 0;
}

DWORD WINAPI ConnKeepaliveThreadProc(LPVOID lpParameter)
{
	(reinterpret_cast<Device*>(lpParameter))->ConnKeepaliveProc();
	return 0;
}

Device::Device(void):
	port(49280),
	socketHandle(INVALID_SOCKET),
	terminate(false),
	workerThread(INVALID_HANDLE_VALUE),
	keepaliveThread(INVALID_HANDLE_VALUE),
	connectRequest(false),
	connected(false),
	connectionLost(false),
	waitForReply(-1),
	onReceiveCb(NULL),
	onReceiveOpaque(NULL),
	workerTerminated(true),
	keepaliveTerminated(true),
	onPoll(NULL),
	onPollOpaque(NULL),
	keepAlivePeriod(-1),
	hideKeepAliveLog(false)
{
}

int Device::Configure(std::string sAddress, int port, std::string initString, int keepAlivePeriod, std::string keepAliveString, bool hideKeepAliveLog) {
	this->sAddress = sAddress;
	this->port = port;
	this->initString = initString;
	this->keepAlivePeriod = keepAlivePeriod;
	this->keepAliveString = keepAliveString;
	this->hideKeepAliveLog = hideKeepAliveLog;
	return 0;
}

int Device::Start(void) {
	ScopedLock<Mutex> lock(mutex);
	assert(onReceiveCb);
	connectRequest = true;
	terminate = false;

	waitForReply = -1;

	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		LOG("WSAStartup failed: %d", iResult);
		return 1;
	}

	keepaliveTerminated = false;
	HANDLE keepaliveThread = CreateThread(NULL, 0, ConnKeepaliveThreadProc, this, CREATE_SUSPENDED, NULL);
	SetThreadPriority(keepaliveThread, THREAD_PRIORITY_NORMAL);
	this->keepaliveThread = keepaliveThread;
	ResumeThread(keepaliveThread);

	workerTerminated = false;
	HANDLE workerThread = CreateThread(NULL, 0, DeviceThreadProc, this, CREATE_SUSPENDED, NULL);
	SetThreadPriority(workerThread, THREAD_PRIORITY_NORMAL);
	this->workerThread = workerThread;
	ResumeThread(workerThread);

	return 0;
}

int Device::Connect(void) {
	if (connected) {
		LOG("Device: Connect: already connected");
		return 0;
	}

	{
		ScopedLock<Mutex> lock(mutexQueue);
		cmdQueue.clear();
	}

    struct sockaddr_in sa;
    struct hostent* hp;

	LOG("Device: Connect to %s:%d", sAddress.c_str(), port);

    hp = gethostbyname(sAddress.c_str());
    if (hp == NULL)
    {
		int wsaLasterror = WSAGetLastError();
		LOG("Failed to resolve host address, WSA error %u", wsaLasterror);
		return -2;
    }

    memset(&sa, 0, sizeof(sa));
    memcpy((char *)&sa.sin_addr, hp->h_addr, hp->h_length);
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((u_short)port);

    socketHandle = socket(hp->h_addrtype, SOCK_STREAM, 0);
    if (socketHandle == INVALID_SOCKET)
    {
		LOG("Failed to create socket");
		return -3;
    }
/*
	int rcvTimeout = 50;
	if (setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO, (const char*)&rcvTimeout, sizeof(int)) == SOCKET_ERROR) {
		LOG(L"Failed to set socket receive timeout");
        closesocket(socketHandle);
        socketHandle = INVALID_SOCKET;
		return -3;
	}
*/
	int sndTimeout = 3000; // 3 s
	if (setsockopt(socketHandle, SOL_SOCKET, SO_SNDTIMEO, (const char*)&sndTimeout, sizeof(int)) == SOCKET_ERROR) {
		LOG("Failed to set socket send timeout");
        closesocket(socketHandle);
        socketHandle = INVALID_SOCKET;
		return -3;
	}

    if (connect(socketHandle, (struct sockaddr *)&sa, sizeof sa) == SOCKET_ERROR) 
    {
		int wsaLasterror = WSAGetLastError();
		LOG("Connection failed, WSA error %d", wsaLasterror);
        closesocket(socketHandle);
        socketHandle = INVALID_SOCKET;
        return -4;
    }
	LOG("Connected to device");

	int rc = SendCmd(initString, false);

	if (rc) {
		LOG("Failed to send preamble commands to device");
        closesocket(socketHandle);
        socketHandle = INVALID_SOCKET;
        return -5;
	}

	connected = true;

	return 0;
}

int Device::Disconnect(void) {
	ScopedLock<Mutex> lock(mutex);

	connectRequest = false;
	LOG("Device: Closing connection");
	ConnClose();

	terminate = true;

	LOG("Device: Waiting for worker thread termination");
	while (!workerTerminated) {
		Sleep(100);
	}

	LOG("Device: Waiting for ka thread termination");
	while (keepaliveTerminated == false) {
		Sleep(50);
	}
	LOG("Device: ka thread terminated");

    WSACleanup();

	return 0;
}

int Device::ConnClose(void) {
	if (socketHandle != INVALID_SOCKET ) {
		SOCKET s = socketHandle;
		socketHandle = INVALID_SOCKET;
		shutdown(s, SD_BOTH);	// doesn't break existing recv call itself
		closesocket(s);
	}
	connected = false;
	return 0;
}

void Device::WorkerProc(void) {
	char charBuffer[2] = {'\0', '\0'};
	std::string cmd;
	while (!terminate) {
		if (socketHandle == INVALID_SOCKET) {
			Sleep(100);
			continue;
		}
		int bytes = recv(socketHandle, charBuffer, 1, 0);
		if (bytes == SOCKET_ERROR) {
			int err = WSAGetLastError();
			LOG("recv() WSA error %d", err);
			connectionLost = true;
			ConnClose();
		} else if (bytes == 0) {
			//LOG(L"recv() 0 bytes, disconnected");
			//break;
		} else if (bytes > 0) {
			waitForReply = -1;
			cmd += charBuffer;
			if (charBuffer[0] == '\r' || charBuffer[0] == '\n') {
				if (cmd.length() > 1) {	// skip single '\n'
					ProcessCmd(cmd);
				}
				cmd = "";
			}
		}
	}
	LOG("WorkerProc Exit");
	workerTerminated = true;
}

void Device::ProcessCmd(std::string cmd) {
	//LOG("RX: %s", cmd.c_str());
	onReceiveCb(cmd, onReceiveOpaque);
}

int Device::SendCmd(std::string cmd, bool hideLog) {
	ScopedLock<Mutex> lock(mutex);
	if (socketHandle == INVALID_SOCKET) {
		return 1;
	}
	if (hideLog == false) {
		LOG("TX: %s", cmd.c_str());
	}
    int rc = send(socketHandle, cmd.c_str(), (int)cmd.length() , 0);
	if (rc == SOCKET_ERROR) {
		LOG("Device: socket send error");
		connectionLost = true;
		ConnClose();
		return 2;
	}
	return 0;
}

void Device::ConnKeepaliveProc(void) {
	LOG("ConnKeepaliveProc Start");
	unsigned int counter = 0;
	int kaCounter = 0;

	Connect();

	enum { HANDLE_PERIOD = 20 };
	while (!terminate) {
		if (connectRequest) {
			ScopedLock<Mutex> lock(mutex);
			if (connected) {
                counter++;
				if (waitForReply >= 0) {
					waitForReply++;
					if (waitForReply == 10000/HANDLE_PERIOD) {
						LOG("Device: no reply for keepalive msg");
						connectionLost = true;
						ConnClose();
					}
				}
				if (keepAlivePeriod > 0 && ((++kaCounter) >= (keepAlivePeriod/HANDLE_PERIOD)) ) {
					// "keepalive"
					kaCounter = 0;
					LOG("Keepalive, counter = %u", counter);
					//waitForReply = 0;
					waitForReply = -1;	// no reply for ka expected
					if (SendCmd(keepAliveString, this->hideKeepAliveLog)) {
						waitForReply = -1;
					}
				} else {
					// handle command queue
					ScopedLock<Mutex> lock(mutexQueue);
					if (cmdQueue.empty() == false) {
						std::string cmd = cmdQueue.front();
						cmdQueue.pop_front();
						SendCmd(cmd, false);
						kaCounter = 0;
					}
				}
			} else {
				// reconnect
				if (((++counter) % (20000/HANDLE_PERIOD)) == 0) {
					LOG("Device: reconnect");
					Connect();
				}
			}
		}
		if (onPoll) {
			onPoll(onPollOpaque);
		}
		Sleep(HANDLE_PERIOD);
	}
	LOG("ConnKeepaliveProc Exit");
	keepaliveTerminated = true;
}

int Device::EnqueueCmd(std::string cmd) {
	ScopedLock<Mutex> lock(mutexQueue);
	cmdQueue.push_back(cmd);
	return 0;
}

