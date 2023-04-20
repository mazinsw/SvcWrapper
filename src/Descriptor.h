#ifndef _DESCRIPTOR_H_
#define _DESCRIPTOR_H_
#include <string>
#include <vector>
#include "strings.h"

class Descriptor
{
    public:
        String id;
        String name;
        String description;
        String executable;
        std::vector<std::pair<String, String> > env;
        String logpath;
        String logmode;
        std::vector<String> startargument;
        String stopexecutable;
        std::vector<String> stopargument;
        String stoparguments;
        
        String directory;
        String workingdirectory;
        
        String quoteParam(String param)
        {
            if (param.size() > 0 && param[0] == TEXT('"'))
            {
                return param;
            }
            if (param.find_first_of(TEXT(' ')) != String::npos)
            {
                return TEXT("\"") + param + TEXT("\"");
            }
            return param;
        }
            
        String envCmd()
        {
            String arguments;
            std::vector<std::pair<String, String> >::iterator it;

            for(it = env.begin(); it != env.end(); it++)
            {
                arguments += it->first + TEXT("=") + it->second;
                arguments.push_back(TEXT('\0'));
            }
            return arguments;
        }
            
        String startarguments()
        {
            String arguments;
            std::vector<String>::iterator it;

            for(it = startargument.begin(); it != startargument.end(); it++)
            {
                arguments += quoteParam(*it) + TEXT(" ");
            }
            return arguments;
        }
            
        String stopargumentsCmd()
        {
            String arguments;
            std::vector<String>::iterator it;

            for(it = stopargument.begin(); it != stopargument.end(); it++)
            {
                arguments += quoteParam(*it) + TEXT(" ");
            }
            return arguments + TEXT(" ") + stoparguments;
        }
        
        String currentDirectory()
        {
            if (workingdirectory.size() == 0)
            {
                return directory;
            }
            return workingdirectory;
        }
};

#endif /* _DESCRIPTOR_H_ */