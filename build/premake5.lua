solution "transm"
    configurations { "Debug", "Release" }
    platforms { "x64" }
    location ("./" .. _ACTION)
    configuration "not windows"
        buildoptions { 
            "-std=c++11",
            "-Wno-unknown-pragmas"
        }
    configuration "vs*"
        libdirs { "$(cepac_prefix)/lib" }
        includedirs { 
	   "$(boost_prefix)",
	   "$(cepac_prefix)/src"
	}
    configuration { "vs*", "x32" }
        libdirs { "$(boost_prefix)/lib32-msvc-12.0" }
    configuration { "vs*", "x64" }
        libdirs { "$(boost_prefix)/lib64-msvc-12.0" }
    configuration "not windows"
        links {
	    "boost_filesystem",
	    "boost_system"
	}
    configuration "Debug"
        flags { "Symbols" }
	optimize "Off"
    configuration "Release"
        optimize "Full"

project "transm.cli"
    kind "ConsoleApp"
    language "C++"
    targetname "transm"
    files { "../source/main.cpp" }
    links { 
        "cepac",
        "transm"
    }
    flags { 
       "Unicode",
       "NoEditAndContinue",
       "NoManifest",
       "NoPCH"
    }
    debugargs { "../../runs/34_standard" }
    configuration "Debug"
	targetdir "../bin/debug"
    configuration "Release"
        flags { "LinkTimeOptimization" }
	targetdir "../bin/release"

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
       "../source/gui/**.h",
       "../source/gui/**.cpp"
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
	targetdir "../bin/debug"
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
	targetdir "../bin/release"

project "transm.test"
    kind "ConsoleApp"
    language "C++"
    targetname "transm-test"
    warnings "Extra"
    files {
       "../tests/**.h",
       "../tests/**.cpp"
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
	targetdir "../bin/debug"
    configuration "Release"
        flags { "LinkTimeOptimization" }
	targetdir "../bin/release"

project "transm"
    kind "StaticLib"
    language "C++"
    warnings "Extra"
    targetdir "../lib/"
    files {
       "../source/core/**.cpp",
       "../source/core/**.h",
       "../source/data/**.cpp",
       "../source/data/**.h",
       "../source/entities/**.cpp",
       "../source/entities/**.h",
       "../source/statistics/**.cpp",
       "../source/statistics/**.h",
       "../source/util/**.cpp",
       "../source/util/**.h"
    }
    excludes {
       "../source/util/HighResolutionTimer*.cpp"
    }
    links { "cepac44a" }
    flags { 
       "Unicode",
       "NoEditAndContinue",
       "NoManifest",
       "NoPCH"
    }
    configuration "Debug"
        flags { "FatalWarnings" }
    configuration "windows"
        files { "../source/util/HighResolutionTimerWindows.cpp" }
    configuration "not windows"
        files { "../source/util/HighResolutionTimerPosix.cpp" }
