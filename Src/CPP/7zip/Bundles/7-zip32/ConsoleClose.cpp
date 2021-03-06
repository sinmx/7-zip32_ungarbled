// ConsoleClose.cpp

#include "StdAfx.h"

#include "ConsoleClose.h"

#if !defined(UNDER_CE) && defined(_WIN32)
#include "../../../Common/MyWindows.h"
#endif

//namespace NConsoleClose {							// 削除

unsigned g_BreakCounter = 0;
static const unsigned kBreakAbortThreshold = 2;

#if !defined(UNDER_CE) && defined(_WIN32)
//static BOOL WINAPI HandlerRoutine(DWORD ctrlType)		// 削除
BOOL HandlerRoutine()									// 追加
{
  /* 削除ここから
  if (ctrlType == CTRL_LOGOFF_EVENT)
  {
    // printf("\nCTRL_LOGOFF_EVENT\n");
    return TRUE;
  }
     削除ここまで */

  g_BreakCounter++;
  if (g_BreakCounter < kBreakAbortThreshold)
    return TRUE;
  return FALSE;
  /*
  switch (ctrlType)
  {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
      if (g_BreakCounter < kBreakAbortThreshold)
      return TRUE;
  }
  return FALSE;
  */
}
#endif

namespace NConsoleClose {			// 追加
/*
void CheckCtrlBreak()
{
  if (TestBreakSignal())
    throw CCtrlBreakException();
}
*/

CCtrlHandlerSetter::CCtrlHandlerSetter()
{
  #if !defined(UNDER_CE) && defined(_WIN32)
//  if (!SetConsoleCtrlHandler(HandlerRoutine, TRUE))	// 削除
//    throw "SetConsoleCtrlHandler fails";				// 削除
  #endif
}

CCtrlHandlerSetter::~CCtrlHandlerSetter()
{
  #if !defined(UNDER_CE) && defined(_WIN32)
//  if (!SetConsoleCtrlHandler(HandlerRoutine, FALSE))	// 削除
  {
    // warning for throw in destructor.
    // throw "SetConsoleCtrlHandler fails";
	g_BreakCounter = 0;									// 追加
  }
  #endif
}

}
