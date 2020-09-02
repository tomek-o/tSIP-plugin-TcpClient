/** \file
*/
//---------------------------------------------------------------------------

#ifndef SettingsH
#define SettingsH
//---------------------------------------------------------------------------
#include <System.hpp>

class Settings
{
public:
	int Read(AnsiString asFileName);
	int Write(AnsiString asFileName);
	void SetDefault(void);
	struct _gui
	{
		enum { SCALING_MIN = 50 };
		enum { SCALING_MAX = 200 };
		int scalingPct;					///< scaling (percentage)
	} gui;
	struct _frmMain
	{
		int iPosX, iPosY;				///< main window coordinates
		int iHeight, iWidth;			///< main window size
		bool bWindowMaximized;			///< is main window maximize?
		bool bAlwaysOnTop;
	} frmMain;
	struct _device
	{
		enum { DEFAULT_PORT = 7778 };
		AnsiString asAddress;
		int port;
		AnsiString initCmd;
		bool bHideKeepAlive;
		int keepalivePeriod;
		AnsiString keepaliveCmd;
		_device(void):
			asAddress("192.168.0.12"),
			port(DEFAULT_PORT),
			bHideKeepAlive(false),
			keepalivePeriod(60000)
		{}
	} Device;
	struct _Logging
	{
		bool bLogToFile;
		bool bFlush;
		enum {
			MIN_MAX_FILE_SIZE = 0,
			MAX_MAX_FILE_SIZE = 1000*1024*1024
		};
		enum {
            DEF_MAX_FILE_SIZE = 10*1024*1024
        };
		int iMaxFileSize;
		unsigned int iMaxUiLogLines;
	} Logging;
};

extern Settings appSettings;

#endif
