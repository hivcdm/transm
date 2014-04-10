solution "transm"
    configurations { "Debug", "Release" }
    platforms { "x64", "x32" }
    location ("workspaces/" .. _ACTION)
    includedirs { "$(boost_prefix)", "../source" }
    configuration "x32"
        libdirs { "$(boost_prefix)/lib32-msvc-12.0" }
    configuration "x64"
        libdirs { "$(boost_prefix)/lib64-msvc-12.0" }
    configuration "Debug"
        flags { "Symbols" }
	optimize "Off"
    configuration "Release"
        optimize "Full"

project "transm.cli"
    kind "ConsoleApp"
    language "C++"
    targetname "transm"
    includedirs { "../source" }
    files { "../source/transm.cli/main.cpp" }
    links { "transm" }
    flags { 
       "Unicode",
       "NoEditAndContinue",
       "NoManifest",
       "NoPCH"
    }
    debugargs { "../source/transm.test/runs/34_standard" }
    configuration "Debug"
	targetdir "../binaries/debug"
    configuration "Release"
        flags { "LinkTimeOptimization" }
	targetdir "../binaries/release"

project "transm.gui"
    kind "WindowedApp"
    language "C++"
    targetname "transm-gui"
    warnings "Extra"
    includedirs {
       "$(wx_prefix)/include/msvc",
       "$(wx_prefix)/include"
    }
    defines { "wxUSE_GUI=1" }
    files {
       "../source/transm.gui/**.h",
       "../source/transm.gui/**.cpp"
    }
    links { "transm" }
    flags { 
       "Unicode",
       "NoEditAndContinue",
       "NoManifest",
       "NoPCH"
    }
    configuration "Debug"
        flags { "FatalWarnings" }
	defines { "__WXDEBUG__" }
	targetdir "../binaries/debug"
    configuration { "x64", "vs2013" }
	libdirs { "$(wx_prefix)/vc120_x64_dll" }
    configuration { "x32", "vs2013" }
	libdirs { "$(wx_prefix)/vc120_dll" }
    configuration { "x32", "vs2010" }
	libdirs { "$(wx_prefix)/vc100_dll" }
    configuration "windows"
        defines { 
	   "WINVER=0x0610",
	   "__WXMSW__",
	   "_WINDOWS",
	   "WIN32"
	}
    configuration "vs*"
        defines { 
	   "wxMSVC_VERSION_AUTO",
	   "_CRT_SECURE_NO_WARNINGS"
	}
	flags { "WinMain" }
    configuration "Release"
        flags { "LinkTimeOptimization" }
	targetdir "../binaries/release"

project "transm.test"
    kind "ConsoleApp"
    language "C++"
    targetname "transm-test"
    warnings "Extra"
    files {
       "../source/transm.test/**.h",
       "../source/transm.test/**.cpp"
    }
    links { "transm" }
    flags { 
       "Unicode",
       "NoEditAndContinue",
       "NoManifest",
       "NoPCH"
    }
    configuration "Debug"
        flags { "FatalWarnings" }
	targetdir "../binaries/debug"
    configuration "Release"
        flags { "LinkTimeOptimization" }
	targetdir "../binaries/release"

project "cepac.cli"
    kind "ConsoleApp"
    language "C++"
    targetname "cepac"
    files { "../source/cepac.cli/main.cpp" }
    links { "transm" }
    debugargs { "../tests/runs/34_standard" }
    flags { 
       "Unicode",
       "NoEditAndContinue",
       "NoManifest",
       "NoPCH"
    }
    configuration "Debug"
        flags { "FatalWarnings" }
	targetdir "../binaries/debug"
    configuration "Release"
        flags { "LinkTimeOptimization" }
	targetdir "../binaries/release"

project "cepac.gui"
    kind "WindowedApp"
    language "C++"
    targetname "cepac-gui"
    warnings "Extra"
    files {
       "../source/cepac.gui/**.h",
       "../source/cepac.gui/**.cpp"
    }
    links {
       "transm",
       "wx"
    }
    flags { 
       "Unicode",
       "NoEditAndContinue",
       "NoManifest",
       "NoPCH"
    }
    configuration "Debug"
        flags { "FatalWarnings" }
	targetdir "../binaries/debug"
    configuration "Release"
        flags { "LinkTimeOptimization" }
	targetdir "../binaries/release"

project "cepac"
    kind "StaticLib"
    language "C++"
    targetdir "../lib/$(IntDir)"
    files {
       "../source/cepac/*.cpp",
       "../source/cepac/*.h"
    }
    flags { 
       "Unicode",
       "NoEditAndContinue",
       "NoManifest",
       "NoPCH"
    }

project "transm"
    kind "StaticLib"
    language "C++"
    warnings "Extra"
    targetdir "../lib/$(IntDir)"
    files {
       "../source/transm/**.cpp",
       "../source/transm/**.h"
    }
    excludes {
       "../source/transm/util/HighResolutionTimerPosix.cpp",
       "../source/transm/main.cpp"
    }
    links { "cepac" }
    flags { 
       "Unicode",
       "NoEditAndContinue",
       "NoManifest",
       "NoPCH"
    }
    configuration "Debug"
        flags { "FatalWarnings" }

if _ACTION == "clean" then
   os.rmdir("workspaces")
   os.rmdir("../binaries")
end
