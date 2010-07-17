/*
This .dll implements functionality that get position from text on some window using Hook API call technology.
Completed by Neil Wang.
*/
#include "stdafx.h"
#include "detours.h"
#define UPDATE_MESSAGE WM_USER+1
#ifdef _MANAGED
#pragma managed(push, off)
#endif

#pragma data_seg("Shared")
HWND targetHandle = NULL;
wchar_t text[256] = {0};
wchar_t buffer[256] = {0};
int xPos = -1;
int yPos = -1;
int targetIndex = -1;
int startIndex = -1;
BOOL hasGetPosition = FALSE;
BOOL IsFromPostMessage = FALSE;
HHOOK g_hook = NULL;
#pragma data_seg()
#pragma comment(linker, "/Section:Shared,rws")

HMODULE ModuleFromAdress(PVOID p)
{
	MEMORY_BASIC_INFORMATION mbi;
	SIZE_T result = VirtualQuery(p,&mbi,sizeof(mbi));
	if(result != 0)
		return (HMODULE)mbi.AllocationBase;
	return NULL;
}

static BOOL (WINAPI * TrueTextOutW)(HDC hdc,int x, int y,LPCWSTR lpstring,int c) = TextOutW;
static BOOL (WINAPI * TrueExtTextOutW)(HDC hdc, int nXStart, int nYStart, UINT fuOptions,const RECT FAR *lprc, LPCWSTR lpszString,UINT cbString,const INT* lpDx) = ExtTextOutW;

BOOL MatchString(LPCWSTR source, LPCWSTR expression)
{
	if(expression == NULL)
	{
		return (source == NULL);
	}
	if(*expression == L'\0')
	{
		return (*source == L'\0');
	}
	if(*expression == L'*')
	{
		int sourceLen = lstrlen(source);
		for(int i=0; i<=sourceLen; i++)
		{
			if(MatchString(source + i,expression + 1))
			    return TRUE;
		}
		return FALSE;
	}
	/*else if(*expression == L'?') for extention
	{
        for(int i=0; i<2; i++)
		{
			if(MatchString(source + i,expression + 1))
			    return TRUE;
		}
		return FALSE;
	}*/
	else
	{
		if(*source == *expression)
		{
			return MatchString(++source,++expression);
		}
		else
		{
			return FALSE;
		}
	}
}

VOID UpdateTargetWindow()
{
	RECT rect;
    GetClientRect((HWND)targetHandle,&rect);
	InvalidateRect((HWND)targetHandle,&rect,TRUE);
	UpdateWindow((HWND)targetHandle);
}

BOOL WINAPI SUITextOutW(HDC hdc,int x, int y,LPCWSTR lpstring,int c)
{
	try
	{
	   if(targetHandle != NULL)
	   {
           if(c > 64 || c < 0)
		   {
			   return TrueTextOutW(hdc,x,y,lpstring,c);
		   }
		   else
		   {
			  if(IsFromPostMessage&&!hasGetPosition)
			  {
				   if(lstrcmp(text,lpstring) == 0)
				   {
						startIndex++;
						if(startIndex == targetIndex)
						{
							hasGetPosition = TRUE;
							IsFromPostMessage = FALSE;
							targetHandle = NULL;
							xPos = x;
							yPos = y;
						}
				   }
				   else
				   {
					   if(MatchString(lpstring,text))
					   {
							startIndex++;
							if(startIndex == targetIndex)
							{
								hasGetPosition = TRUE;
								IsFromPostMessage = FALSE;
								targetHandle = NULL;
								xPos = x;
								yPos = y;
							}
					   }
				   }
			  }
              return TrueTextOutW(hdc,x,y,lpstring,c);
		   }
	   } 
	   else
	   {
           return TrueTextOutW(hdc,x,y,lpstring,c);
	   }
	} 
	catch(...)
	{
	    return FALSE;
	}
}

BOOL WINAPI SUIExtTextOutW(HDC hdc, int nXStart, int nYStart, UINT fuOptions,const RECT FAR *lprc, LPCWSTR lpszString,UINT cbString,const INT *lpDx)
{
try
	{
	   if(targetHandle != NULL)
	   {
           if(cbString > 64)
		   {
			   return TrueExtTextOutW(hdc, nXStart, nYStart, fuOptions,lprc, lpszString,cbString,lpDx);
		   }
		   else
		   {
			  //wchar_t buffer[64] = {0};
			  //lstrcpy(buffer,lpszString);
			  memset(buffer,0,512);
              lstrcpy(buffer,lpszString);
			  int tempX = nXStart;
			  int tempY = nYStart;
			  if(IsFromPostMessage&&!hasGetPosition)
			  {
				   if(lstrcmp(text,buffer) == 0)
				   {
						startIndex++;
						if(startIndex == targetIndex)
						{
							IsFromPostMessage = FALSE;
							hasGetPosition = TRUE;
							targetIndex = -1;
							targetHandle = NULL;
							xPos = tempX;
							yPos = tempY;
						}
				   }
				   else
				   {
					   if(MatchString(buffer,text))
					   {
							startIndex++;
							if(startIndex == targetIndex)
							{
								IsFromPostMessage = FALSE;
								hasGetPosition = TRUE;
								targetIndex = -1;
								targetHandle = NULL;
								xPos = tempX;
								yPos = tempY;
							}
					   }
				   }
			  }
              return TrueExtTextOutW(hdc, nXStart, nYStart, fuOptions,lprc,buffer,cbString,lpDx);
		   }
	   } 
	   else
	   {
           return TrueExtTextOutW(hdc, nXStart, nYStart, fuOptions,lprc, lpszString,cbString,lpDx);
	   }
	} 
	catch(...)
	{
	    return FALSE;
		//return TrueExtTextOutW(hdc, nXStart, nYStart, fuOptions,lprc, lpszString,cbString,lpDx);
	}
}

static VOID ModifyFuncEntry()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)TrueExtTextOutW, SUIExtTextOutW);
	DetourAttach(&(PVOID&)TrueTextOutW, SUITextOutW);
	DetourTransactionCommit();
}

static VOID UnModifyFunction()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)TrueExtTextOutW, SUIExtTextOutW);
	DetourDetach(&(PVOID&)TrueTextOutW, SUITextOutW);
	DetourTransactionCommit();
}

LRESULT CALLBACK GetMsgPro(int nCode, WPARAM wParam, LPARAM lParam)
{
	try
	{
        if(nCode == HC_ACTION)
		{
			PMSG pMsg = (PMSG)lParam;
			if(pMsg->message == UPDATE_MESSAGE)
			{
				IsFromPostMessage = TRUE;
				targetHandle = pMsg->hwnd;
				UpdateTargetWindow();
			}
		}
	}
	catch(...)
	{}
	return CallNextHookEx(g_hook, nCode,wParam, lParam);
}

VOID PlaceHolder()
{

}

static VOID InstallGlobalHook()
{ 
    if(g_hook == NULL)
	{
		g_hook = SetWindowsHookEx(WH_GETMESSAGE,(HOOKPROC)GetMsgPro,ModuleFromAdress(PlaceHolder),NULL);
	}
	
}
static VOID UnstallGlbalHook()
{
	if(g_hook != NULL)
	{
		UnhookWindowsHookEx(g_hook);
	    g_hook = NULL;
	}

}
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  dwReason,
                       LPVOID lpReserved
					 )
{
    try
	{
        if (dwReason == DLL_PROCESS_ATTACH) {
		    ModifyFuncEntry();
        }
		else if (dwReason == DLL_PROCESS_DETACH) {
			UnModifyFunction();
		}
	}
	catch(...)
	{}
    return TRUE;
}

extern "C" _declspec (dllexport) VOID UnInstallHook()
{
	if(g_hook != NULL)
	{
       UnhookWindowsHookEx(g_hook);
	   g_hook = NULL;
	}
}

extern "C" _declspec (dllexport) VOID InstallHook(HWND winhandle)
{
	UnInstallHook();
	DWORD threadId = GetWindowThreadProcessId(winhandle,NULL);
	g_hook = SetWindowsHookEx(WH_GETMESSAGE,(HOOKPROC)GetMsgPro,ModuleFromAdress(PlaceHolder),threadId);
}

extern "C" _declspec (dllexport) POINT GetPointFromTextWIndex(HWND winhandle,LPWSTR target,int index)
{
    try
	{
		Sleep(200);
		memset(text,0,512);
		hasGetPosition = FALSE;
		IsFromPostMessage = FALSE;
		lstrcpy(text,target);
		xPos = -1;
		yPos = -1;
		startIndex = -1;
		targetIndex = index;
		InstallHook(winhandle);
		Sleep(200);
		PostMessage(winhandle,UPDATE_MESSAGE,NULL,NULL);
		Sleep(200);
		int counter = 0;
		while((!hasGetPosition) && (counter < 20))
		{
			Sleep(100);
			counter++;
		}
		POINT p;
		p.x = xPos;
		p.y = yPos;
		return p;
	}
	catch(...)
	{
		POINT p2;
		p2.x = -1;
		p2.y = -1;
		return p2;
	}
	
}

extern "C" _declspec (dllexport) POINT GetPointFromTextW(HWND winhandle,LPWSTR target)
{
    return GetPointFromTextWIndex(winhandle,target,0);
}

extern "C" _declspec (dllexport) POINT GetPointFromTextIndex(HWND winhandle,LPWSTR target,int index)
{
   return GetPointFromTextWIndex(winhandle,target,index); 
}

extern "C" _declspec (dllexport) POINT GetPointFromText(HWND winhandle,LPWSTR target)
{
    return GetPointFromTextIndex(winhandle,target,0);
}

extern "C" _declspec (dllexport) int GetX()
{
   return xPos;
}
extern "C" _declspec (dllexport) int GetY()
{
   return yPos;
}


#ifdef _MANAGED
#pragma managed(pop)
#endif

