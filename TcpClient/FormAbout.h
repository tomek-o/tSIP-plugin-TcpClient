/** \brief "About program" window header file
*/

//----------------------------------------------------------------------------
#ifndef FormAboutH
#define FormAboutH
//----------------------------------------------------------------------------
#include <vcl\System.hpp>
#include <vcl\Windows.hpp>
#include <vcl\SysUtils.hpp>
#include <vcl\Classes.hpp>
#include <vcl\Graphics.hpp>
#include <vcl\Forms.hpp>
#include <vcl\Controls.hpp>
#include <vcl\StdCtrls.hpp>
#include <vcl\Buttons.hpp>
#include <vcl\ExtCtrls.hpp>
//----------------------------------------------------------------------------
class TfrmAbout : public TForm
{
__published:
	TPanel *Panel1;
	TImage *ProgramIcon;
	TLabel *ProductName;
	TButton *OKButton;
	TLabel *lblVersion;
	TLabel *lblBuilt;
	TLabel *lblBuildTimestamp;
	TMemo *Memo;
private:
public:
	virtual __fastcall TfrmAbout(TComponent* AOwner);
};
//----------------------------------------------------------------------------
extern PACKAGE TfrmAbout *frmAbout;
//----------------------------------------------------------------------------
#endif
