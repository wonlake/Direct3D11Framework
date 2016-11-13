#定义链接选项
LINK_FLAGS = /NOLOGO /SUBSYSTEM:WINDOWS /MACHINE:X86  /out:../bin/Direct3D11Framework.exe
#定义库文件
LIBS =  "kernel32.lib" "user32.lib" "gdi32.lib"
#定义库路径
LIBPATHS = /LIBPATH:D:\ExtensionLibrary\DXSDK\lib\x86	\
	/LIBPATH:D:\ExtensionLibrary\General\lib	\
	/LIBPATH:D:\ExtensionLibrary\General\lib\Dynamic
#定义对象文件路径
OBJPATH= Release\*.obj
#定义源文件
SOURCES = src/*.cpp
#定义编译选项
COMPILER_FLAGS =  /c /MD /nologo /W3 /WX- /O2 /Oi /Oy- \
		  /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" \
		  /Gm- /EHsc /GS /Gy /fp:precise /Zc:wchar_t /Zc:forScope
#定义包含文件路径
INCLUDES = /I./include	\
	   /ID:\ExtensionLibrary\DXSDK\include	\
	   /ID:\ExtensionLibrary\General\include	\
	   /ID:\ExtensionLibrary\General\include\colladadom	\
	   /ID:\ExtensionLibrary\General\include\colladadom\1.4
#指定中间文件生成路径
MEDIATE_DIR = /FoRelease/ /FdRelease/vc100.pdb
#执行编译链接过程
target:compile
	link $(OBJPATH) $(LIBS) $(LIBPATHS) $(LINK_FLAGS) 
compile:
	cl $(SOURCES) $(COMPILER_FLAGS) $(INCLUDES) $(MEDIATE_DIR)
run:
	..\bin\Direct3D11Framework.exe
clean:
	del $(OBJPATH) /Q

