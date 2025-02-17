:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
:: Contact: http://www.qt-project.org/legal
::
:: This file is part of the tools applications of the Qt Toolkit.
::
:: $QT_BEGIN_LICENSE:LGPL21$
:: Commercial License Usage
:: Licensees holding valid commercial Qt licenses may use this file in
:: accordance with the commercial license agreement provided with the
:: Software or, alternatively, in accordance with the terms contained in
:: a written agreement between you and Digia. For licensing terms and
:: conditions see http://qt.digia.com/licensing. For further information
:: use the contact form at http://qt.digia.com/contact-us.
::
:: GNU Lesser General Public License Usage
:: Alternatively, this file may be used under the terms of the GNU Lesser
:: General Public License version 2.1 or version 3 as published by the Free
:: Software Foundation and appearing in the file LICENSE.LGPLv21 and
:: LICENSE.LGPLv3 included in the packaging of this file. Please review the
:: following information to ensure the GNU Lesser General Public License
:: requirements will be met: https://www.gnu.org/licenses/lgpl.html and
:: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
::
:: In addition, as a special exception, Digia gives you certain additional
:: rights. These rights are described in the Digia Qt LGPL Exception
:: version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
::
:: $QT_END_LICENSE$
::
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

@echo off
set QTSRC=%~dp0
set QTDIR=%CD%
if not exist %QTSRC%\.gitignore goto sconf
echo Please wait while bootstrapping configure ...

for %%C in (cl.exe icl.exe g++.exe perl.exe) do set %%C=%%~$PATH:C

if "%perl.exe%" == "" (
    echo Perl not found in PATH. Aborting. >&2
    exit /b 1
)
if not exist mkspecs (
    md mkspecs
    if errorlevel 1 goto exit
)
perl %QTSRC%bin\syncqt.pl -minimal -module QtCore -outdir %QTDIR% %QTSRC%
if errorlevel 1 goto exit

if not exist tools\configure (
    md tools\configure
    if errorlevel 1 goto exit
)
cd tools\configure
if errorlevel 1 goto exit

echo #### Generated by configure.bat - DO NOT EDIT! ####> Makefile
echo/>> Makefile
for /f "tokens=3 usebackq" %%V in (`findstr QT_VERSION_STR %QTSRC%\src\corelib\global\qglobal.h`) do @echo QTVERSION = %%~V>> Makefile
if not "%cl.exe%" == "" (
    echo CXX = cl>>Makefile
    echo EXTRA_CXXFLAGS =>>Makefile
    rem This must have a trailing space.
    echo QTSRC = %QTSRC% >> Makefile
    set tmpl=win32
    set make=nmake
) else if not "%icl.exe%" == "" (
    echo CXX = icl>>Makefile
    echo EXTRA_CXXFLAGS = /Zc:forScope>>Makefile
    rem This must have a trailing space.
    echo QTSRC = %QTSRC% >> Makefile
    set tmpl=win32
    set make=nmake
) else if not "%g++.exe%" == "" (
    echo CXX = g++>>Makefile
    echo EXTRA_CXXFLAGS =>>Makefile
    rem This must NOT have a trailing space.
    echo QTSRC = %QTSRC:\=/%>> Makefile
    set tmpl=mingw
    set make=mingw32-make
) else (
    echo No suitable compiler found in PATH. Aborting. >&2
    cd ..\..
    exit /b 1
)
echo/>> Makefile
type %QTSRC%tools\configure\Makefile.%tmpl% >> Makefile

%make%
if errorlevel 1 (cd ..\.. & exit /b 1)

cd ..\..

:conf
configure.exe -srcdir %QTSRC% %*
goto exit

:sconf
%QTSRC%\configure.exe %*
:exit
