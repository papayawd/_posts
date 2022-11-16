
// DriverLoadDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "DriverLoad.h"
#include "DriverLoadDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDriverLoadDlg 对话框



CDriverLoadDlg::CDriverLoadDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DRIVERLOAD_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDriverLoadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_Edit);
}

BEGIN_MESSAGE_MAP(CDriverLoadDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CDriverLoadDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CDriverLoadDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CDriverLoadDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CDriverLoadDlg::OnBnClickedButton4)
END_MESSAGE_MAP()


// CDriverLoadDlg 消息处理程序

BOOL CDriverLoadDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CDriverLoadDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CDriverLoadDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

CString CDriverLoadDlg::getFilePath() { // 拖拽后获取文件路径
	CString path;
	GetDlgItemText(IDC_EDIT1, path);
	return path;
}
CString CDriverLoadDlg::getFileName() { // 通过文件路径获取文件名
	CString path;
	GetDlgItemText(IDC_EDIT1, path);
	CString filename = path.Right(path.GetLength() - path.ReverseFind('\\') - 1);
	return filename;
}

void CDriverLoadDlg::OnBnClickedButton1() // 驱动注册
{
	// TODO: 在此添加控件通知处理程序代码
	CString path = this->getFilePath();
	if (path.IsEmpty()) {
		MessageBox(__T("no file select\n"), __T("Button1"));
		return;
	}

	// scManager 管理器 用于管理一个服务---包括服务的创建 启动 停止 卸载
	this->scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (this->scManager == NULL) {
		MessageBox( __T("scManager open error\n"), __T("Button1"));
		return;
	}
	CString s;
	s.Format(L"this->scManager=%d\n", (int)this->scManager);
	MessageBox(s, __T("Button1"));

	CString fileName = this->getFileName();

	// 创建服务 服务运行的内容就是驱动  通过path告诉驱动路径
	// SERVICE_DEMAND_START表示手动开启 此外还有 SERVICE_BOOT_START SERVICE_SYSTEM_START
	SC_HANDLE serviceHandle = CreateService(this->scManager, fileName, fileName, SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER,SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, path, NULL, NULL, NULL, NULL, NULL);

	if (serviceHandle = NULL) {
		DWORD error = GetLastError();
		if (error == ERROR_SERVICE_EXISTS) {
			MessageBox( __T("service already exist\n"), __T("Button1"));
		}
		else {
			CString str;
			str.Format(L"error code=%d\n", error);
			MessageBox(str, __T("Button1"));
			OutputDebugString(str);
		}
		return;
	}

	// 服务一般用完就关掉，再一次获取服务通过scManager使用OpenService
	CloseServiceHandle(serviceHandle);

}



void CDriverLoadDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	CString path = this->getFilePath();
	if (path.IsEmpty()) {
		MessageBox(__T("no file select\n"), __T("Button2"));
		return;
	}
	if (this->scManager == NULL) {
		MessageBox(__T("pls restart service scManager open error\n"), __T("Button2"));
		return;
	}
	

	CString fileName = this->getFileName();

	// 通过scManager使用OpenService来获取已经创建的服务
	SC_HANDLE serviceHandle = OpenService(this->scManager, fileName, SERVICE_ALL_ACCESS);
	if (serviceHandle == NULL) {
		DWORD error = GetLastError();
		if (error == ERROR_SERVICE_DOES_NOT_EXIST) {
			MessageBox(__T("service does not exist\n"), __T("Button2"));
		}
		else {
			CString str;
			str.Format(L"error code=%d\n", error);
			MessageBox(str, __T("Button2"));
			OutputDebugString(str);
		}
		return;
	}
	
	//  开启服务
	int result = StartService(serviceHandle, 0, NULL);
	if (result == 0) {
		DWORD error = GetLastError();
		if (error == ERROR_SERVICE_ALREADY_RUNNING) {
			MessageBox(__T("service already running\n"), __T("Button2"));
		}
	}

	CloseServiceHandle(serviceHandle);
}


void CDriverLoadDlg::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码
	CString path = this->getFilePath();
	if (path.IsEmpty()) {
		MessageBox(__T("no file select\n"), __T("Button2"));
		return;
	}
	if (this->scManager == NULL) {
		MessageBox(__T("pls restart service scManager open error\n"), __T("Button2"));
		return;
	}

	CString fileName = this->getFileName();

	SC_HANDLE serviceHandle = OpenService(this->scManager, fileName, SERVICE_ALL_ACCESS);
	if (serviceHandle == NULL) {
		DWORD error = GetLastError();
		if (error == ERROR_SERVICE_DOES_NOT_EXIST) {
			MessageBox(__T("service does not exist\n"), __T("Button3"));
		}
		else {
			CString str;
			str.Format(L"error code=%d\n", error);
			MessageBox(str, __T("Button3"));
			OutputDebugString(str);
		}
		return;
	}

	SERVICE_STATUS error = { 0 };
	// 停止服务 此外还有 PAUSE暂停服务 和 CONTINUE继续服务
	// SERVICE_CONTROL_PAUSE SERVICE_CONTROL_CONTINUE
	int result = ControlService(serviceHandle, SERVICE_CONTROL_STOP, &error);
	if (result == 0) {
		DWORD error = GetLastError();
		if (error == ERROR_SERVICE_NOT_ACTIVE) {
			MessageBox(__T("service does not running\n"), __T("Button3"));
		}
		else {
			CString str;
			str.Format(L"error code=%d\n", error);
			MessageBox(str, __T("Button3"));
			OutputDebugString(str);
		}
		return;
	}

	CloseServiceHandle(serviceHandle);
}


void CDriverLoadDlg::OnBnClickedButton4()
{
	// TODO: 在此添加控件通知处理程序代码
	CString path = this->getFilePath();
	if (path.IsEmpty()) {
		MessageBox(__T("no file select\n"), __T("Button2"));
		return;
	}
	if (this->scManager == NULL) {
		MessageBox(__T("pls restart service scManager open error\n"), __T("Button2"));
		return;
	}

	CString fileName = this->getFileName();

	SC_HANDLE serviceHandle = OpenService(this->scManager, fileName, SERVICE_ALL_ACCESS);
	if (serviceHandle == NULL) {
		DWORD error = GetLastError();
		if (error == ERROR_SERVICE_DOES_NOT_EXIST) {
			MessageBox(__T("service does not exist\n"), __T("Button4"));
		}
		else {
			CString str;
			str.Format(L"error code=%d\n", error);
			MessageBox(str, __T("Button4"));
			OutputDebugString(str);
		}
		return;
	}
	// 删除服务 从注册表中删除
	if (!DeleteService(serviceHandle)) {
		DWORD error = GetLastError();
		CString str;
		str.Format(L"DeleteService error code=%d\n", error);
		MessageBox(str, __T("Button4"));
		return;
	}

	CloseServiceHandle(serviceHandle);
	// 最后删除服务管理器 并且置NULL
	if (!CloseServiceHandle(this->scManager)) {
		DWORD error = GetLastError();
		CString str;
		str.Format(L"CloseServiceHandle(this->scManager) error code=%d\n", error);
		MessageBox(str, __T("Button4"));
		return;
	}
	this->scManager = NULL;
}

