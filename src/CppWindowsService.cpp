/****************************** Module Header ******************************\
* Module Name:  CppWindowsService.cpp
* Project:      CppWindowsService
* Copyright (c) Microsoft Corporation.
*
* The file defines the entry point of the application. According to the
* arguments in the command line, the function installs or uninstalls or
* starts the service by calling into different routines.
*
* This source is subject to the Microsoft Public License.
* See http://www.microsoft.com/en-us/openness/resources/licenses.aspx#MPL.
* All other rights reserved.
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
* EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include <stdio.h>
#include <windows.h>
#include <fstream>
#include <sys/stat.h>
#include <codecvt>
#include <iostream>
#include <vector>
#include "ServiceInstaller.h"
#include "ServiceBase.h"
#include "SampleService.h"
#include "rapidxml.hpp"
#include "strings.h"
#include "Descriptor.h"
#include "utils.h"

std::string WideCharToACP(const std::wstring & str)
{
   if (str.empty())
      return std::string();

   size_t charsNeeded = ::WideCharToMultiByte(CP_ACP, 0, 
      str.data(), (int)str.size(), NULL, 0, NULL, NULL);
   if (charsNeeded == 0)
      throw std::runtime_error("Failed to calculate WideChar string to Windows-1252");

   std::vector<char> buffer(charsNeeded);
   int charsConverted = ::WideCharToMultiByte(CP_ACP, 0, 
      str.data(), (int)str.size(), &buffer[0], buffer.size(), NULL, NULL);
   if (charsConverted == 0)
      throw std::runtime_error("Failed converting WideChar string to Windows-1252");
   return std::string(&buffer[0], charsConverted);
}

std::wstring ACPToWideChar(const std::string & str)
{
   if (str.empty())
      return std::wstring();

   size_t charsNeeded = ::MultiByteToWideChar(CP_ACP, 0, 
      str.data(), (int)str.size(), NULL, 0);
   if (charsNeeded == 0)
      throw std::runtime_error("Failed to calculate Windows-1252 string to WideChar");

   std::vector<wchar_t> buffer(charsNeeded);
   int charsConverted = ::MultiByteToWideChar(CP_ACP, 0, 
      str.data(), (int)str.size(), &buffer[0], buffer.size());
   if (charsConverted == 0)
      throw std::runtime_error("Failed converting Windows-1252 string to WideChar");
   return std::wstring(&buffer[0], charsConverted);
}

std::wstring UTF8ToWideChar(const std::string & str)
{
   if (str.empty())
      return std::wstring();

   size_t charsNeeded = ::MultiByteToWideChar(CP_UTF8, 0, 
      str.data(), (int)str.size(), NULL, 0);
   if (charsNeeded == 0)
      throw std::runtime_error("Failed to calculate UTF-8 string to WideChar");

   std::vector<wchar_t> buffer(charsNeeded);
   int charsConverted = ::MultiByteToWideChar(CP_UTF8, 0, 
      str.data(), (int)str.size(), &buffer[0], buffer.size());
   if (charsConverted == 0)
      throw std::runtime_error("Failed converting UTF-8 string to WideChar");
   return std::wstring(&buffer[0], charsConverted);
}

String LoadUtf8FileToString(const String& filename)
{
    std::string buffer;            // stores file contents
#ifdef _UNICODE
    std::wstring filenameW = filename;
#else
    std::wstring filenameW = ACPToWideChar(filename);
#endif
    FILE *fp = _wfopen(filenameW.c_str(), L"r");

    // Failed to open file
    if (fp == NULL)
    {
        // ...handle some error...
        throw std::runtime_error("File not found");
    }
    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    // Read entire file contents in to memory
    if (filesize > 0)
    {
        buffer.resize(filesize);
        size_t chars_read = fread(&(buffer.front()), sizeof(char), filesize, fp);
        buffer.resize(chars_read);
        buffer.shrink_to_fit();
    }
    fclose(fp);
#ifdef _UNICODE
    return UTF8ToWideChar(buffer);
#else
    return WideCharToACP(UTF8ToWideChar(buffer));
#endif
}


// Settings of the service

// Service start options.
#define SERVICE_START_TYPE       SERVICE_AUTO_START

// List of service dependencies - "dep1\0dep2\0\0"
#define SERVICE_DEPENDENCIES     TEXT("")

// The name of the account under which the service should run
// #define SERVICE_ACCOUNT          TEXT("NULL\\NULL")
// #define SERVICE_ACCOUNT          TEXT("NT AUTHORITY\\LocalService")
#define SERVICE_ACCOUNT          NULL

// The password to the service account name
#define SERVICE_PASSWORD         NULL

#include "../mingw-unicode-main/mingw-unicode.c"
//
//  FUNCTION: wmain(int, TCHAR *[])
//
//  PURPOSE: entrypoint for the application.
//
//  PARAMETERS:
//    argc - number of command line arguments
//    argv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    wmain() either performs the command line task, or run the service.
//
int _tmain(int argc, TCHAR **argv)
{
    using namespace rapidxml;
    xml_document<TCHAR> doc;    // character type defaults to char
    Descriptor d;

    TCHAR szPath[MAX_PATH];

    if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)) == 0)
    {
        _tprintf(TEXT("GetModuleFileName failed w/err 0x%08lx\n"), GetLastError());
        return 1;
    }
    String exefilename = szPath;
    d.directory = exefilename.substr(0,exefilename.find_last_of(TEXT('\\')));
    d.logpath = d.directory + TEXT("\\logs");
    String xmlfilename=exefilename.substr(0,
                             exefilename.find_last_of(TEXT('.'))) + TEXT(".xml");
    String str;
    try
    {
        str = LoadUtf8FileToString(xmlfilename);
    }
    catch(std::runtime_error e)
    {
        Cout << TEXT("Error loading XML file: ") << e.what() << TEXT("\n");
        return 2;
    }
    try
    {        
        doc.parse<0>((TCHAR*)str.c_str());   // 0 means default parse flags
    }
    catch(std::runtime_error e)
    {
        Cout << TEXT("Error parsing XML file: ") << e.what() << TEXT("\n");
        return 3;
    }
    xml_node<TCHAR> *svc_node = doc.first_node(TEXT("service"));
    for (xml_node<TCHAR> *node = svc_node->first_node(); node;
         node = node->next_sibling())
    {
        if (_tcsicmp(TEXT("id"), node->name()) == 0)
        {
            d.id = node->value();
        }
        else if (_tcsicmp(TEXT("name"), node->name()) == 0)
        {
            d.name = node->value();
        }
        else if (_tcsicmp(TEXT("description"), node->name()) == 0)
        {
            d.description = node->value();
        }
        else if (_tcsicmp(TEXT("executable"), node->name()) == 0)
        {
            d.executable = node->value();
        }
        else if (_tcsicmp(TEXT("workingdirectory"), node->name()) == 0)
        {
            d.workingdirectory = node->value();
        }
        else if (_tcsicmp(TEXT("env"), node->name()) == 0)
        {
            xml_attribute<TCHAR> *attr_name = node->first_attribute(TEXT("name"));
            xml_attribute<TCHAR> *attr_value = node->first_attribute(TEXT("value"));
            if (attr_name == NULL || attr_value == NULL)
            {
                continue;
            }
            d.env.push_back(std::make_pair(attr_name->value(), attr_value->value()));
        }
        else if (_tcsicmp(TEXT("logpath"), node->name()) == 0)
        {
            d.logpath = node->value();
        }
        else if (_tcsicmp(TEXT("logmode"), node->name()) == 0)
        {
            d.logmode = node->value();
        }
        else if (_tcsicmp(TEXT("startargument"), node->name()) == 0)
        {
            d.startargument.push_back(node->value());
        }
        else if (_tcsicmp(TEXT("stopexecutable"), node->name()) == 0)
        {
            d.stopexecutable = node->value();
        }
        else if (_tcsicmp(TEXT("stoparguments"), node->name()) == 0)
        {
            d.stoparguments = node->value();
        }
        else if (_tcsicmp(TEXT("stopargument"), node->name()) == 0)
        {
            d.stopargument.push_back(node->value());
        }
    }
    if (!CreateRecursiveDirectory(d.logpath.c_str()))
    {
        Cout << TEXT("Can't create log directory \"") << d.logpath << TEXT("\"\n");
        return 4;
    }
    if (argc > 1)
    {
        if (_tcsicmp(TEXT("install"), argv[1]) == 0)
        {
            // Install the service when the command is
            // "install".
            InstallService(
                d.id.c_str(),               // Name of service
                d.name.c_str(),             // Name to display
                d.description.c_str(),             // Description
                SERVICE_START_TYPE,         // Service start type
                SERVICE_DEPENDENCIES,       // Dependencies
                SERVICE_ACCOUNT,            // Service running account
                SERVICE_PASSWORD            // Password of the account
            );
        }
        else if (_tcsicmp(TEXT("uninstall"), argv[1]) == 0)
        {
            // Uninstall the service when the command isn "uninstall".
            UninstallService(d.id.c_str());
        }
        else if (_tcsicmp(TEXT("help"), argv[1]) == 0)
        {
            _tprintf(TEXT("Parameters:\n"));
            _tprintf(TEXT(" install    to install the service.\n"));
            _tprintf(TEXT(" uninstall  to remove the service.\n"));
        }
        else if (_tcsicmp(TEXT("test"), argv[1]) == 0)
        {
            CSampleService service(&d, d.name.c_str());
            service.Test();
        }
    }
    else
    {
        CSampleService service(&d, d.name.c_str());
        if (!CServiceBase::Run(service))
        {
            _tprintf(TEXT("Service failed to run w/err 0x%08lx\n"), GetLastError());
        }
    }

    return 0;
}