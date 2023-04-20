CPP    = g++
RM     = rm -f
OBJS   = ../src/CppWindowsService.o \
         ../src/SampleService.o \
         ../src/utils.o \
         ../src/ServiceInstaller.o \
         ../src/ServiceBase.o

LIBS   = -m64 -std=c++11
CFLAGS = -m64 -std=c++11 -DUNICODE -D_UNICODE -I..\vendor\rapidxml -fno-diagnostics-show-option

.PHONY: all

all: ../bin/x64/SvcWrapper.exe

clean:
	$(RM) $(OBJS) ../bin/x64/SvcWrapper.exe

clear:
	$(RM) $(OBJS)

../bin/x64/SvcWrapper.exe: $(OBJS)
	$(CPP) -Wall -s -O2 -o $@ $(OBJS) $(LIBS)

../src/CppWindowsService.o: ../src/CppWindowsService.cpp ../src/ServiceInstaller.h ../src/ServiceBase.h ../src/SampleService.h ../vendor/rapidxml/rapidxml.hpp ../src/strings.h ../src/Descriptor.h ../src/utils.h ../vendor/mingw-unicode-main/mingw-unicode.c
	$(CPP) -Wall -s -O2 -c $< -o $@ $(CFLAGS)

../src/SampleService.o: ../src/SampleService.cpp ../src/SampleService.h ../src/ThreadPool.h
	$(CPP) -Wall -s -O2 -c $< -o $@ $(CFLAGS)

../src/utils.o: ../src/utils.cpp ../src/utils.h ../src/strings.h ../src/Descriptor.h
	$(CPP) -Wall -s -O2 -c $< -o $@ $(CFLAGS)

../src/ServiceInstaller.o: ../src/ServiceInstaller.cpp ../src/ServiceInstaller.h ../src/strings.h
	$(CPP) -Wall -s -O2 -c $< -o $@ $(CFLAGS)

../src/ServiceBase.o: ../src/ServiceBase.cpp ../src/ServiceBase.h ../src/nsis_tchar.h
	$(CPP) -Wall -s -O2 -c $< -o $@ $(CFLAGS)

