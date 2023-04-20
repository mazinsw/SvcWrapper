#include <vector>
#include "utils.h"
#include "strings.h"
#include "Descriptor.h"

bool CreateRecursiveDirectory(const TCHAR* path)
{
    String folder_name = path;
    const TCHAR *pos = NULL, *start;
    
    if (folder_name[folder_name.size() - 1] != TEXT('\\'))
    {
        folder_name += TEXT('\\');
    }
    start = folder_name.c_str();
    pos = folder_name.c_str();
    do
    {
        pos = _tcschr(pos, TEXT('\\'));
        if (pos == NULL)
        {
            break;
        }
        if (*(pos - 1) == TEXT(':'))
        {
            pos++;
            continue;
        }
        folder_name[pos - start] = TEXT('\0');
        if (!CreateDirectory(folder_name.c_str(), NULL))
        {
            DWORD dwError = GetLastError();
            if (dwError != ERROR_ALREADY_EXISTS)
            {
                return false;
            }
        }
        folder_name[pos - start] = TEXT('\\');
        pos++;
    } while(true);
    return true;
}