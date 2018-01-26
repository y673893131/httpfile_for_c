#pragma once
#include "afxcmn.h"
#include "afxwin.h"


class CsimpleDlg : public CDialogEx
{
public:
	CsimpleDlg(CWnd* pParent = NULL);

	enum { IDD = IDD_SIMPLE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	HICON m_hIcon;

	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	int m_transportRate;
	CString m_szRemote;
	afx_msg void OnBnClickedButton2();
	CString m_szLocal;
	BOOL m_SyncCheck;
	CString m_transportType;
	CString m_szProgessNote;
	CProgressCtrl m_progressctrl;
	CString m_szProgress;
	CStatic m_staProgress;
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	BOOL m_breakpointtrans;
};
