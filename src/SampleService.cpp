/****************************** Module Header ******************************\
* Module Name:  SampleService.cpp
* Project:      CppWindowsService
* Copyright (c) Microsoft Corporation.
*
* Provides a sample service class that derives from the service base class -
* CServiceBase. The sample service logs the service start and stop
* information to the Application event log, and shows how to run the main
* function of the service in a thread pool worker thread.
*
* This source is subject to the Microsoft Public License.
* See http://www.microsoft.com/en-us/openness/resources/licenses.aspx#MPL.
* All other rights reserved.
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
* EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include <iostream>
#include "SampleService.h"
#include "ThreadPool.h"


CSampleService::CSampleService(Descriptor *d,
                               PCTSTR pszServiceName,
                               BOOL fCanStop,
                               BOOL fCanShutdown,
                               BOOL fCanPauseContinue)
    : CServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue), d(d)
{
    m_fStopping = FALSE;
    m_testMode = FALSE;
    m_fStarted = FALSE;
    dwLastError = 0;
    
    // Create a manual-reset event that is not signaled at first to indicate
    // the stopped signal of the service.
    m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_hStoppedEvent == NULL)
    {
        throw GetLastError();
    }
    // Create a manual-reset event that is not signaled at first to indicate
    // the stopped signal of the service.
    m_hStartedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_hStartedEvent == NULL)
    {
        throw GetLastError();
    }
}


CSampleService::~CSampleService(void)
{
    if (m_hStartedEvent)
    {
        CloseHandle(m_hStartedEvent);
        m_hStartedEvent = NULL;
    }
    if (m_hStoppedEvent)
    {
        CloseHandle(m_hStoppedEvent);
        m_hStoppedEvent = NULL;
    }
}

void CSampleService::Test()
{
    m_testMode = TRUE;
    Start(0, NULL);
    WaitForSingleObject(m_hStoppedEvent, INFINITE);
}

//
//   FUNCTION: CSampleService::OnStart(DWORD, LPTSTR *)
//
//   PURPOSE: The function is executed when a Start command is sent to the
//   service by the SCM or when the operating system starts (for a service
//   that starts automatically). It specifies actions to take when the
//   service starts. In this code sample, OnStart logs a service-start
//   message to the Application log, and queues the main service function for
//   execution in a thread pool worker thread.
//
//   PARAMETERS:
//   * dwArgc   - number of command line arguments
//   * lpszArgv - array of command line arguments
//
//   NOTE: A service application is designed to be long running. Therefore,
//   it usually polls or monitors something in the system. The monitoring is
//   set up in the OnStart method. However, OnStart does not actually do the
//   monitoring. The OnStart method must return to the operating system after
//   the service's operation has begun. It must not loop forever or block. To
//   set up a simple monitoring mechanism, one general solution is to create
//   a timer in OnStart. The timer would then raise events in your code
//   periodically, at which time your service could do its monitoring. The
//   other solution is to spawn a new thread to perform the main service
//   functions, which is demonstrated in this code sample.
//
void CSampleService::OnStart(DWORD dwArgc, LPTSTR *lpszArgv)
{
    TCHAR buff[1024];
    const HANDLE events[2] =
    {
        m_hStartedEvent, m_hStoppedEvent
    };


    // Signal the stopped event.
    ResetEvent(m_hStoppedEvent);
    ResetEvent(m_hStartedEvent);

    // Queue the main service function for execution in a worker thread.
    CThreadPool::QueueUserWorkItem(&CSampleService::ServiceWorkerThread, this);
    DWORD timeout = 12000;
    if (WaitForMultipleObjects(2, events, FALSE, timeout) != WAIT_OBJECT_0)
    {
        m_fStopping = TRUE;
        _stprintf(buff, TEXT("Wait to start process failed w/err 0x%08lx"),
                 dwLastError);
        WriteEventLogEntry(buff, EVENTLOG_WARNING_TYPE);
        throw dwLastError;
    }
    else
    {
        // Log a service start message to the Application log.
        WriteEventLogEntry(TEXT("Service started successfully"),
                           EVENTLOG_INFORMATION_TYPE);
    }
}

String CSampleService::GetEnvString()
{
    TCHAR *env = GetEnvironmentStrings();
    if (!env)
    {
        return TEXT("");
    }
    const TCHAR *var = env;
    size_t totallen = 0;
    size_t len;
    while ((len = _tcslen(var)) > 0)
    {
        totallen += len + 1;
        var += len + 1;
    }
    String result(env, totallen);
    FreeEnvironmentStrings(env);
    return result;
}

//
//   FUNCTION: CSampleService::ServiceWorkerThread(void)
//
//   PURPOSE: The method performs the main function of the service. It runs
//   on a thread pool worker thread.
//
void CSampleService::ServiceWorkerThread(void)
{
    int repeatCount = 0, maxRepeatCount = 3;
    int repeatDelay = 3000;
    DWORD dwFlags=0;
    LPCTSTR lpApplicationName = NULL;
    LPTSTR lpCommandLine = NULL;
    LPCTSTR lpCurrentDirectory = NULL;
    TCHAR buff[1024];
    String cmdLine = d->startarguments();
    if (d->executable.size() > 0)
    {
        cmdLine = d->quoteParam(d->executable) + TEXT(" ") + cmdLine;
    }
    String currDir = d->currentDirectory();
    if (cmdLine.size() > 0)
    {
        lpCommandLine = (LPTSTR)cmdLine.c_str();
    }
    lpCurrentDirectory = currDir.c_str();
#ifdef UNICODE
    dwFlags = CREATE_UNICODE_ENVIRONMENT;
#endif
    std::vector<std::pair<String, String> >::iterator it;
    for (it = d->env.begin(); it != d->env.end(); it++)
    {
        if (!SetEnvironmentVariable(it->first.c_str(), it->second.c_str()))
        {
            dwLastError = GetLastError();
            _stprintf(buff, TEXT("Set Environment Variable failed w/err 0x%08lx"),
                     dwLastError);
            WriteEventLogEntry(buff, EVENTLOG_ERROR_TYPE);
            // Signal the stopped event.
            SetEvent(m_hStoppedEvent);
            return;
        }
    }
    String env = GetEnvString();
    LPCTSTR lpEnvironment = NULL;
    if (env.size() > 0)
    {
        lpEnvironment = env.c_str();
    }
    while (repeatCount < maxRepeatCount && !m_fStopping)
    {
        repeatCount++;
        ZeroMemory( &si, sizeof(si) );
        si.cb = sizeof(si);
        ZeroMemory( &pi, sizeof(pi) );
        if (!CreateProcess(lpApplicationName, lpCommandLine, NULL, NULL, FALSE,
                           dwFlags, (LPVOID)lpEnvironment, lpCurrentDirectory, &si, &pi))
        {
            dwLastError = GetLastError();
            _stprintf(buff,
                     TEXT("Start Create Process failed w/err 0x%08lx"),
                     dwLastError);
            WriteEventLogEntry(buff, EVENTLOG_WARNING_TYPE);
            break;
        }
        else
        {
            dwLastError = WaitForProcessToExit(&pi, FALSE);
            if (m_fStopping)
            {
                break;
            }
            if (m_fStarted)
            {
                repeatCount = 0;
            }
            _stprintf(buff,
                     TEXT("Service stopped unexpectedly w/err 0x%08lx, waiting to restart %d/%d"),
                     dwLastError, repeatCount, maxRepeatCount);
            WriteEventLogEntry(buff, EVENTLOG_WARNING_TYPE);
        }
        if (repeatCount < maxRepeatCount && !m_fStopping)
        {
            SetServiceStatus(SERVICE_START_PENDING, dwLastError);
            Sleep(repeatDelay);
        }
    }
    if (!m_fStopping && m_fStarted)
    {
        OnUnexpectedlyStopped(dwLastError);
    }
    // Signal the stopped event.
    SetEvent(m_hStoppedEvent);
}

void CSampleService::OnUnexpectedlyStopped(DWORD errorCode)
{
    TCHAR buff[1024];

    _stprintf(buff, TEXT("Service stopped unexpectedly w/err 0x%08lx"), errorCode);
    WriteEventLogEntry(buff, EVENTLOG_ERROR_TYPE);
    SetServiceStatus(SERVICE_STOPPED, errorCode);
}

//
//   FUNCTION: CSampleService::OnStop(void)
//
//   PURPOSE: The function is executed when a Stop command is sent to the
//   service by SCM. It specifies actions to take when a service stops
//   running. In this code sample, OnStop logs a service-stop message to the
//   Application log, and waits for the finish of the main service function.
//
//   COMMENTS:
//   Be sure to periodically call ReportServiceStatus() with
//   SERVICE_STOP_PENDING if the procedure is going to take long time.
//
void CSampleService::OnStop()
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD dwFlags=0;
    TCHAR buff[1024];

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );
    LPCTSTR lpApplicationName = NULL;
    LPTSTR lpCommandLine = NULL;
    LPCTSTR lpCurrentDirectory = NULL;
    String cmdLine = d->stopargumentsCmd();
    if (d->stopexecutable.size() > 0)
    {
        cmdLine = d->quoteParam(d->stopexecutable) + TEXT(" ") + cmdLine;
    }
    String currDir = d->currentDirectory();
    if (cmdLine.size() > 0)
    {
        lpCommandLine = (LPTSTR)cmdLine.c_str();
    }
    lpCurrentDirectory = currDir.c_str();
#ifdef UNICODE
    dwFlags = CREATE_UNICODE_ENVIRONMENT;
#endif
    String env = GetEnvString();
    LPCTSTR lpEnvironment = NULL;
    if (env.size() > 0)
    {
        lpEnvironment = env.c_str();
    }
    m_fStopping = TRUE;
    if(!CreateProcess(lpApplicationName, lpCommandLine, NULL, NULL, FALSE,
                      dwFlags, (LPVOID)lpEnvironment, lpCurrentDirectory, &si, &pi))
    {
        dwLastError = GetLastError();
        _stprintf(buff, TEXT("Stop Create Process failed w/err 0x%08lx"), dwLastError);
        WriteEventLogEntry(buff, EVENTLOG_WARNING_TYPE);
        throw dwLastError;
    }
    WaitForProcessToExit(&pi, TRUE);
    // Indicate that the service is stopping and wait for the finish of the
    // main service function (ServiceWorkerThread).
    DWORD timeout = 2000;
    if (WaitForSingleObject(m_hStoppedEvent, timeout) != WAIT_OBJECT_0)
    {
        _stprintf(buff, TEXT("Wait to start process stop failed w/err 0x%08lx"),
                 dwLastError);
        WriteEventLogEntry(buff, EVENTLOG_WARNING_TYPE);
        throw dwLastError;
    }
    else
    {
        // Log a service stop message to the Application log.
        WriteEventLogEntry(TEXT("Service stopped successfully"),
                           EVENTLOG_INFORMATION_TYPE);
    }
    m_fStarted = FALSE;
}

DWORD CSampleService::WaitForProcessToExit(PROCESS_INFORMATION *pi,
        BOOL stopping)
{
    DWORD exitCode = 9999;
    DWORD timeout = 2000;

    if (stopping)
    {
        // Successfully created the process.  Wait for it to finish.
        while (WaitForSingleObject( pi->hProcess, timeout ) == WAIT_TIMEOUT)
        {
            SetServiceStatus(SERVICE_STOP_PENDING);
        }
    }
    else
    {
        // Successfully created the process.  Wait for it to finish.
        if (WaitForSingleObject( pi->hProcess, timeout / 2 ) == WAIT_TIMEOUT)
        {
            BOOL oldStarted = m_fStarted;
            m_fStarted = TRUE;
            if (oldStarted)
            {
                SetServiceStatus(SERVICE_RUNNING);
            }
            SetEvent(m_hStartedEvent);
        }
        exitCode = WaitForSingleObject(pi->hProcess, INFINITE);
    }

    // Get the exit code.
    BOOL result = GetExitCodeProcess(pi->hProcess, &exitCode);

    // Close the handles.
    CloseHandle( pi->hProcess );
    CloseHandle( pi->hThread );

    if (!result)
    {
        // Could not get exit code.
        WriteEventLogEntry(TEXT("Executed command but couldn't get exit code."),
                           EVENTLOG_INFORMATION_TYPE);
    }
    // We succeeded.
    return exitCode;
}

void CSampleService::SetServiceStatus(DWORD dwCurrentState,
                                      DWORD dwWin32ExitCode,
                                      DWORD dwWaitHint)
{
    if (!m_testMode)
    {
        CServiceBase::SetServiceStatus(dwCurrentState, dwWin32ExitCode, dwWaitHint);
        return;
    }
    String status;
    switch(dwCurrentState)
	{
	case SERVICE_STOP_PENDING:
        status = TEXT("Service Stop Pending");
		break;
	case SERVICE_STOPPED:
        status = TEXT("Service Stopped");
		break;
	case SERVICE_START_PENDING:
        status = TEXT("Service Start Pending");
		break;
	case SERVICE_RUNNING:
        status = TEXT("Service Running");
		break;
	case SERVICE_CONTINUE_PENDING:
        status = TEXT("Service Continue Pending");
		break;
	case SERVICE_PAUSE_PENDING:
        status = TEXT("Service Pause Pending");
		break;
	case SERVICE_PAUSED:
        status = TEXT("Service Paused");
		break;
	default:
        status = TEXT("Service Unknow");
		break;
	}
    Cout << status;
    if (dwWin32ExitCode != 0)
    {
        _tprintf(TEXT(" - Error Code: 0x%08lx"), dwWin32ExitCode);
    }
    Cout << TEXT("\n");
}

void CSampleService::WriteEventLogEntry(PCTSTR pszMessage, WORD wType)
{
    if (!m_testMode)
    {
        CServiceBase::WriteEventLogEntry(pszMessage, wType);
        return;
    }
    String type;
    switch(wType)
	{
	case EVENTLOG_SUCCESS:
        type = TEXT("Event Sucess");
		break;
	case EVENTLOG_ERROR_TYPE:
        type = TEXT("Event Error");
		break;
	case EVENTLOG_WARNING_TYPE:
        type = TEXT("Event Warning");
		break;
	case EVENTLOG_INFORMATION_TYPE:
        type = TEXT("Event Information");
		break;
	case EVENTLOG_AUDIT_SUCCESS:
        type = TEXT("Event Audit success");
		break;
	case EVENTLOG_AUDIT_FAILURE:
        type = TEXT("Event Audit failure");
		break;
	default:
        type = TEXT("Event Unknow");
		break;
	}
    Cout << type << TEXT(": ") << pszMessage << TEXT("\n");
}
