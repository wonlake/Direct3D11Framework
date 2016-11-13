@echo off
if exist "Release" (echo "Release exists!") else ( mkdir "Release" )
path= %path%;D:\CodeBlocks\MinGW\bin;
cmd /k ""D:\Visual Studio 2010\VC\vcvarsall.bat"" x86

@echo on
