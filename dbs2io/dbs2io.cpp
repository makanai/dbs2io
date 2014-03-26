/----------------------------------------------------------------------------
// dbs2io.cpp :
// 指定されたプロセスを起動し、OutputDebugStringの出力をフックして標準出力へ出力する。
//
// Usege: dbs2io.exe [-exec:execFilename] [-work:currentDirectory] [-exit:exitOnText] [-time:timeOutSec]
// -exec: 実行ファイル名
// -work: 実行ファイルのワーキングディレクトリ
// -exit: 終了文字列。これが標準出力に出るとdbs2ioは正常終了する。
// -time: タイムアウト時間。この時間以上の標準出力が無い場合、dbs2ioはエラー終了する。
//
// 使い方としては。
// 1. デバッグ出力で動作するアプリケーションを、dbs2io経由で起動する。デバッグ出力が標準出力に流れるようになる。
// 2. テスト組む。正常終了時にキーワードを出力するようにしとく。仮に"success"とする。
// 3. dbs2ioで-exitでキーワード"success"を指定すると、dbs2io経由で正常終了を検出できるようになる。
// 4. dbs2ioで-timeで時間指定すると、何らかの理由でテストが終わらない場合、エラーを検出できるようになる。
// という感じ。
// ps3ctr, psp2ctrあたりを参考にWin32/Win64で互換動作するツール、という位置づけ。
/----------------------------------------------------------------------------

#include "stdafx.h"
#include <windows.h>
#include <mmsystem.h>

#define BUFFSIZE	1024

typedef struct tagDEBUGMODULEINFO {
	DWORD				dwPid;
	LPSTR				lpszPathName;
	LPSTR				lpszWorkName;
	LPSTR				lpszExitText;
	STARTUPINFO			si;
	PROCESS_INFORMATION pi;
	float				limitSec;
	HANDLE				hDbgThread;
	DWORD				dwExitCode;
} DEBUGMODULEINFO;
typedef DEBUGMODULEINFO* LPDEBUGMODULEINFO;


//---------------------------------------------------------------------------
//	デバッグ文字列を取得する
//---------------------------------------------------------------------------
BOOL GetOutputDebugString(
HANDLE hProcess, LPDEBUG_EVENT lpDebug, LPTSTR lpszDebugString)
{
	DWORD	dwRead;
	char	szBuff[BUFFSIZE];

	//デバッグ文字列を取得する
	ReadProcessMemory(hProcess,
			lpDebug->u.DebugString.lpDebugStringData,
			szBuff, lpDebug->u.DebugString.nDebugStringLength, &dwRead);
	*(szBuff+dwRead) = '\0';

	//取得した文字列がUNICODEのときは変換を行う
	if(lpDebug->u.DebugString.fUnicode){
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK,
				(LPWSTR)szBuff, -1, lpszDebugString, BUFFSIZE, NULL, NULL);
	}else{
		lstrcpy(lpszDebugString, szBuff);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//	デバッグイベントループ
//---------------------------------------------------------------------------
void DebugEventLoop(LPDEBUGMODULEINFO lpdmi)
{
	DEBUG_EVENT debug;
	char		szBuff[BUFFSIZE];

	float	totalSec = 0;
	DWORD	ctr1 = timeGetTime();

	while(TRUE){
		BOOL	result = WaitForDebugEvent(&debug, INFINITE);
		DWORD	ctr2 = timeGetTime();
		float	sec = (ctr2 - ctr1) / 1000.f;
		ctr1 = ctr2;
		totalSec += sec;

		if( lpdmi->limitSec>0 && totalSec>=lpdmi->limitSec ){
			printf( "timeout. %f %f\n", totalSec, lpdmi->limitSec );
			lpdmi->dwExitCode = 1;
			return;
		}

		if( !result ){
			printf("WaitForDebugEvent Fail (%d)", GetLastError());
			return;
		}

		switch(debug.dwDebugEventCode){
		case OUTPUT_DEBUG_STRING_EVENT:
			*szBuff = '\0';
			GetOutputDebugString(lpdmi->pi.hProcess, &debug, szBuff);
			printf( "%s", szBuff );
			totalSec = 0;
			if( lpdmi->lpszExitText ){
				if( strstr( szBuff, lpdmi->lpszExitText ) ){
					printf( "find exit string.\n" );
					return;
				}
			}
			break;
		case EXIT_PROCESS_DEBUG_EVENT:
			printf( "exit process, code=%d\n", debug.u.ExitProcess.dwExitCode );
			return;
		default:
			break;
		}
		ContinueDebugEvent(debug.dwProcessId, debug.dwThreadId, DBG_CONTINUE);
	}
}

//---------------------------------------------------------------------------
//	デバッグモジュールの実行
//---------------------------------------------------------------------------
void DebugTherad(LPVOID lpvParam)
{
	LPDEBUGMODULEINFO	lpdmi = (LPDEBUGMODULEINFO)lpvParam;
	BOOL				fResult = FALSE;

	__try{
		if(lpdmi->pi.dwProcessId == 0L){
			fResult = CreateProcess(NULL, lpdmi->lpszPathName,
					NULL, NULL, TRUE,
					//DEBUG_PROCESS | CREATE_NEW_CONSOLE | NORMAL_PRIORITY_CLASS,
					CREATE_NEW_CONSOLE | NORMAL_PRIORITY_CLASS,
					NULL, lpdmi->lpszWorkName, &(lpdmi->si), &(lpdmi->pi));
			if(fResult == FALSE){
				printf("CreateProcess Fail (%d)", GetLastError());
				__leave;
			}
			//Sleep( 2000 );//なにかこのタイミングでプロセス起動に失敗することがある。。。スリープしてみる。
			WaitForInputIdle( lpdmi->pi.hProcess, INFINITE );//sleepではなく、プロセスの初期化終了を待つ。こちらが安定っぽい。
			DebugActiveProcess(lpdmi->pi.dwProcessId);
			CloseHandle(lpdmi->pi.hThread);
		}	else{
			lpdmi->pi.hProcess = OpenProcess(
					PROCESS_ALL_ACCESS, FALSE, lpdmi->pi.dwProcessId);
			if(lpdmi->pi.hProcess == NULL){
				printf("OpenProcess Fail (%d)", GetLastError());
				__leave;
			}
			DebugActiveProcess(lpdmi->pi.dwProcessId);
		}

		DebugEventLoop(lpdmi);
	}
	__finally{
		DWORD	dwExitCode = lpdmi->dwExitCode;
		HANDLE	hProcess = lpdmi->pi.hProcess;
		HANDLE	hThread = lpdmi->hDbgThread;
		memset( lpdmi, 0, sizeof(DEBUGMODULEINFO) );
		LocalFree(lpdmi);

		CloseHandle(hProcess);
		ExitThread( dwExitCode );
	}
}

//---------------------------------------------------------------------------
//	デバッグモジュールの実行
//---------------------------------------------------------------------------
HANDLE	StartDebug(DWORD dwPid, LPSTR lpszFileName, LPSTR lpszWorkName, LPSTR lpszExitText, float limitSec )
{
	static LPDEBUGMODULEINFO	lpdmi = (LPDEBUGMODULEINFO)LocalAlloc(LPTR, sizeof(DEBUGMODULEINFO));
	lpdmi->dwExitCode = 0;
	if(dwPid == 0L){
		lpdmi->si.cb          = sizeof(STARTUPINFO);
		lpdmi->si.lpTitle     = lpszFileName;
		lpdmi->si.wShowWindow = SW_SHOWDEFAULT;
		lpdmi->lpszPathName   = lpszFileName;
		lpdmi->lpszWorkName   = lpszWorkName;
		lpdmi->lpszExitText   = lpszExitText;
		lpdmi->limitSec       = limitSec;
	}
	else{
		lpdmi->pi.dwProcessId = dwPid;
	}

	DWORD	dwDbgThreadId;
	lpdmi->hDbgThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DebugTherad, (LPVOID)lpdmi, 0, &dwDbgThreadId );
	if( lpdmi->hDbgThread == NULL ){
		printf("CreateThread Fail (%d)", GetLastError());
		LocalFree(lpdmi);
		return 0;
	}
	return lpdmi->hDbgThread;
}

//---------------------------------------------------------------------------
// main
//---------------------------------------------------------------------------
int _tmain(int argc, _TCHAR* argv[])
{
	if( argc==1 ){
		printf( "dbs2io.exe [-exec:execFilename] [-work:currentDirectory] [-exit:exitOnText] [-time:timeOutSec]\n" );	
		return 0;
	}

	_TCHAR*	execName = "\0";
	_TCHAR*	workName = "\0";
	_TCHAR*	exitText = NULL;
	float	time = 0;
	for( int i=0; i<argc; i++ ){
		if( argv[i][0]!='-' && argv[i][0]!='/' ){
			continue;
		}
		_TCHAR*	name = &argv[i][1];
		if( strncmp( name, "exec", strlen("exec") )==0 ){
			if( strlen(name)>strlen("exec:") ){
				execName = &name[5];
			}
		}
		if( strncmp( name, "work", strlen("work") )==0 ){
			if( strlen(name)>strlen("work:") ){
				workName = &name[5];
			}
		}
		if( strncmp( name, "exit", strlen("exit") )==0 ){
			if( strlen(name)>strlen("exit:") ){
				exitText = &name[5];
			}
		}
		if( strncmp( name, "time", strlen("time") )==0 ){
			if( strlen(name)>strlen("time:") ){
				time = (float)atof( &name[5] );
			}
		}
	}

	HANDLE	hDbgThread = StartDebug(0, execName, workName, exitText, time );

	DWORD	exitCode = 0;
	for(;;){
		if( hDbgThread==0 ){
			break;
		}
		BOOL	result = GetExitCodeThread( hDbgThread, &exitCode );
		if( result==0 ){
			if( exitCode==STILL_ACTIVE ){
//				exitCode = 0;//Xクリック等で終了した場合、STILL_ACTIVEのままになっている。
			}
			break;
		}
		if( exitCode!=STILL_ACTIVE ){
			break;
		}
	}
	if( hDbgThread ){
		CloseHandle(hDbgThread);
	}

	return exitCode;
}

