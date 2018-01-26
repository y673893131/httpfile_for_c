#include "stdafx.h"
#include "simple.h"
#include "simpleDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
	#pragma comment(lib, "../Debug/HttpFileModule.lib")
#else
	#pragma comment(lib, "../Release/HttpFileModule.lib")
#endif

#include "../HttpFileModule/HttpFileHead.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


CsimpleDlg::CsimpleDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CsimpleDlg::IDD, pParent)
	, m_transportRate(80)
	, m_szRemote("http://192.168.31.134:8000/Desktop/")
	, m_szLocal("K:\\¹¤¾ß\\iso\\zh-hans_windows_xp_professional_with_service_pack_3_x86_cd_vl_x14-74070 (ED2000.COM).iso")
	, m_SyncCheck(FALSE)
	, m_transportType("upload")
	, m_szProgress(_T(""))
	, m_breakpointtrans(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CsimpleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_transportRate);
	DDX_Text(pDX, IDC_EDIT2, m_szRemote);
	DDX_Text(pDX, IDC_EDIT3, m_szLocal);
	DDX_Check(pDX, IDC_CHECK1, m_SyncCheck);
	DDX_CBString(pDX, IDC_COMBO1, m_transportType);
	DDX_Control(pDX, IDC_PROGRESS1, m_progressctrl);
	DDX_Text(pDX, IDC_STATIC_PROGRESS, m_szProgress);
	DDX_Control(pDX, IDC_STATIC_PROGRESS, m_staProgress);
	DDX_Check(pDX, IDC_CHECK2, m_breakpointtrans);
}

BEGIN_MESSAGE_MAP(CsimpleDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CsimpleDlg::OnBnClickedOk)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON2, &CsimpleDlg::OnBnClickedButton2)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL CsimpleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	m_progressctrl.SetRange(0,100);
	m_progressctrl.SetPos(0);
	m_progressctrl.SetBarColor(RGB(0,255,64));
	//m_progressctrl.SetBkColor(RGB(200,255,255));
	m_staProgress.SetParent(m_progressctrl.GetSafeOwner());

	return TRUE;
}

void CsimpleDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void CsimpleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); 

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CsimpleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CsimpleDlg::OnBnClickedOk()
{
	UpdateData(TRUE);

	if (m_transportType == "upload")
	{
		CHttpFileModule::Instance()->SetTransportType(CHttpFileModule::TRANSPORT_UPLOAD);
	}
	else
	{
		CHttpFileModule::Instance()->SetTransportType(CHttpFileModule::TRANSPORT_DOWNLOAD);
	}

	CHttpFileModule::Instance()->SetBreakpointTransport(m_breakpointtrans);
	CHttpFileModule::Instance()->SetTransportRate(m_transportRate * 1024);
	CHttpFileModule::Instance()->SetBlock(m_SyncCheck);
	CHttpFileModule::Instance()->Start(m_szRemote.GetBuffer(), m_szLocal.GetBuffer());
	SetTimer(1,100,NULL);
	//CDialogEx::OnOK();
}


void CsimpleDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1)
	{
		float progress = CHttpFileModule::Instance()->GetPercent();
		m_szProgress.Format(_T("%.3f%%"),progress);
		if ((int)progress >= 100)
		{
			KillTimer(nIDEvent);
			if ((int)progress == 100)
			{
				m_progressctrl.SetPos((int)(progress+0.5));
			}
			else
			{
				m_szProgessNote.Format("Progress:%d,Error:%d",progress,GetLastError());
				AfxMessageBox(m_szProgessNote);
			}
		}
		else
		{
			m_progressctrl.SetPos((int)(progress+0.5));
		}

		UpdateData(FALSE);
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CsimpleDlg::OnBnClickedButton2()
{
	KillTimer(1);
	CHttpFileModule::Instance()->Stop();
}


HBRUSH CsimpleDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	if (pWnd->GetDlgCtrlID() == IDC_STATIC_PROGRESS)
	{
		pDC->SetBkMode(TRANSPARENT);
	}

	return hbr;
}
