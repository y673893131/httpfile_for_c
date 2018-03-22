#pragma once

#if defined(EXPORT_API)  
#define	DLL_API	__declspec(dllexport)
#else  
#define	DLL_API	__declspec(dllimport)  
#endif

#include <WinInet.h>
#include <afxinet.h>
#include <string>
#include <MMSystem.h>

class DLL_API CHttpFileModule
{
public:
	/* instance */
	static CHttpFileModule* Instance();

	/* start work */
	bool Start(char* pRemoteUrl,const char *pLocalPath);

	/* upload file */
	bool HttpUpLoading(const char *phostName,const char *postData,int iLen,char *pResult,int *iResultLen);

	/* stop upload or download file */
	void Stop();

	/* is stop */
	BOOL IsStop();

	/* define func */
	enum ENUM_TRANSPORT_TYPE
	{
		TRANSPORT_DOWNLOAD=0,
		TRANSPORT_UPLOAD
	};
private:
#define ZERO 0
#define DEFAULT_RECVPACK_SIZE 8096
#define DEFAULT_REMOTE_PORT 80
#define TIMER_DOWNLOAD 1
#define NORMAL_CONNECT INTERNET_FLAG_KEEP_CONNECTION  
#define SECURE_CONNECT NORMAL_CONNECT | INTERNET_FLAG_SECURE  
#define NORMAL_REQUEST INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE   
#define SECURE_REQUEST NORMAL_REQUEST | INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID  

#define NEW_ARRAY_PTR(x,l) \
	DELETE_ARRAY_PTR(x);*x = new char[l + 8];memset(*x,0x00,l+8);
#define DELETE_PTR(x) \
	if (*x)	{delete (*x);*x = NULL;};
#define DELETE_ARRAY_PTR(x) \
	if (*x)	{delete[] (*x);*x = NULL;};

#define RELEASE_HANDEL(x) \
	if ((*x) != INVALID_HANDLE_VALUE){ ::CloseHandle(*x); *x = INVALID_HANDLE_VALUE; };

	enum ENUM_PERCENT
	{
		PERCENT_BEGIN=0,
		PERCENT_END=100,
		PERCENT_EXCEPTION_SUB_BREAK,
		PERCENT_EXCEPTION_ERROR_CODE,
		PERCENT_EXCEPTION_OTHER
	};

/* set param */
public:
	/* set trans port */
	void SetTransportType(ENUM_TRANSPORT_TYPE type);

	/* set block status */
	void SetBlock(BOOL bBlock=FALSE);

	/* set transport rate */
	void SetTransportRate(DWORD nBytePerSecond=DEFAULT_RECVPACK_SIZE);

	/* set transport port */
	void SetTransportPort(INTERNET_PORT nPort=DEFAULT_REMOTE_PORT);

	/* set breakpoint transport */
	void SetBreakpointTransport(BOOL bBreakpoint=FALSE);

/* get param */
public:
	/* get block status */
	BOOL GetBlock();

	/* get transport rate */
	DWORD GetTransportRate();

	/* get transport port */
	INTERNET_PORT GetTransportPort();

	/* get local file */
	CString GetLocalFile();

	/* get remote file */
	CString GetRemoteFile();

	/* get percent */
	FLOAT GetPercent();

	/* get percent handle */
	HANDLE GetHandlePercent();

	/* get last error code */
	UINT GetLastErrorCode();

private:
	static DWORD WINAPI	DownloadThreadFunc(LPVOID lpParam);
	static DWORD WINAPI	UploadThreadFunc(LPVOID lpParam);
	static MMRESULT CALLBACK DownloadTimerProc(INT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

	void SetMmTimer(MMRESULT mmtm);
	void SetPercent(FLOAT fPercent);
	void SetErrorCode(UINT code);

	BOOL GetBreakpoint();
	CString GetBoundary();
	ENUM_TRANSPORT_TYPE GetTransportType();
	char* GetTransportBuf(UINT& ibufflen);
	CInternetSession* GetSession(const char* pStrArgs=NULL);
	CHttpFile* GetHttpFile(const char* pStrArgs=NULL);
	CFile* GetFile();

	void ReleaseSession();
	void ReleaseHttpFile();
	void ReleaseFile();

	CString FormatDownloadHeader();
	CString FormatUploadHeader(CString& szBoundary);
	CString FormatUploadPerFileData(CString& szBoundary);
	CString FormatUploadPostData(CString& szBoundary);
	void StopDoing();
	
public:
	virtual ~CHttpFileModule();

private:
	CHttpFileModule();

	void Init();
	void UnInit();
	void InitTimer();
	void UnInitTimer();
	void CheckSecure();

private:
	char m_szRemoteFile[MAX_PATH];
	char m_szLocalFile[MAX_PATH];
	char m_szUploadFileName[MAX_PATH];

private:
	BOOL m_bBlock;								//block status: true(block) false(nonblock)
	DWORD m_nTransRate;							//transport file speed(Byte)
	INTERNET_PORT m_nPort;						//transport Port
	FLOAT m_fPercent;							//transport percent
	HANDLE m_hSemaphorePercent;					//percent timer semp
	HANDLE m_hThreadDownload;					//download file thread
	MMRESULT m_mmTimerTransport;				//timer
	BOOL m_bStop;								//transport status
	BOOL m_bBreakpoint;							//break point transport
	ENUM_TRANSPORT_TYPE m_nTransPortType;		//transport type
	LPTHREAD_START_ROUTINE m_threadFun;			//thread function
	UINT m_nBufTransportLength;					//transport buffer length
	char* m_pBufTransport;						//transport buffer
	CInternetSession* m_pSess;					//session
	CHttpConnection* m_pConn;					//http connection
	CHttpFile* m_pHF;							//http file
	CFile* m_pF;								//local file
	UINT m_nErrorCode;							//error code
	BOOL m_bsecureFlag;							//http:FALSE,https:TRUE
	static CHttpFileModule* s_instance;			//signal model
};
