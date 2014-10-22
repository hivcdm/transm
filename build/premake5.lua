solution "transm"
    configurations { "debug", "release" }
    platforms { "x64" }
    location ("./" .. _ACTION)
    libdirs { 
       "../third-party/cepac/lib",
       "../third-party/boost/lib"
    }
    includedirs { 
       "../third-party/cepac/src",
       "../third-party/boost/include"
    }
    configuration "debug"
        flags { "Symbols" }
	optimize "Off"
    configuration "release"
        optimize "Full"
    configuration "not windows"
        buildoptions {
            "-std=c++11",
            "-Wno-unknown-pragmas"
        }

project "transm.cli"
    kind "ConsoleApp"
    language "C++"
    targetname "transm"
    warnings "Extra"
    targetdir "../bin"
    flags { 
       "Unicode",
       "NoEditAndContinue",
       "NoManifest",
       "NoPCH"
    }
    files {
       "../source/main.cpp",
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
    configuration "debug"
        flags { "FatalWarnings" }
	links { "../third-party/cepac/lib/cepacd" }
    configuration "release"
        flags { "LinkTimeOptimization" }
	links { "../third-party/cepac/lib/cepac" }
    configuration "windows"
        files { "../source/util/HighResolutionTimerWindows.cpp" }
    configuration "not windows"
        files { "../source/util/HighResolutionTimerPosix.cpp" }
    configuration "vs*"
        defines { "_SCL_SECURE_NO_WARNINGS" }
    targetsuffix ("-v" .. os.outputof("cat ../VERSION"))

project "transm.gui"
    kind "WindowedApp"
    language "C++"
    targetname "transm"
    warnings "Extra"
    targetdir "../bin"
    includedirs {
       "../third-party/wxWidgets/include"
    }
    defines { "wxUSE_GUI=1" }
    files {
       "../source/gui/**.h",
       "../source/gui/**.cpp",
       "../source/core/**.cpp",
       "../source/core/**.h",
       "../source/data/**.cpp",
       "../source/data/**.h",
       "../source/entities/**.cpp",
       "../source/entities/**.h",
       "../source/statistics/**.cpp",
       "../source/statistics/**.h",
       "../source/util/**.cpp",
       "../source/util/**.h",
       "../build/resources/resource.h",
       "../build/resources/resource.rc"
    }
    excludes {
       "../source/util/HighResolutionTimer*.cpp"
    }
    flags { 
       "Unicode",
       "NoEditAndContinue",
       "NoManifest",
       "NoPCH"
    }
    configuration "debug"
        flags { "FatalWarnings" }
	defines { "__WXDEBUG__" }
	links { "../third-party/cepac/lib/cepacd" }
    configuration "release"
        flags { "LinkTimeOptimization" }
	links { "../third-party/cepac/lib/cepac" }
    configuration "windows"
        defines { 
	   "WINVER=0x0610",
	   "__WXMSW__",
	   "_WINDOWS",
	   "WIN32"
	}
        files { "../source/util/HighResolutionTimerWindows.cpp" }
	targetsuffix ("-win-v" .. os.outputof("cat ../VERSION"))
    configuration "not windows"
        files { "../source/util/HighResolutionTimerPosix.cpp" }
    configuration "vs*"
        defines { "_CRT_SECURE_NO_WARNINGS" }
        defines { "_SCL_SECURE_NO_WARNINGS" }
	flags { "WinMain" }
	libdirs { "../third-party/wxWidgets/lib/vc_x64_lib" }
    targetsuffix ("-gui-v" .. os.outputof("cat ../VERSION"))
