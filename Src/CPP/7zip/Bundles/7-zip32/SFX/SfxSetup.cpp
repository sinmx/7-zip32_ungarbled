// Main.cpp

#include "StdAfx.h"

#include "../../../../Common/MyWindows.h"		// �p�X�ύX

#include "../../../../Common/MyInitGuid.h"		// �p�X�ύX

#include "../../../../Common/CommandLineParser.h"		// �p�X�ύX
#include "../../../../Common/StringConvert.h"		// �p�X�ύX
#include "../../../../Common/TextConfig.h"		// �p�X�ύX

#include "../../../../Windows/DLL.h"		// �p�X�ύX
#include "../../../../Windows/ErrorMsg.h"		// �p�X�ύX
#include "../../../../Windows/FileDir.h"		// �p�X�ύX
#include "../../../../Windows/FileFind.h"		// �p�X�ύX
#include "../../../../Windows/FileIO.h"		// �p�X�ύX
#include "../../../../Windows/FileName.h"		// �p�X�ύX
#include "../../../../Windows/NtCheck.h"		// �p�X�ύX
#include "../../../../Windows/ResourceString.h"		// �p�X�ύX

#include "../../../UI/Explorer/MyMessages.h"		// �p�X�ύX

#include "../../SFXSetup/ExtractEngine.h"		// �p�X�ύX

#include "resource.h"

#include "ExtractDialog.h"				// �ǉ�
#include "Windows/ResourceString.h"		// �ǉ�

using namespace NWindows;
using namespace NFile;
using namespace NDir;

HINSTANCE g_hInstance;

static CFSTR kTempDirPrefix = FTEXT("7zSDJC");		// �ύX

#define _SHELL_EXECUTE

static bool ReadDataString(CFSTR fileName, LPCSTR startID,
    LPCSTR endID, AString &stringResult)
{
  stringResult.Empty();
  NIO::CInFile inFile;
  if (!inFile.Open(fileName))
    return false;
  const int kBufferSize = (1 << 12);

  Byte buffer[kBufferSize];
  int signatureStartSize = MyStringLen(startID);
  int signatureEndSize = MyStringLen(endID);
  
  UInt32 numBytesPrev = 0;
  bool writeMode = false;
  UInt64 posTotal = 0;
  for (;;)
  {
    if (posTotal > (1 << 20))
      return (stringResult.IsEmpty());
    UInt32 numReadBytes = kBufferSize - numBytesPrev;
    UInt32 processedSize;
    if (!inFile.Read(buffer + numBytesPrev, numReadBytes, processedSize))
      return false;
    if (processedSize == 0)
      return true;
    UInt32 numBytesInBuffer = numBytesPrev + processedSize;
    UInt32 pos = 0;
    for (;;)
    {
      if (writeMode)
      {
        if (pos > numBytesInBuffer - signatureEndSize)
          break;
        if (memcmp(buffer + pos, endID, signatureEndSize) == 0)
          return true;
        char b = buffer[pos];
        if (b == 0)
          return false;
        stringResult += b;
        pos++;
      }
      else
      {
        if (pos > numBytesInBuffer - signatureStartSize)
          break;
        if (memcmp(buffer + pos, startID, signatureStartSize) == 0)
        {
          writeMode = true;
          pos += signatureStartSize;
        }
        else
          pos++;
      }
    }
    numBytesPrev = numBytesInBuffer - pos;
    posTotal += pos;
    memmove(buffer, buffer + pos, numBytesPrev);
  }
}

static char kStartID[] = { ',','!','@','I','n','s','t','a','l','l','@','!','U','T','F','-','8','!', 0 };
static char kEndID[]   = { ',','!','@','I','n','s','t','a','l','l','E','n','d','@','!', 0 };

struct CInstallIDInit
{
  CInstallIDInit()
  {
    kStartID[0] = ';';
    kEndID[0] = ';';
  };
} g_CInstallIDInit;


#define NT_CHECK_FAIL_ACTION ShowErrorMessage(L"Unsupported Windows version"); return 1;

static void ShowErrorMessageSpec(const UString &name)
{
  UString message = NError::MyFormatMessage(::GetLastError());
  int pos = message.Find(L"%1");
  if (pos >= 0)
  {
    message.Delete(pos, 2);
    message.Insert(pos, name);
  }
  ShowErrorMessage(NULL, message);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE /* hPrevInstance */,
    #ifdef UNDER_CE
    LPWSTR
    #else
    LPSTR
    #endif
    /* lpCmdLine */,int /* nCmdShow */)
{
  g_hInstance = (HINSTANCE)hInstance;

  NT_CHECK

  // InitCommonControls();

  UString archiveName, switches;
  #ifdef _SHELL_EXECUTE
  UString executeFile, executeParameters;
  #endif
  NCommandLineParser::SplitCommandLine(GetCommandLineW(), archiveName, switches);

  FString fullPath;
  NDLL::MyGetModuleFileName(fullPath);

  switches.Trim();
  bool assumeYes = false;
  if (switches.IsPrefixedBy_Ascii_NoCase("-y"))
  {
    assumeYes = true;
    switches = switches.Ptr(2);
    switches.Trim();
  }

  AString config;
  if (!ReadDataString(fullPath, kStartID, kEndID, config))
  {
    if (!assumeYes)
      ShowErrorMessage(L"Can't load config info");
    return 1;
  }

  UString dirPrefix = L"." WSTRING_PATH_SEPARATOR;
  UString appLaunched;
  bool showProgress = true;
  UString friendlyName;		// �ǉ�
  UString installPrompt;	// �ǉ�
  bool isInstaller;			// �ǉ�
  if (!config.IsEmpty())
  {
    CObjectVector<CTextConfigPair> pairs;
    if (!GetTextConfig(config, pairs))
    {
      if (!assumeYes)
        ShowErrorMessageRes(IDS_CONFIG_FAILED);		// �ύX
      return 1;
    }
    friendlyName = GetTextConfigValue(pairs, L"Title");			// �ύX
    installPrompt = GetTextConfigValue(pairs, L"BeginPrompt");	// �ύX
    isInstaller = GetTextConfigValue(pairs, L"IsInstaller").IsEqualTo_NoCase(L"yes");	// �ǉ�
    if (isInstaller)	// �ǉ�
    {					// �ǉ�
    UString progress = GetTextConfigValue(pairs, L"Progress");
    if (progress.IsEqualTo_Ascii_NoCase("no"))
      showProgress = false;
    int index = FindTextConfigItem(pairs, L"Directory");
    if (index >= 0)
      dirPrefix = pairs[index].String;
    if (!installPrompt.IsEmpty() && !assumeYes)
    {
      if (MessageBoxW(0, installPrompt, friendlyName, MB_YESNO |
          MB_ICONQUESTION) != IDYES)
        return 0;
    }
    appLaunched = GetTextConfigValue(pairs, L"RunProgram");
    
    #ifdef _SHELL_EXECUTE
    executeFile = GetTextConfigValue(pairs, L"ExecuteFile");
    executeParameters = GetTextConfigValue(pairs, L"ExecuteParameters");
    #endif
	}	// �ǉ�
  }

  CTempDir tempDir;

  /* �ǉ� */
	UString tempDirPath;
  	if (isInstaller)
	{
		if (!tempDir.Create(kTempDirPrefix))
		{
			if (!assumeYes)
				ShowErrorMessageRes(IDS_CANT_CREATE_TEMP_FOLDER);
			return 1;
		}
		tempDirPath = GetUnicodeString(tempDir.GetPath());
	}
	else
	{
		tempDirPath = fullPath.Mid(0, fullPath.ReverseFind(L'\\') + 1);	// �ύX
		if (!assumeYes)
		{
			if (friendlyName.IsEmpty())
				friendlyName = NWindows::MyLoadString(IDS_EXTRACT_FRIENDLY_NAME);
			if (installPrompt.IsEmpty())
				installPrompt = NWindows::MyLoadString(IDS_EXTRACT_INSTALL_PROMPT);
			CExtractDialog dlg;
			if (dlg.Create(friendlyName, installPrompt, tempDirPath, 0) != IDOK)
				return 0;
			tempDirPath = dlg.GetFolderName();
		}
	}
  /* �ǉ������܂�*/
  /* �폜 
  if (!tempDir.Create(kTempDirPrefix))
  {
    if (!assumeYes)
      ShowErrorMessage(L"Can not create temp folder archive");
    return 1;
  }
     �폜�����܂� */
  CCodecs *codecs = new CCodecs;
  CMyComPtr<IUnknown> compressCodecsInfo = codecs;
  HRESULT result = codecs->Load();
  if (result != S_OK)
  {
    ShowErrorMessageRes(IDS_CANT_LOAD_CODECS);			// �ύX
    return 1;
  }

//  const FString tempDirPath = tempDir.GetPath();	// �폜
  // tempDirPath = L"M:\\1\\"; // to test low disk space
  {
    bool isCorrupt = false;
    UString errorMessage;
    HRESULT result = ExtractArchive(codecs, fullPath, tempDirPath, showProgress,
      isCorrupt, errorMessage);
    
    if (result != S_OK)
    {
      if (!assumeYes)
      {
        if (result == S_FALSE || isCorrupt)
        {
          NWindows::MyLoadString(IDS_EXTRACTION_ERROR_MESSAGE, errorMessage);
          result = E_FAIL;
        }
        if (result != E_ABORT)
        {
          if (errorMessage.IsEmpty())
            errorMessage = NError::MyFormatMessage(result);
          ::MessageBoxW(0, errorMessage, NWindows::MyLoadString(IDS_EXTRACTION_ERROR_TITLE), MB_ICONERROR);
        }
      }
      return 1;
    }
  }
	if (!isInstaller)	// �ǉ�
		return 0;		// �ǉ�

  #ifndef UNDER_CE
  CCurrentDirRestorer currentDirRestorer;
  if (!SetCurrentDir(tempDirPath))
    return 1;
  #endif
  
  HANDLE hProcess = 0;
#ifdef _SHELL_EXECUTE
  if (!executeFile.IsEmpty())
  {
    CSysString filePath = GetSystemString(executeFile);
    SHELLEXECUTEINFO execInfo;
    execInfo.cbSize = sizeof(execInfo);
    execInfo.fMask = SEE_MASK_NOCLOSEPROCESS
      #ifndef UNDER_CE
      | SEE_MASK_FLAG_DDEWAIT
      #endif
      ;
    execInfo.hwnd = NULL;
    execInfo.lpVerb = NULL;
    execInfo.lpFile = filePath;

    if (!switches.IsEmpty())
    {
      executeParameters.Add_Space_if_NotEmpty();
      executeParameters += switches;
    }

    CSysString parametersSys = GetSystemString(executeParameters);
    if (parametersSys.IsEmpty())
      execInfo.lpParameters = NULL;
    else
      execInfo.lpParameters = parametersSys;

    execInfo.lpDirectory = NULL;
    execInfo.nShow = SW_SHOWNORMAL;
    execInfo.hProcess = 0;
    /* BOOL success = */ ::ShellExecuteEx(&execInfo);
    UINT32 result = (UINT32)(UINT_PTR)execInfo.hInstApp;
    if(result <= 32)
    {
      if (!assumeYes)
        ShowErrorMessageRes(IDS_CANT_OPEN_FILE);			// �ύX
      return 1;
    }
    hProcess = execInfo.hProcess;
  }
  else
#endif
  {
    if (appLaunched.IsEmpty())
    {
      appLaunched = L"setup.exe";
      if (!NFind::DoesFileExist(us2fs(appLaunched)))
      {
        if (!assumeYes)
          ShowErrorMessageRes(IDS_CANT_FIND_SETUP);			// �ύX
        return 1;
      }
    }
    
    {
      FString s2 = tempDirPath;
      NName::NormalizeDirPathPrefix(s2);
      appLaunched.Replace(L"%%T" WSTRING_PATH_SEPARATOR, fs2us(s2));
    }
    
    UString appNameForError = appLaunched; // actually we need to rtemove parameters also

    appLaunched.Replace(L"%%T", fs2us(tempDirPath));

    if (!switches.IsEmpty())
    {
      appLaunched.Add_Space();
      appLaunched += switches;
    }
    STARTUPINFO startupInfo;
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.lpReserved = 0;
    startupInfo.lpDesktop = 0;
    startupInfo.lpTitle = 0;
    startupInfo.dwFlags = 0;
    startupInfo.cbReserved2 = 0;
    startupInfo.lpReserved2 = 0;
    
    PROCESS_INFORMATION processInformation;
    
    CSysString appLaunchedSys = GetSystemString(dirPrefix + appLaunched);
    
    BOOL createResult = CreateProcess(NULL, (LPTSTR)(LPCTSTR)appLaunchedSys,
      NULL, NULL, FALSE, 0, NULL, NULL /*tempDir.GetPath() */,
      &startupInfo, &processInformation);
    if (createResult == 0)
    {
      if (!assumeYes)
      {
        // we print name of exe file, if error message is
        // ERROR_BAD_EXE_FORMAT: "%1 is not a valid Win32 application".
        ShowErrorMessageSpec(appNameForError);
      }
      return 1;
    }
    ::CloseHandle(processInformation.hThread);
    hProcess = processInformation.hProcess;
  }
  if (hProcess != 0)
  {
    WaitForSingleObject(hProcess, INFINITE);
    ::CloseHandle(hProcess);
  }
  return 0;
}