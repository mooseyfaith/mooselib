@echo off

set mode=debug
rem set mode=release

set name=moose_preprocessor
set gui_application=0
set application_init_function=application_init
set application_main_loop_function=application_main_loop
set source=preprocessor.cpp

rem paths relative to build dir
set moose_dir=%cd%/

set include_dirs=/I "%moose_dir%3rdparty" /I "%moose_dir%code"
set libs=kernel32.lib user32.lib shell32.lib gdi32.lib opengl32.lib
set options=/Zi /nologo /EHsc
set link_options=/link /INCREMENTAL:NO
set dll_flag= 

if %gui_application%==1 (
	set include_dirs=%include_dirs% /I "%moose_dir%3rdparty\freetype\include\freetype2"
	set dll_flag=/LD

	if %mode%==debug (
		set libs=%libs% "%moose_dir%3rdparty\freetype\lib\freetyped.lib"
	) else (
		set libs=%libs% "%moose_dir%3rdparty\freetype\lib\freetype.lib"
	)
)

if %mode%==debug (
	set options=%options% /Od /DDEBUG /MTd
	echo debug mode
) else (
	set options=%options% /O2 /MT
	echo release mode
)

rem echo include_dirs %include_dirs%
rem echo libs %libs%
rem echo options %options% %dll_flag%
rem echo link_options %link_options%

if not exist build\ mkdir build
pushd build

rem check for live-code-editing: try to rename .exe, if its not possible, we assume the .exe is running
if %gui_application%==1 (if exist "%name%.exe" (
	COPY /B "%name%.exe"+NUL "%name%.exe" > NUL 2> NUL

	if errorlevel 1 goto enable_live_code_editing
))

set live_code_editing=0
echo normal compile mode
goto skip_enable_live_code_editing

:enable_live_code_editing:
echo live code editing mode
set live_code_editing=1

:skip_enable_live_code_editing
rem end of live-code-editing check

rem echo buid directory %cd%

set t=%time:~0,8%
set t=%t::=-%

del *.pdb > NUL 2> NUL

echo "compiling dll" > compile_dll_lock.tmp
rem option /ignore:4099 disables linker warning, that .pdb files for freetype.lib do not exists (we only use release build)
cl -Fe%name% %options% %dll_flag% "%cd%\..\code\%source%" %libs% /DWIN32 /DWIN32_EXPORT %include_dirs% %link_options% /ignore:4099 /PDB:"%name%%date%-%t%.pdb"

if errorlevel 1 (
	del compile_dll_lock.tmp
	popd
	exit /B
)

del compile_dll_lock.tmp

if %live_code_editing%==0 (
	if %gui_application%==1 (
		cl -Fe"%name%" %options% /DWIN32_DLL_NAME=\"%name%.dll\" /DWIN32_INIT_FUNCTION_NAME=\"%application_init_function%\" /DWIN32_MAIN_LOOP_FUNCTION_NAME=\"%application_main_loop_function%\" 	%include_dirs% "%moose_dir%code\win32_platform.cpp" %libs% %link_options%

		if errorlevel 1 (
			popd
			exit /B
		)
	)

	if not exist "%name%.sln" (
		echo creating visual studio debug solution
		echo please set the working directory to "%cd%\..\data"
		devenv "%name%.exe"
	)
)

popd
