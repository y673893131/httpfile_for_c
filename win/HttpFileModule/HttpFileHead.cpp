#include "StdAfx.h"
#include "HttpFileHead.h"
#include "ErrorDefine.h"
#include <io.h>
#include <assert.h>

CHttpFileModule* CHttpFileModule::s_instance = NULL;

CHttpFileModule::CHttpFileModule()
	:m_bBlock(FALSE)
	,m_nTransRate(DEFAULT_RECVPACK_SIZE)
	,m_nPort(DEFAULT_REMOTE_PORT)
	,m_hSemaphorePercent(INVALID_HANDLE_VALUE)
	,m_hThreadDownload(INVALID_HANDLE_VALUE)
	,m_fPercent(PERCENT_BEGIN)
	,m_nTransPortType(TRANSPORT_DOWNLOAD)
	,m_bBreakpoint(FALSE)
	,m_bStop(TRUE)
	,m_mmTimerTransport(NULL)
	,m_pBufTransport(NULL)
	,m_nBufTransportLength(ZERO)
	,m_pSess(NULL)
	,m_pConn(NULL)
	,m_pHF(NULL)
	,m_pF(NULL)
	,m_nErrorCode(ERROR_SUCCESS_DEFAULT)
	,m_bsecureFlag(FALSE)
{
}

CHttpFileModule::~CHttpFileModule()
{
	DELETE_PTR(&s_instance);
	UnInit();
	RELEASE_HANDEL(&m_hThreadDownload);
}

CHttpFileModule* CHttpFileModule::Instance()
{
	if (s_instance == NULL)
	{
		s_instance = new CHttpFileModule();
	}

	return s_instance;
}

void CHttpFileModule::Init()
{
	m_bStop = FALSE;
	SetErrorCode(ERROR_SUCCESS_DEFAULT);
	CheckSecure();
	m_hSemaphorePercent = ::CreateSemaphore(NULL, 0, 1024, NULL);
	m_threadFun = m_nTransPortType == TRANSPORT_DOWNLOAD ? reinterpret_cast<LPTHREAD_START_ROUTINE>(DownloadThreadFunc)
		: reinterpret_cast<LPTHREAD_START_ROUTINE>(UploadThreadFunc);
}

void CHttpFileModule::InitTimer()
{
	m_nBufTransportLength = DEFAULT_RECVPACK_SIZE;
	if (GetTransportRate() <= 0)
	{
		SetTransportRate(DEFAULT_RECVPACK_SIZE);
	}
	else
	{
		if (GetTransportRate() / DEFAULT_RECVPACK_SIZE >= 1024)
			m_nBufTransportLength = DEFAULT_RECVPACK_SIZE * 8;
	}

	int nDeltaTime = (int)((1000 / (GetTransportRate() * 1.0 / m_nBufTransportLength)) + 0.5);
	if (nDeltaTime == 0)
		nDeltaTime = 1;
	
	NEW_ARRAY_PTR(&m_pBufTransport,m_nBufTransportLength);
	
	SetMmTimer(timeSetEvent(nDeltaTime, 1, reinterpret_cast<LPTIMECALLBACK>(DownloadTimerProc),
		reinterpret_cast<DWORD_PTR>(GetHandlePercent()), TIME_PERIODIC));
}

void CHttpFileModule::UnInitTimer()
{
	timeKillEvent(m_mmTimerTransport);
	DELETE_ARRAY_PTR(&m_pBufTransport);
}

void CHttpFileModule::UnInit()
{
	m_bStop = TRUE;
	RELEASE_HANDEL(&m_hSemaphorePercent);
	UnInitTimer();
	ReleaseHttpFile();
	ReleaseSession();
	ReleaseFile();
}

void CHttpFileModule::CheckSecure()
{
	CString szRemote(m_szRemoteFile);
	if (!szRemote.Find("https://"))
		m_bsecureFlag = TRUE;
	else
		m_bsecureFlag = FALSE;
}

bool CHttpFileModule::Start(char* pRemoteUrl,const char *pLocalPath)
{
	if (!pRemoteUrl || !pLocalPath)	{
		SetErrorCode(ERROR_INPUT_PARAM);
		return false;
	}

	if (!IsStop()){
		SetErrorCode(ERROR_LAST_TASK_IS_WORKING);
		return false;
	}

	memset(m_szLocalFile,0x00,sizeof(m_szLocalFile));
	strncpy_s(m_szLocalFile,pLocalPath,sizeof(m_szLocalFile) - 1);

	memset(m_szRemoteFile,0x00,sizeof(m_szRemoteFile));
	strncpy_s(m_szRemoteFile,pRemoteUrl,sizeof(m_szRemoteFile) - 1);

	Init();

	if (!GetBlock())
	{
		m_hThreadDownload = ::CreateThread(NULL, 0, m_threadFun, this, 0, NULL);
		if (m_hThreadDownload == INVALID_HANDLE_VALUE)
		{
			UnInit();
			SetErrorCode(ERROR_CREATE_TASK_THREAD_FAILED);
			return false;
		}

		return true;
	}

	return m_threadFun(this) == 0 ? true : false;
}

DWORD CHttpFileModule::DownloadThreadFunc(LPVOID lpParam)
{
	CHttpFileModule* pHttpModule = static_cast<CHttpFileModule*>(lpParam);

	DWORD ret = -1;

	do{
		try{
			CHttpFile* pHF = pHttpModule->GetHttpFile("lpload");
			DWORD dwStatusCode = 0;
			pHF->QueryInfoStatusCode(dwStatusCode);
			
			if(dwStatusCode == HTTP_STATUS_OK || dwStatusCode == HTTP_STATUS_PARTIAL_CONTENT)
			{
				DWORD nBufLen=0,nLen=0,nBaseLen=0;
				pHF->QueryInfo(HTTP_QUERY_CONTENT_LENGTH, nLen);
				nBufLen=nLen;
				if(nLen<=0){
					pHttpModule->SetErrorCode(ERROR_QUARY_CONTENT_LENGTH);
					break;
				}

				if (dwStatusCode == HTTP_STATUS_PARTIAL_CONTENT)
					pHttpModule->SetBreakpointTransport(FALSE);

				CFile* pF = pHttpModule->GetFile();
				if (dwStatusCode == HTTP_STATUS_PARTIAL_CONTENT)
				{
					pF->SeekToEnd();
					nBaseLen = (DWORD)pF->GetLength();
					nBufLen += nBaseLen;
					pHttpModule->SetPercent((FLOAT)((nBaseLen) / (1.0*nBufLen) * 100));
				}

				pHttpModule->InitTimer();
				UINT len = 0;
				char* pBuf = pHttpModule->GetTransportBuf(len);

				for(;;)
				{
					if (pHttpModule->IsStop())
						break;

					if (WAIT_OBJECT_0 != ::WaitForSingleObject(pHttpModule->GetHandlePercent(), INFINITE))
						break;

					UINT n = pHF->Read(pBuf,(nLen<len)?nLen:len);
					if (n == 0)
						break; 

					pF->Write(pBuf,n);
					nLen -= n ;
					nBaseLen += n;
					pHttpModule->SetPercent((FLOAT)((nBaseLen) / (1.0*nBufLen) * 100));
				}

				if (nLen != 0) 
				{
					if (!pHttpModule->IsStop())
						pHttpModule->SetPercent(PERCENT_EXCEPTION_SUB_BREAK);
				}
				else
				{
					pHttpModule->SetPercent(PERCENT_END);
				}

				ret = 0;
			}
			else
			{
				SetLastError(dwStatusCode);
				pHttpModule->SetErrorCode(ERROR_HTTP_EXCEPTION);
				pHttpModule->SetPercent(PERCENT_EXCEPTION_ERROR_CODE);
			}
		}
		catch(...)
		{
			pHttpModule->SetErrorCode(ERROR_HTTP_EXCEPTION);
			pHttpModule->SetPercent(PERCENT_EXCEPTION_OTHER);
			break;
		}
	}while(0);

	pHttpModule->UnInit();

	return ret;
}

DWORD CHttpFileModule::UploadThreadFunc(LPVOID lpParam)
{
	CHttpFileModule* pHttpModule = static_cast<CHttpFileModule*>(lpParam);

	DWORD ret=-1;

	do{
		try{
			CFile* pF = pHttpModule->GetFile();
			if (!pF)
				break;

			DWORD nLen = (DWORD)pF->GetLength();
			DWORD nBufLen = nLen;
			if (nLen == 0) break;

			CHttpFile* pHF = pHttpModule->GetHttpFile("upload");
			if (!pHF)
				break;

			CString szBoundary(""),szPerFileHeaders(""),szPerFileData(""),szPostFileData("");

			szBoundary = pHttpModule->GetBoundary();
			szPerFileHeaders = pHttpModule->FormatUploadHeader(szBoundary);
			szPerFileData = pHttpModule->FormatUploadPerFileData(szBoundary);
			szPostFileData = pHttpModule->FormatUploadPostData(szBoundary);

			pHF->AddRequestHeaders(szPerFileHeaders);
			pHF->SendRequestEx(nLen + szPerFileData.GetLength() + szPostFileData.GetLength(), HSR_SYNC | HSR_INITIATE);
			pHF->Write(szPerFileData, szPerFileData.GetLength());

			pHttpModule->InitTimer();
			UINT len = 0,n = 0;
			char* pBuf = pHttpModule->GetTransportBuf(len);

			for(;;)
			{
				if (pHttpModule->IsStop())
					break;

				if (WAIT_OBJECT_0 != ::WaitForSingleObject(pHttpModule->GetHandlePercent(), INFINITE))
					break;

				n = pF->Read(pBuf,(nLen<len)?nLen:len);
				if (n <= 0)
				{
					pHF->Write(szPostFileData,szPostFileData.GetLength());
					pHF->EndRequest(HSR_SYNC);
					DWORD dwStatusCode=0;
					pHF->QueryInfoStatusCode(dwStatusCode);

					DWORD dwResponseLength = (DWORD)pHF->GetLength();
					CString szResp("");
					while (0 != dwResponseLength)
					{
						char* szResponse = (LPSTR)malloc(dwResponseLength + 1);
						szResponse[dwResponseLength] = '\0';
						pHF->Read(szResponse, dwResponseLength);
						szResp += szResponse;
						free(szResponse);
						szResponse = NULL;
						dwResponseLength = (DWORD)pHF->GetLength();
					}

					break;
				}

				pHF->Write(pBuf, n);

				nLen -= n ;

				pHttpModule->SetPercent((FLOAT)((nBufLen - nLen) / (1.0*nBufLen) * 100));
			}

			//recv sub break
			if (nLen != 0) 
				pHttpModule->SetPercent(PERCENT_EXCEPTION_SUB_BREAK);
			else
				pHttpModule->SetPercent(PERCENT_END);

			ret = 0;
		}
		catch(...)
		{
			pHttpModule->SetErrorCode(ERROR_HTTP_EXCEPTION);
			pHttpModule->SetPercent(PERCENT_EXCEPTION_OTHER);
			break;
		}
	}while(0);

	pHttpModule->UnInit();

	return ret;
}

CString CHttpFileModule::FormatDownloadHeader()
{
	unsigned int pos = 0;
	if (GetBreakpoint())
	{
		if (0 == _access(GetLocalFile(), 0))
		{
			FILE *fp;
			fopen_s(&fp,GetLocalFile(),"r");
			if(fp!=NULL)
			{
				fseek(fp,0,SEEK_END);
				pos = ftell(fp);
				fclose(fp);
			}
			else
			{
				SetErrorCode(ERROR_LOCAL_FILE_CAN_NOT_ACCESS);
			}
		}		
	}

	CString szRequestHeader("");
	szRequestHeader.Format(_T("Accept: */*\r\n")
		_T("User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3\r\n")
		_T("Range: bytes=%u-\r\n"), pos);

	return szRequestHeader;
}

CString CHttpFileModule::FormatUploadHeader(CString& szBoundary)
{
	CString strFormat;
	CString strData;
	strFormat = _T("Content-Type: multipart/form-data; boundary=%s\r\n");
	strData.Format(strFormat, szBoundary);
	return strData;
}

CString CHttpFileModule::FormatUploadPerFileData(CString& szBoundary)
{
	CString strFormat;
	CString strData;
	strFormat += _T("--%s");
	strFormat += _T("\r\n");
	strFormat += _T("Content-Disposition: form-data; name=\"recordid\"");//param
	strFormat += _T("\r\n\r\n");
	strFormat += _T("1");
	strFormat += _T("\r\n");
	strFormat += _T("--%s");
	strFormat += _T("\r\n");
	strFormat += _T("Content-Disposition: form-data; name=\"trackdata\"; filename=\"%s\"");//file
	strFormat += _T("\r\n");
	strFormat += _T("Content-Type: application/x-www-form-urlencoded");//type
	strFormat += _T("\r\n");
	strFormat += _T("Content-Transfer-Encoding: binary");
	strFormat += _T("\r\n\r\n");
	strData.Format(strFormat, szBoundary, szBoundary, m_szUploadFileName);
	return strData;
}

CString CHttpFileModule::FormatUploadPostData(CString& szBoundary)
{
	CString strFormat;
	CString strData;
	strFormat = _T("\r\n");
	strFormat += _T("--%s");
	strFormat += _T("\r\n");
	strFormat += _T("Content-Disposition: form-data; name=\"submitted\"");
	strFormat += _T("\r\n\r\n");
	strFormat += _T("hello");
	strFormat += _T("\r\n");
	strFormat += _T("--%s--");
	strFormat += _T("\r\n");
	strData.Format(strFormat, szBoundary, szBoundary);
	return strData;
}

MMRESULT CHttpFileModule::DownloadTimerProc(INT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	::ReleaseSemaphore(reinterpret_cast<HANDLE>(dwUser), 1, NULL);
	return 1;
}

void CHttpFileModule::Stop()
{
	StopDoing();
	if (m_hThreadDownload != INVALID_HANDLE_VALUE)
	{
		::WaitForSingleObject(m_hThreadDownload,INFINITE);
	}
}

void CHttpFileModule::StopDoing()
{
	m_bStop = TRUE;
}

BOOL CHttpFileModule::IsStop()
{
	return m_bStop;
}

void CHttpFileModule::SetTransportType(ENUM_TRANSPORT_TYPE type)
{
	m_nTransPortType = type;
}

void CHttpFileModule::SetBlock(BOOL bBlock)
{
	m_bBlock = bBlock;
}

void CHttpFileModule::SetTransportRate(DWORD nBytePerSecond)
{
	m_nTransRate = nBytePerSecond;
}

void CHttpFileModule::SetTransportPort(INTERNET_PORT nPort)
{
	m_nPort = nPort;
}

void CHttpFileModule::SetPercent(FLOAT fPercent)
{
	m_fPercent = fPercent;
}

void CHttpFileModule::SetErrorCode(UINT code)
{
	m_nErrorCode = code;
}

void CHttpFileModule::SetMmTimer(MMRESULT mmtm)
{
	m_mmTimerTransport = mmtm;
}

void CHttpFileModule::SetBreakpointTransport(BOOL bBreakpoint)
{
	m_bBreakpoint = bBreakpoint;
}

BOOL CHttpFileModule::GetBlock()
{
	return m_bBlock;
}

DWORD CHttpFileModule::GetTransportRate()
{
	return m_nTransRate;
}

INTERNET_PORT CHttpFileModule::GetTransportPort()
{
	return m_nPort;
}

CString CHttpFileModule::GetLocalFile()
{
	return m_szLocalFile;
}

CString CHttpFileModule::GetRemoteFile()
{
	return m_szRemoteFile;
}

FLOAT CHttpFileModule::GetPercent()
{
	return m_fPercent;
}

HANDLE CHttpFileModule::GetHandlePercent()
{
	return m_hSemaphorePercent;
}

BOOL CHttpFileModule::GetBreakpoint()
{
	return m_bBreakpoint;
}

CString CHttpFileModule::GetBoundary()
{
	CString szBoundary("");
	SYSTEMTIME tm;
	::GetLocalTime(&tm);
	szBoundary.Format(_T("--%04d%02d%02d%02d%02d%02d%03d--"),tm.wYear,
		tm.wMonth,tm.wDay,tm.wHour,tm.wMinute,tm.wSecond,tm.wMilliseconds);
	return szBoundary;
}

CHttpFileModule::ENUM_TRANSPORT_TYPE CHttpFileModule::GetTransportType()
{
	return m_nTransPortType;
}

char* CHttpFileModule::GetTransportBuf(UINT& nbufflen)
{
	nbufflen = m_nBufTransportLength;
	return m_pBufTransport;
}

CInternetSession* CHttpFileModule::GetSession(const char* pStrArgs)
{
	if (!m_pSess)
	{
		m_pSess = new CInternetSession(pStrArgs);
		assert(m_pSess != NULL);
	}


	m_pSess->SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 2000);          //2秒的连接超时   
	m_pSess->SetOption(INTERNET_OPTION_SEND_TIMEOUT, 2000);             //2秒的发送超时   
	m_pSess->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 2000);          //2秒的接收超时   
	m_pSess->SetOption(INTERNET_OPTION_DATA_SEND_TIMEOUT, 2000);        //2秒的发送超时   
	m_pSess->SetOption(INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, 2000);     //2秒的接收超时
	
	return m_pSess;
}

CHttpFile* CHttpFileModule::GetHttpFile(const char* pStrArgs)
{
	if (!m_pHF)
	{
		DWORD dwFlag = INTERNET_FLAG_TRANSFER_BINARY|INTERNET_FLAG_DONT_CACHE|INTERNET_FLAG_PRAGMA_NOCACHE;
		if (m_bsecureFlag) dwFlag |= INTERNET_FLAG_SECURE;
		if (TRANSPORT_DOWNLOAD == GetTransportType())
		{
			m_pHF = (CHttpFile*)GetSession(pStrArgs)->OpenURL(GetRemoteFile(),1,dwFlag,FormatDownloadHeader(),-1);
		}
		else
		{
			DWORD dwServiceType=0;
			INTERNET_PORT nPort=0;
			CString szServer(""),szObject("");
			if (!AfxParseURL(GetRemoteFile(), dwServiceType, szServer, szObject, nPort))
			{
				UnInit();
				SetErrorCode(ERROR_ANALYSIS_URL);
				return NULL;
			}

			m_pConn = GetSession(pStrArgs)->GetHttpConnection(szServer,dwFlag,nPort);
			if (m_pConn == NULL)
			{
				UnInit();
				SetErrorCode(ERROR_GET_HTTP_CONNECTION);
				return NULL;
			}

			SetTransportPort(nPort);

			int nPos = GetLocalFile().ReverseFind('\\');
			if (nPos < 0)
				nPos = GetLocalFile().ReverseFind('/');

			strncpy_s(m_szUploadFileName,GetLocalFile().Mid(nPos+1).GetBuffer(),sizeof(m_szUploadFileName) - 1);

			m_pHF = m_pConn->OpenRequest(CHttpConnection::HTTP_VERB_POST,szObject);
		}
	}

	return m_pHF;
}

CFile* CHttpFileModule::GetFile()
{
	if (!m_pF)
	{
		UINT mode = CFile::modeRead;
		if (TRANSPORT_DOWNLOAD == GetTransportType())
		{
			mode = CFile::modeCreate | CFile::modeWrite;
			if (GetBreakpoint())
				mode |= CFile::modeNoTruncate;
		}

		m_pF = new CFile(GetLocalFile(), mode);
		if (m_pF)
			SetErrorCode(ERROR_LOCAL_FILE_CAN_NOT_ACCESS);

		assert(m_pF != NULL);
	}

	return m_pF;
}

void CHttpFileModule::ReleaseSession()
{
	if (m_pConn)
	{
		m_pConn->Close();
		DELETE_PTR(&m_pConn);
	}

	if (m_pSess)
	{
		m_pSess->Close();
		DELETE_PTR(&m_pSess);
	}
}

void CHttpFileModule::ReleaseHttpFile()
{
	if (m_pHF)
	{
		m_pHF->Close();
		DELETE_PTR(&m_pHF);
	}
}

void CHttpFileModule::ReleaseFile()
{
	if (m_pF)
	{
		m_pF->Close();
		DELETE_PTR(&m_pF);
	}
}