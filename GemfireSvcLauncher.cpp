/** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **
*  Copyright (C) 2012 Avinash Dongre ( dongre.avinash@gmail.com )
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
* 
*  The above copyright notice and this permission notice shall be included in
*  all copies or substantial portions of the Software.
* 
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*  THE SOFTWARE.
** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **/

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <locale.h>
#include <windows.h>
#include <vector>
#include <iostream>
#include <io.h>

#define VERSION_TEXT "1.0"
#define QUOTESTRING   "\""

void StopGemfireServer();
DWORD create_process (char *cmdline, char *out, DWORD outsize);

std::string  gJavaHome;
std::string  gGemfireHome;
bool         isLocator               = false;
bool         isCacheServer           = false;
bool         isInstall               = false;
bool         isUninstall             = false;
bool         isQuery                 = false;
std::string  gServiceName             = "";
std::string  gServiceLauncherCommand = "";
std::string  gLogFilePath;
std::string  gWorkingDirectory;
std::string  gParams;

std::vector<std::string>     gParameters;


void Log(const char* format, ...)  {
        FILE *fp = fopen ( gLogFilePath.c_str(), "a+");
        va_list argptr;
        va_start(argptr, format);
        vfprintf(fp, format, argptr);
        va_end(argptr);
        fclose ( fp );
}

void InitializeEnvironment() {
    char *pValue;
    size_t len;

    errno_t err    = _dupenv_s( &pValue, &len, "JAVA_HOME" );     
    gJavaHome  = pValue;

    char *path_env = (char*)malloc(sizeof(char)*2048);

    _snprintf(path_env, 2048, "PATH=%s\\bin;%s", gJavaHome.c_str(), getenv("PATH"));
    _putenv ( path_env );

    err = _dupenv_s( &pValue, &len, "GEMFIRE" );
    gGemfireHome    = pValue;
}

void StartGemfireServer() {	
    Log("Entering StartGemfireServer\n");

    std::string command =   gServiceLauncherCommand + " start " + gParams;
    Log("StartGemfireServer Command = %s\n" ,command.c_str());

    system(command.c_str());

    Log("Exiting StartGemfireServer\n");
}

void StopGemfireServer() 
{
    Log("Entering StopGemfireServer\n");

    std::string command =   gServiceLauncherCommand + " stop " + gWorkingDirectory;
    Log("StopGemfireServer Command = %s\n" ,command.c_str());

    system(command.c_str());

    Log("Exiting StopGemfireServer\n");
}

/* Windows service C++ implementation
based on MSDN sample by Nigel Thompson (Microsoft Developer Network Technology Group) November 1995 */

class CNTService {
public:
    BOOL Init ();
    void Run ();

    CNTService (const char *szServiceName);
    ~CNTService ();

    BOOL IsInstalled ();
    BOOL Install ();

    BOOL StartService ();
    void SetStatus (DWORD dwState);
    BOOL Initialize ();

    static void WINAPI ServiceMain (DWORD dwArgc, LPTSTR * lpszArgv);
    static void WINAPI Handler (DWORD dwOpcode);

    char m_szServiceName[64];
    int m_iMajorVersion;
    int m_iMinorVersion;
    SERVICE_STATUS_HANDLE m_hServiceStatus;
    SERVICE_STATUS m_Status;
    BOOL m_bIsRunning;

    static CNTService *m_pThis; 

private:
    HANDLE m_hEventSource;

};

DWORD create_process (char *cmdline, char *out, DWORD outsize){
    PROCESS_INFORMATION pi; 
    STARTUPINFOA si;
    BOOL bSuccess = FALSE; 
    SECURITY_ATTRIBUTES saAttr; 
    HANDLE read_pipe = NULL;
    HANDLE write_pipe = NULL;
    DWORD size;
    DWORD total;


    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 

    if (!CreatePipe(&read_pipe, &write_pipe, &saAttr, 0)) {
        printf ("create_process: CreatePipe failed with error %d\n", GetLastError ());
        return -1;
    }

    if (!SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0)) {
        printf ("create_process: SetHandleInformation failed with error %d\n", GetLastError ());
        CloseHandle (write_pipe);
        CloseHandle (read_pipe);
        return -1;
    }

    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO); 
    si.hStdError = write_pipe;
    si.hStdOutput = write_pipe;
    si.dwFlags |= STARTF_USESTDHANDLES;

    bSuccess = CreateProcessA(NULL, 
        cmdline,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi);

    if (!bSuccess) {
        printf ("create_process: CreateProcess failed with error %d\n", GetLastError ());
        CloseHandle (read_pipe);
        CloseHandle (write_pipe);
        return -1;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle (write_pipe);

    bSuccess = TRUE;
    total = size = 0;
    while (bSuccess)
    { 
        bSuccess = ReadFile(read_pipe, out + size, outsize - size, &size, NULL);
        if(!bSuccess || size == 0) break;
        total += size;
    }
    CloseHandle (read_pipe);
    if (total > 0)
        out [total] = 0;
    return total;
}
// for debug purpose

/*
* CNTService::Run ()
* Worker loop for the service
*/
void CNTService::Run() {
    Log("Entering CNTService::Run\n");
    InitializeEnvironment();

    std::string command =   gServiceLauncherCommand ;
    command             +=  "status ";
    command             +=  gWorkingDirectory;

    Log ( "CNTService::Run Command = %s\n", command.c_str());

    char buf [1024];
    while (1) {
        if (create_process (const_cast<char*>(command.c_str()), buf, 1024) > 0) {
            if (strstr (buf, "stopped")) {
                break;
            } else {
                Sleep(60000);
            }
        } else {
            break;
        }
    }    
    StopGemfireServer();
    Log("Exiting CNTService::Run\n");
}

/**
* Overriding Windows Service Init Method. Used to initialize the service under the 
* Service Control Manager
*/
BOOL CNTService::Init () {
    Log("Entering CNTService::Init\n");
    InitializeEnvironment();
    StartGemfireServer(); 
    Log("Exiting CNTService::Init\n");
    return TRUE;
}

/* CNTService class guts
Trust me, you don't want to scroll down :( */

CNTService * CNTService::m_pThis = NULL;

CNTService::CNTService (const char *szServiceName) {
    m_pThis = this;

    // set default service name
    strncpy (m_szServiceName, szServiceName, sizeof (m_szServiceName) - 1);
    m_hEventSource = NULL;

    // set initial service status
    m_hServiceStatus                   = NULL;
    m_Status.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    m_Status.dwCurrentState            = SERVICE_STOPPED;
    m_Status.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    m_Status.dwWin32ExitCode           = 0;
    m_Status.dwServiceSpecificExitCode = 0;
    m_Status.dwCheckPoint              = 0;
    m_Status.dwWaitHint                = 0;
}

CNTService::~CNTService () {
    if (m_hEventSource)
        ::DeregisterEventSource (m_hEventSource);
}

/**
* CNTService::IsInstalled ()
* Checks if the service is installed or not
*/
BOOL CNTService::IsInstalled () {
    BOOL bResult = FALSE;
    SC_HANDLE hSCM =::OpenSCManagerA (NULL,NULL,SC_MANAGER_ALL_ACCESS);  
    if (hSCM) {
        SC_HANDLE hService =::OpenServiceA (hSCM, m_szServiceName, SERVICE_QUERY_CONFIG);
        if (hService) {
            bResult = TRUE;
            ::CloseServiceHandle (hService);
        }
        ::CloseServiceHandle (hSCM);
    }
    return bResult;
}
/**
* CNTService::Install ()
*/
BOOL CNTService::Install () {
    SC_HANDLE hSCM =::OpenSCManagerA (  NULL, 
                                        NULL, 
                                        SC_MANAGER_ALL_ACCESS); 
    if (!hSCM)
        return FALSE;
    char szFilePath[FILENAME_MAX];
    ::GetModuleFileNameA (NULL, szFilePath, sizeof (szFilePath));

    std::string CommandString = "";
    CommandString += szFilePath;
    CommandString += " ";
    CommandString += "--servicemode ";
    CommandString += m_szServiceName;
    CommandString += " ";

    for ( std::vector<std::string>::iterator iter = gParameters.begin();
        iter != gParameters.end(); ++iter) {
            CommandString += *iter;
            CommandString += " ";
    }    

    std::string serviceDisplayName = "Gemfire ";
    if ( isLocator ) 
        serviceDisplayName += "Locator ";
    else 
        serviceDisplayName += "CacheServer ";

    serviceDisplayName += "(" + gServiceName + ")";
    SC_HANDLE hService =::CreateServiceA (hSCM,
        m_szServiceName,
        serviceDisplayName.c_str(),
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        CommandString.c_str(),
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);
    if (!hService) {
        ::CloseServiceHandle (hSCM);
        return FALSE;
    }

    char szKey[256];
    HKEY hKey = NULL;

    strcpy (szKey, "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");
    strcat (szKey, m_szServiceName);
    if (::RegCreateKeyA (HKEY_LOCAL_MACHINE, (LPCSTR) szKey, &hKey) != ERROR_SUCCESS) {
        ::CloseServiceHandle (hService);
        ::CloseServiceHandle (hSCM);
        return FALSE;
    }
    ::RegSetValueExA (hKey, "EventMessageFile", 0, REG_EXPAND_SZ, (CONST BYTE *) szFilePath, (int) strlen (szFilePath) + 1);
    DWORD dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
    ::RegSetValueExA (hKey, "TypesSupported", 0, REG_DWORD, (CONST BYTE *) & dwData, sizeof (DWORD));
    ::RegCloseKey (hKey);
    ::CloseServiceHandle (hService);
    ::CloseServiceHandle (hSCM);


    SC_HANDLE schSCManager;
    SC_HANDLE schService;
    SERVICE_DESCRIPTION sd;
    LPTSTR szDesc = TEXT("");

    if ( isLocator ) 
        szDesc = TEXT("Gemfire Locator Service");
    else 
        szDesc = TEXT("Gemfire CacheServer Service");

    schSCManager = ::OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS); 
    if ( schSCManager )  {    
        schService = ::OpenServiceA( schSCManager, m_szServiceName, SERVICE_CHANGE_CONFIG);  
        if ( schService ) {
            sd.lpDescription = szDesc;
            ::ChangeServiceConfig2( schService, SERVICE_CONFIG_DESCRIPTION,&sd);
            ::CloseServiceHandle(schService);
        }
        ::CloseServiceHandle(schSCManager);
    }
    return TRUE;
}

/**
* CNTService::StartService ()
* This method is called when the user clicks start in the Service Control Manager
*/
BOOL CNTService::StartService () {
    Log( "Entering CNTService::StartService\n");
    SERVICE_TABLE_ENTRYA st[] = {
        {m_szServiceName, (LPSERVICE_MAIN_FUNCTIONA) ServiceMain}
        ,
        {NULL, NULL}
    };
    Log( "Exiting CNTService::StartService\n");
    return::StartServiceCtrlDispatcherA (st);
}

/** 
* CNTService::SetStatus 
* Used to set the status of the service, whether it is running, started or stopped
*/
void CNTService::SetStatus (DWORD dwState) {
    m_Status.dwCurrentState = dwState;
    ::SetServiceStatus (m_hServiceStatus, &m_Status);
}
/**
* CNTService::Initialize ()
* Initializing the service
*/
BOOL CNTService::Initialize() {
    Log("Entering CNTService::Initialize\n");
    SetStatus (SERVICE_START_PENDING);

    BOOL bResult = Init();

    m_Status.dwWin32ExitCode = GetLastError ();
    m_Status.dwCheckPoint = 0;
    m_Status.dwWaitHint = 0;
    if (!bResult) {
        SetStatus (SERVICE_STOPPED);
        return FALSE;
    }
    SetStatus (SERVICE_RUNNING);
    Log("Exiting CNTService::Initialize\n");
    return TRUE;
}
/*
* CNTService::ServiceMain
* Function that provides the entry point for the actual service code. 
*/
void CNTService::ServiceMain (DWORD dwArgc, LPTSTR * lpszArgv) {
    Log("Entering CNTService::ServiceMain\n");
    CNTService *pService = m_pThis;

    dwArgc;
    lpszArgv;

    pService->m_Status.dwCurrentState = SERVICE_START_PENDING;
    pService->m_hServiceStatus = RegisterServiceCtrlHandlerA ((LPCSTR) pService->m_szServiceName, Handler);
    if (pService->m_hServiceStatus != NULL) {
        if (pService->Initialize()) {
            pService->m_bIsRunning             = TRUE;
            pService->m_Status.dwWin32ExitCode = 0;
            pService->m_Status.dwCheckPoint    = 0;
            pService->m_Status.dwWaitHint      = 0;
            pService->Run();
        }
        pService->SetStatus (SERVICE_STOPPED);
    }
    Log("Exiting CNTService::ServiceMain\n");
}
/*
* CNTService::Handler --> Service Handler Function
* Function that processes command messages from the service manager. 
*/
void CNTService::Handler (DWORD dwOpcode) {
    Log("Entering CNTService::Handler\n");
    CNTService *pService = m_pThis;
    BOOL err;

    if ((dwOpcode == SERVICE_CONTROL_STOP) || (dwOpcode == SERVICE_CONTROL_SHUTDOWN)) {
        // Stop the web server when you recieve the stop command
        StopGemfireServer();

        SC_HANDLE hSCM =::OpenSCManagerA (NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (!hSCM)
            return;

        SC_HANDLE hService =::OpenServiceA (hSCM, (LPCSTR) pService->m_szServiceName, SC_MANAGER_ALL_ACCESS);
        if (hService){
            pService->SetStatus (SERVICE_STOP_PENDING);
            pService->m_bIsRunning = FALSE;
            err = ControlService (hService, SERVICE_CONTROL_STOP, &pService->m_Status);
            pService->SetStatus (SERVICE_CONTROL_STOP);
            CloseServiceHandle (hService);
            CloseServiceHandle (hSCM);
        }
    }
    ::SetServiceStatus (pService->m_hServiceStatus, &pService->m_Status);
    Log("Exiting CNTService::Handler\n");
}



void Usage () {
    std::cerr << " Usage : " << std::endl;
    std::cerr << std::endl << std::endl;

    std::cerr << " ****  Starting and Stopping Service  ****  " << std::endl;
    std::cerr << " Use Windows Service Control Manager or net start/stop commands " << std::endl;
    std::cerr << std::endl << std::endl;

    std::cerr << " ****  Creating a new service  ****  " << std::endl;
    std::cerr << " GemfireSvcLauncher --install --type [locator | cacheserver ] --name <Name Of the Service> --params <All the required parameters required to start cacheserver/locator process>" << std::endl;
    std::cerr << " Note: No space is allowed in service name. " << std::endl;
    std::cerr << " Example: " << std::endl << std::endl;
    std::cerr << " GemfireSvcLauncher --install --type cacheserver --name <MyGemFireService> --params -J-Xmx1024m -J-Xms128m cache-xml-file=c:\\cacheserver.xml -dir=E:\\gfecs  mcast-port=0  log-level=config " << std::endl;

    exit(1);
}

void ParseCommandLineArguments(const int argc, char *argv[]) {
    for (int i = 1; i < argc && argv[i]; i++) {
        if (!strcmp (argv[i], "--install")) {
            isInstall = true;
            if (!strcmp (argv[++i], "--type")) {
                if (!_strnicmp ("locator", argv[++i], 7)){
                    isLocator = true;
                } else if (!_strnicmp ("cacheserver", argv[i], 11)) {
                    isCacheServer = true;
                } else {
                    Usage();
                }
            } else {
                Usage();
            }
            if (!strcmp (argv[++i], "--name")) {
                if (argv[++i]) {
                    gServiceName = argv[i];
                } else {
                    Usage();
                }
            } else {
                Usage();
            }
            if (!strcmp (argv[++i], "--params")) {
                i++;
                while ( i < argc && argv[i] ) {
                    if (strstr( argv[i], "-dir=" ) ){
                        gWorkingDirectory = argv[i];                        
                    }
                    gParameters.push_back(argv[i]);
                    i++;
                }
            }else {
                Usage();
            }            
        } else {
            Usage();
        }    
    }
}

int main (int argc, char *argv[]) {    
    InitializeEnvironment();

    if ( argv[1] && (strcmp( argv[1], "--servicemode")) == 0 ) {
        CNTService ntService(argv[2]);        
        int idx = 3;

        while ( argv[idx] ) {
            if (strstr( argv[idx], "-dir=" ) ) {
                gWorkingDirectory = argv[idx];                        
            }
            gParams += argv[idx];
            gParams += " ";
            idx++;
        }

        gServiceLauncherCommand = "\"" + gGemfireHome + "\\bin\\cacheserver.bat" + "\"" + " ";

        std::string::size_type foundPos = gWorkingDirectory.find_first_of("=", 0);
        gLogFilePath = gWorkingDirectory.substr(foundPos + 1, gWorkingDirectory.size() );       
        gLogFilePath += "\\GemfireSvcLauncher.log";

        ntService.StartService();
    }
    else if ( argc > 2 ) {
        // foreground mode
        ParseCommandLineArguments(argc, argv);    
        gLogFilePath = gWorkingDirectory + "\\GemfireSvcLauncher.log";
        setlocale (LC_ALL, "");

        CNTService ntService(gServiceName.c_str());

        if ( isInstall ) {        
            if (ntService.IsInstalled ()) {
                std::cout << gServiceName << " - Service is already installed " << std::endl;
                exit ( 0 );
            }
            if (ntService.Install ()) {
                std::cout << gServiceName << " - Service Installed " << std::endl;
            }
        }
    } else {
        Usage();
    }

    return 0;

}