#��������ѡ��
LINK_FLAGS = /NOLOGO /SUBSYSTEM:WINDOWS /MACHINE:X86  /out:../bin/Direct3D11Framework.exe
#������ļ�
LIBS =  "kernel32.lib" "user32.lib" "gdi32.lib"
#�����·��
LIBPATHS = /LIBPATH:D:\ExtensionLibrary\DXSDK\lib\x86	\
	/LIBPATH:D:\ExtensionLibrary\General\lib	\
	/LIBPATH:D:\ExtensionLibrary\General\lib\Dynamic
#��������ļ�·��
OBJPATH= Release\*.obj
#����Դ�ļ�
SOURCES = src/*.cpp
#�������ѡ��
COMPILER_FLAGS =  /c /MD /nologo /W3 /WX- /O2 /Oi /Oy- \
		  /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" \
		  /Gm- /EHsc /GS /Gy /fp:precise /Zc:wchar_t /Zc:forScope
#��������ļ�·��
INCLUDES = /I./include	\
	   /ID:\ExtensionLibrary\DXSDK\include	\
	   /ID:\ExtensionLibrary\General\include	\
	   /ID:\ExtensionLibrary\General\include\colladadom	\
	   /ID:\ExtensionLibrary\General\include\colladadom\1.4
#ָ���м��ļ�����·��
MEDIATE_DIR = /FoRelease/ /FdRelease/vc100.pdb
#ִ�б������ӹ���
target:compile
	link $(OBJPATH) $(LIBS) $(LIBPATHS) $(LINK_FLAGS) 
compile:
	cl $(SOURCES) $(COMPILER_FLAGS) $(INCLUDES) $(MEDIATE_DIR)
run:
	..\bin\Direct3D11Framework.exe
clean:
	del $(OBJPATH) /Q

