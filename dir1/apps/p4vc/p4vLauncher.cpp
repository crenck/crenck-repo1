#include "p4vLauncher.h"

#include <QScopedArrayPointer>
#include <QTimer>

#include <iostream>

#if defined(Q_OS_WIN)
#include <windows.h>
#define WIN32_LEAN_AND_MEAN
#include <psapi.h>


static bool launchApp(wchar_t* app, wchar_t* args)
{
    PROCESS_INFORMATION processInformation;
    ::memset(&processInformation, 0, sizeof(processInformation));

    STARTUPINFOW startupInfo;
    ::memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    bool result = ::CreateProcessW(app, args, NULL, NULL, false, 
        DETACHED_PROCESS | NORMAL_PRIORITY_CLASS, NULL, NULL, &startupInfo, &processInformation);

    ::CloseHandle(processInformation.hProcess);
    ::CloseHandle(processInformation.hThread);

    return result;
}

static bool launchFromLocation(const std::string& location)
{
    int len = ::MultiByteToWideChar(CP_UTF8, 0, location.c_str(), -1, NULL, 0);
    if (len <= 0)
        return false;

    QScopedArrayPointer<wchar_t> filename(new wchar_t[len]);
    if (!filename)
        return false;

    ::MultiByteToWideChar(CP_UTF8, 0, location.c_str(), -1, filename.data(), len);

    wchar_t tempCmdLine[MAX_PATH] = L"";
    ::wcsncat_s(tempCmdLine, MAX_PATH, filename.data(), _TRUNCATE);
    ::wcsncat_s(tempCmdLine, MAX_PATH, L" -faceless", _TRUNCATE);

    return launchApp(filename.data(), tempCmdLine);
}

static bool launchFromEnvPath()
{
    wchar_t tempCmdLine[MAX_PATH] = L"p4v.exe -faceless";
    return launchApp(L"p4v.exe", tempCmdLine);
}

static bool launchFromProcessLocation()
{
    wchar_t filename[_MAX_PATH]; 
    HANDLE processHandle = ::OpenProcess(PROCESS_ALL_ACCESS, false, ::GetCurrentProcessId()); 

    if (processHandle == NULL)
        return false;
    if (::GetModuleFileNameExW(processHandle, NULL, filename, sizeof(filename)) == 0)
        return false;
    ::CloseHandle(processHandle);

    wchar_t drive[_MAX_DRIVE];
    wchar_t dir[_MAX_PATH];

    if (_wsplitpath_s(filename, drive, _MAX_DRIVE, dir, _MAX_PATH, NULL, 0, NULL, 0) != 0)
        return false;

    _wmakepath_s(filename, MAX_PATH, drive, dir, L"p4v", L".exe");
    
    wchar_t tempCmdLine[MAX_PATH] = L"\"";
    ::wcsncat_s(tempCmdLine, MAX_PATH, filename, _TRUNCATE);
    ::wcsncat_s(tempCmdLine, MAX_PATH, L"\" -faceless", _TRUNCATE);

    return launchApp(filename, tempCmdLine);
}

#elif defined(Q_OS_OSX)
#include <mach-o/dyld.h>

static bool launchFromLocation(const std::string& location, 
                                    bool macOS105Mode)
{
    std::string command;
    
    // open does not support --args on MacOSX105
    // This is an alternative less elegant method of bootstrapping p4v 
    //        as a service
    if (macOS105Mode)
        command = "'" + location + "/p4v.app/Contents/MacOS/p4v' -faceless &";
    else
        command = "open '" + location + "/p4v.app' -n --args -faceless > /dev/null 2>&1";

    return (system(command.c_str()) == 0);
}

static bool launchFromAppPath(bool macOS105Mode)
{
    return launchFromLocation("/Applications", macOS105Mode);
}

static bool launchFromProcessLocation(bool macOS105Mode)
{
    char filename[PATH_MAX];
    uint32_t size = sizeof(filename);
    if (_NSGetExecutablePath(filename, &size) != 0)
        return false;
        
    char* last = filename + strlen(filename);
    while (last > filename) 
        if (*(--last) == '/') 
            break;
    *last = 0;

    return launchFromLocation(filename, macOS105Mode);
}

#elif defined(Q_OS_LINUX)

static bool launchFromLocation(const std::string& location)
{
    std::string command = "'" + location + "/p4v' -faceless &";
    return (system(command.c_str()) == 0);
}


static bool launchFromEnvPath()
{
    return (system("p4v -faceless &") != -1);
}


static bool launchFromProcessLocation()
{
    char filename[PATH_MAX + 1];
    ssize_t len = ::readlink("/proc/self/exe", filename, sizeof(filename)-1);
    if (len == -1)
        return false;

    filename[len] = '\0';
        
    char* last = filename + len;
    while (last > filename) 
        if (*(--last) == '/') 
            break;
    *last = 0;

    return launchFromLocation(filename);
}

#endif

namespace p4vc
{

P4VLauncher::P4VLauncher(int argc, char *argv[], QSharedPointer<P4VCMessage> message)
    : QCoreApplication(argc, argv), mMacOS105Mode( false ), mPort(0), mClient(message)
{
    // Use default port/host for now, parse args for values later
    QTimer::singleShot(0, this, static_cast<void (P4VLauncher::*)(void)>(&P4VLauncher::connect));

    QObject::connect(&mClient, static_cast<void (VCClient::*)(bool)>(&VCClient::connectionRefused),
        this, &P4VLauncher::retryConnection);
    QObject::connect(&mClient, static_cast<void (VCClient::*)(void)>(&VCClient::timedOut),
        this, &P4VLauncher::connectionTimeout);
    QObject::connect(&mClient, static_cast<void (VCClient::*)(int)>(&VCClient::clientQuit),
        this, &P4VLauncher::clientQuit);
}

P4VLauncher::~P4VLauncher()
{
}

void P4VLauncher::connect()
{
    connect("", 7999);
}

void P4VLauncher::connect(const std::string& utf8P4vPath, unsigned short port)
{
    mPort = port;
    mLaunchPath = utf8P4vPath;
    // Try to connect to existing p4v service first
    mClient.connectToHost("127.0.0.1", mPort);
}

void P4VLauncher::clientQuit(int exitCode)
{
    exit(exitCode);
}

void P4VLauncher::retryConnection(bool macOs105Mode)
{
    // Sets mac Os 10.5 mode
    mMacOS105Mode = macOs105Mode;

    // Initial connection failed, p4v isn't likely launched
    // Launch p4v -faceless service
    launch(mLaunchPath);
    // Start retry connection loop
    mClient.connectToHost("127.0.0.1", mPort);
}

void P4VLauncher::connectionTimeout()
{
    std::cout << "Connection to P4V server timed out...\n";
    exit(-1);
}

bool P4VLauncher::autoLaunch()
{
    bool result(false);
    // On windows and linux try to find it in the process location
    // and if it is not there in environment path
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    result = launchFromProcessLocation();
    if (!result)
        result = launchFromEnvPath();

    // On macosx try to find it in the Application folder
    // if it is not there run it from p4vc location if it is there
#elif defined(Q_OS_OSX)

     
#ifdef QT_DEBUG
    // In debug the behavior is switched - we would prefer to execute just
    // build p4v to test with instead of preinstalled p4v version 
    result = launchFromProcessLocation(mMacOS105Mode);
    if (!result)
       result = launchFromAppPath(mMacOS105Mode);
#else
    result = launchFromAppPath(mMacOS105Mode);
    if (!result)
       result = launchFromProcessLocation(mMacOS105Mode);
#endif

#else
    // Unimplimented
#endif
    return result;
}

bool P4VLauncher::launch(const std::string& utf8P4vPath)
{
    bool result(false);

    if (utf8P4vPath.empty())
    {
        result = autoLaunch();
    }
    else
    {
#if defined(Q_OS_OSX)
        result = launchFromLocation(utf8P4vPath, mMacOS105Mode);
#else
        result = launchFromLocation(utf8P4vPath);
#endif
    }
    return result;
}

}
