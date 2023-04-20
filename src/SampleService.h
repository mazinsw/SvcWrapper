/****************************** Module Header ******************************\
* Module Name:  SampleService.h
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

#pragma once

#include "ServiceBase.h"
#include "Descriptor.h"


class CSampleService : public CServiceBase
{
public:
    CSampleService(
        Descriptor* d,
        PCTSTR pszServiceName, 
        BOOL fCanStop = TRUE, 
        BOOL fCanShutdown = TRUE, 
        BOOL fCanPauseContinue = FALSE);
    virtual ~CSampleService(void);

    // Start the service.
    void Test();

protected:

    virtual void OnStart(DWORD dwArgc, PTSTR *pszArgv);
    virtual void OnStop();
    virtual void OnUnexpectedlyStopped(DWORD errorCode);

    void ServiceWorkerThread(void);

    // Set the service status and report the status to the SCM.
    virtual void SetServiceStatus(DWORD dwCurrentState, 
        DWORD dwWin32ExitCode = NO_ERROR, 
        DWORD dwWaitHint = 0);

    // Log a message to the Application event log.
    virtual void WriteEventLogEntry(PCTSTR pszMessage, WORD wType);

private:
    /**
     * pi: Process information
     * 
     * Return exit code
     */
    DWORD WaitForProcessToExit(PROCESS_INFORMATION *pi, BOOL stopping);
    
    String GetEnvString();
    
    DWORD dwLastError;

    HANDLE m_hStoppedEvent;
    HANDLE m_hStartedEvent;
    Descriptor* d;
    
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    
    BOOL m_fStarted;
    BOOL m_fStopping;
    BOOL m_testMode;
};
