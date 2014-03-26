/----------------------------------------------------------------------------
// dbs2io.cpp :
// �w�肳�ꂽ�v���Z�X���N�����AOutputDebugString�̏o�͂��t�b�N���ĕW���o�͂֏o�͂���B
//
// Usege: dbs2io.exe [-exec:execFilename] [-work:currentDirectory] [-exit:exitOnText] [-time:timeOutSec]
// -exec: ���s�t�@�C����
// -work: ���s�t�@�C���̃��[�L���O�f�B���N�g��
// -exit: �I��������B���ꂪ�W���o�͂ɏo���dbs2io�͐���I������B
// -time: �^�C���A�E�g���ԁB���̎��Ԉȏ�̕W���o�͂������ꍇ�Adbs2io�̓G���[�I������B
//
// �g�����Ƃ��ẮB
// 1. �f�o�b�O�o�͂œ��삷��A�v���P�[�V�������Adbs2io�o�R�ŋN������B�f�o�b�O�o�͂��W���o�͂ɗ����悤�ɂȂ�B
// 2. �e�X�g�g�ށB����I�����ɃL�[���[�h���o�͂���悤�ɂ��Ƃ��B����"success"�Ƃ���B
// 3. dbs2io��-exit�ŃL�[���[�h"success"���w�肷��ƁAdbs2io�o�R�Ő���I�������o�ł���悤�ɂȂ�B
// 4. dbs2io��-time�Ŏ��Ԏw�肷��ƁA���炩�̗��R�Ńe�X�g���I���Ȃ��ꍇ�A�G���[�����o�ł���悤�ɂȂ�B
// �Ƃ��������B
// ps3ctr, psp2ctr��������Q�l��Win32/Win64�Ō݊����삷��c�[���A�Ƃ����ʒu�Â��B
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
//	�f�o�b�O��������擾����
//---------------------------------------------------------------------------
BOOL GetOutputDebugString(
HANDLE hProcess, LPDEBUG_EVENT lpDebug, LPTSTR lpszDebugString)
{
	DWORD	dwRead;
	char	szBuff[BUFFSIZE];

	//�f�o�b�O��������擾����
	ReadProcessMemory(hProcess,
			lpDebug->u.DebugString.lpDebugStringData,
			szBuff, lpDebug->u.DebugString.nDebugStringLength, &dwRead);
	*(szBuff+dwRead) = '\0';

	//�擾����������UNICODE�̂Ƃ��͕ϊ����s��
	if(lpDebug->u.DebugString.fUnicode){
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK,
				(LPWSTR)szBuff, -1, lpszDebugString, BUFFSIZE, NULL, NULL);
	}else{
		lstrcpy(lpszDebugString, szBuff);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//	�f�o�b�O�C�x���g���[�v
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
//	�f�o�b�O���W���[���̎��s
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
			//Sleep( 2000 );//�Ȃɂ����̃^�C�~���O�Ńv���Z�X�N���Ɏ��s���邱�Ƃ�����B�B�B�X���[�v���Ă݂�B
			WaitForInputIdle( lpdmi->pi.hProcess, INFINITE );//sleep�ł͂Ȃ��A�v���Z�X�̏������I����҂B�����炪������ۂ��B
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
//	�f�o�b�O���W���[���̎��s
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
//				exitCode = 0;//X�N���b�N���ŏI�������ꍇ�ASTILL_ACTIVE�̂܂܂ɂȂ��Ă���B
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

