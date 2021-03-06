/*  Copyright (C) 2010-2011  kaosu (qiupf2000@gmail.com)
 *  This file is part of the Interactive Text Hooker.

 *  Interactive Text Hooker is free software: you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "main.h"
#include "cmdq.h"
#include "profile.h"
#include <CommCtrl.h>
WCHAR name[]=L"ith.exe";

static WCHAR exist[]=L"ITH_PIPE_EXIST";
static WCHAR mutex[]=L"ITH_RUNNING";
static WCHAR message[]=L"Cant't enable SeDebugPrevilege. ITH might malfunction.\r\n\
Please run ITH as administrator or turn off UAC.";
static WCHAR new_year[]=L"Happy Chinese Lunar New Year!";
static WCHAR nygala[]=L"CCTV New Year's Gala will start after";
extern LPCWSTR ClassName,ClassNameAdmin;
HINSTANCE hIns;
TextBuffer			*texts;
HookManager		*man;
ProfileManager		*pfman;
CommandQueue	*cmdq;
BitMap					*pid_map;
HANDLE hPipeExist;
RECT window;
bool	running=true,admin=false;
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, RECT *rc);

//extern LPWSTR GetModulePath();
void GetDebugPriv(void)
{
	HANDLE	hToken;
	UINT_PTR	dwRet;
	TOKEN_PRIVILEGES Privileges={1,{0x14,0}};
	Privileges.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
	NtOpenProcessToken(NtCurrentProcess(), TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&hToken);	
	if (STATUS_SUCCESS==NtAdjustPrivilegesToken(hToken,FALSE,&Privileges,sizeof(Privileges),NULL,&dwRet))
		admin=true;
	NtClose(hToken);
}
void SaveSettings()
{
	HANDLE hFile=IthCreateFile(L"ITH64.ini",GENERIC_WRITE,FILE_SHARE_READ,FILE_OVERWRITE_IF);
	if (hFile!=INVALID_HANDLE_VALUE)
	{
		char* buffer=new char[0x1000];
		char* ptr=buffer;
		IO_STATUS_BLOCK ios;
		ptr+=sprintf(ptr,"split_time=%d\r\n",split_time);
		ptr+=sprintf(ptr,"process_time=%d\r\n",process_time);
		ptr+=sprintf(ptr,"inject_delay=%d\r\n",inject_delay);
		ptr+=sprintf(ptr,"insert_delay=%d\r\n",insert_delay);
		ptr+=sprintf(ptr,"auto_inject=%d\r\n",auto_inject);
		ptr+=sprintf(ptr,"auto_insert=%d\r\n",auto_insert);
		ptr+=sprintf(ptr,"auto_copy=%d\r\n",clipboard_flag);
		ptr+=sprintf(ptr,"auto_suppress=%d\r\n",cyclic_remove);
		RECT rc;
		if (IsWindow(hMainWnd))
		{
			GetWindowRect(hMainWnd,&rc);
			ptr+=sprintf(ptr,"window_left=%d\r\n",rc.left);
			ptr+=sprintf(ptr,"window_right=%d\r\n",rc.right);
			ptr+=sprintf(ptr,"window_top=%d\r\n",rc.top);
			ptr+=sprintf(ptr,"window_bottom=%d\r\n",rc.bottom);
		}
		NtWriteFile(hFile,0,0,0,&ios,buffer,ptr-buffer+1,0,0);
		delete buffer;
		NtClose(hFile);
	}
}
void LoadSettings()
{
	HANDLE hFile=IthCreateFile(L"ITH64.ini",GENERIC_READ,FILE_SHARE_READ,FILE_OPEN);
	if (hFile!=INVALID_HANDLE_VALUE)
	{
		IO_STATUS_BLOCK ios;
		FILE_STANDARD_INFORMATION info;
		NtQueryInformationFile(hFile,&ios,&info,sizeof(info),FileStandardInformation);
		char* buffer=new char[info.AllocationSize.LowPart];
		char* ptr;
		NtReadFile(hFile,0,0,0,&ios,buffer,info.AllocationSize.LowPart,0,0);
		ptr=strstr(buffer,"split_time");
		if (ptr==0) ptr=buffer;
		if (sscanf(ptr,"split_time=%d",&split_time)==0) split_time=200;

		ptr=strstr(ptr,"process_time");
		if (ptr==0) ptr=buffer;
		if (sscanf(ptr,"process_time=%d",&process_time)==0) process_time=50;

		ptr=strstr(ptr,"inject_delay");
		if (ptr==0) ptr=buffer;
		if (sscanf(ptr,"inject_delay=%d",&inject_delay)==0) inject_delay=3000;

		ptr=strstr(ptr,"insert_delay");
		if (ptr==0) ptr=buffer;
		if (sscanf(ptr,"insert_delay=%d",&insert_delay)==0) insert_delay=500;

		ptr=strstr(ptr,"auto_inject");
		if (ptr==0) ptr=buffer;
		if (sscanf(ptr,"auto_inject=%d",&auto_inject)==0) auto_inject=1;
		auto_inject=auto_inject>0?1:0;

		ptr=strstr(ptr,"auto_insert");
		if (ptr==0) ptr=buffer;
		if (sscanf(ptr,"auto_insert=%d",&auto_insert)==0) auto_insert=1;
		auto_insert=auto_insert>0?1:0;

		ptr=strstr(ptr,"auto_copy");
		if (ptr==0) ptr=buffer;
		if (sscanf(ptr,"auto_copy=%d",&clipboard_flag)==0) clipboard_flag=1;
		clipboard_flag=clipboard_flag>0?1:0;

		ptr=strstr(ptr,"auto_suppress");
		if (ptr==0) ptr=buffer;
		if (sscanf(ptr,"auto_suppress=%d",&cyclic_remove)==0) cyclic_remove=1;
		cyclic_remove=cyclic_remove>0?1:0;

		ptr=strstr(ptr,"window_left");
		if (ptr==0) ptr=buffer;
		if (sscanf(ptr,"window_left=%d",&window.left)==0) window.left=100;

		ptr=strstr(ptr,"window_right");
		if (ptr==0) ptr=buffer;
		if (sscanf(ptr,"window_right=%d",&window.right)==0) window.right=700;

		ptr=strstr(ptr,"window_top");
		if (ptr==0) ptr=buffer;
		if (sscanf(ptr,"window_top=%d",&window.top)==0) window.top=100;

		ptr=strstr(ptr,"window_bottom");
		if (ptr==0) ptr=buffer;
		if (sscanf(ptr,"window_bottom=%d",&window.bottom)==0) window.bottom=600;
		delete buffer;
		NtClose(hFile);
	}
	else
	{
		split_time=200;
		process_time=50;
		inject_delay=3000;
		insert_delay=500;
		auto_inject=1;
		auto_insert=1;
		clipboard_flag=0;
		cyclic_remove=0;
		window.left=100;
		window.top=100;
		window.right=800;
		window.bottom=600;
	}
}
int Init()
{
	IthInitSystemService();
	UINT_PTR s;
	HANDLE hm=IthCreateMutex(mutex,1,&s);
	if (s)
	{
		HWND hwnd=FindWindow(ClassName,ClassName);
		if (hwnd==0) hwnd=FindWindow(ClassName,ClassNameAdmin);
		if (hwnd)
		{
			ShowWindow(hwnd,SW_SHOWNORMAL);
			SetForegroundWindow(hwnd);
		}
		return 1;
	}
	hPipeExist=IthCreateEvent(exist);
	NtSetEvent(hPipeExist,0);
	GetDebugPriv();
	return 0;
}
UINT_PTR GetModuleBase()
{
	return (UINT_PTR)peb->ImageBaseAddress;
}
int main()
{
	MSG msg;
	if (Init()) goto _exit;

	hIns=(HINSTANCE)GetModuleBase();
	MyRegisterClass(hIns);
	InitCommonControls();
	LoadSettings();
	InitInstance(hIns,admin,&window);
	InitializeCriticalSection(&detach_cs);
	
	pid_map=new BitMap;
	texts=new TextBuffer;
	man=new HookManager;
	pfman=new ProfileManager;
	cmdq=new CommandQueue;
	CreateNewPipe();
	NtClose(IthCreateThread(CmdThread,0));
	if (!admin) man->AddConsoleOutput(message);
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	NtClearEvent(hPipeExist);
	delete cmdq;
	delete pfman;
	delete man;
	delete texts;
	delete pid_map;
_exit:
	IthCloseSystemService();
	NtTerminateProcess(NtCurrentProcess());
}
