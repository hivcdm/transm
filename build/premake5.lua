solution "transm"
    configurations { "Debug", "Release" }
    platforms { "x32", "x64" }
    location ("workspaces/" .. _ACTION)
    includedirs { "$(boost_prefix)", "../source" }
    configuration "x32"
        libdirs { "$(boost_prefix)/lib32-msvc-12.0" }
    configuration "x64"
        libdirs { "$(boost_prefix)/lib64-msvc-12.0" }
    flags { 
       "Unicode",
       "NoEditAndContinue",
       "NoManifest",
       "NoPCH"
    }
    configuration "Debug"
        flags { "Symbols" }
	optimize "Off"
	targetdir "../binaries/debug"
    configuration "Release"
        optimize "Full"
	targetdir "../binaries/release"

project "transm.cli"
    kind "ConsoleApp"
    language "C++"
    includedirs { "../source" }
    files { "../source/transm.cli/main.cpp" }
    links { "transm" }
    configuration "Release"
        flags { "LinkTimeOptimization" }
    debugargs { "../source/transm.test/runs/34_standard" }

project "transm.gui"
    kind "WindowedApp"
    language "C++"
    warnings "Extra"
    files {
       "../source/transm.gui/**.h",
       "../source/transm.gui/**.cpp"
    }
    links {
       "transm",
       "wx"
    }
    configuration "Debug"
        flags { "FatalWarnings" }

project "transm.test"
    kind "ConsoleApp"
    language "C++"
    warnings "Extra"
    files {
       "../source/transm.test/**.h",
       "../source/transm.test/**.cpp"
    }
    links { "transm" }
    configuration "Debug"
        flags { "FatalWarnings" }

project "cepac"
    kind "StaticLib"
    language "C++"
    files {
       "../source/cepac/*.cpp",
       "../source/cepac/*.h"
    }

project "transm"
    kind "StaticLib"
    language "C++"
    warnings "Extra"
    files {
       "../source/transm/**.cpp",
       "../source/transm/**.h"
    }
    excludes {
       "../source/transm/util/HighResolutionTimerPosix.cpp",
       "../source/transm/main.cpp"
    }
    links { "cepac" }
    configuration "Debug"
        flags { "FatalWarnings" }

project "cepac.cli"
    kind "ConsoleApp"
    language "C++"
    files { "../source/cepac.cli/main.cpp" }
    links { "transm" }
    debugargs { "../tests/runs/34_standard" }
    configuration "Release"
        flags { "LinkTimeOptimization" }

project "cepac.gui"
    kind "WindowedApp"
    language "C++"
    warnings "Extra"
    files {
       "../source/cepac.gui/**.h",
       "../source/cepac.gui/**.cpp"
    }
    links {
       "transm",
       "wx"
    }
    configuration "Release"
        flags { "LinkTimeOptimization" }

if _ACTION == "clean" then
   os.rmdir("workspaces")
   os.rmdir("../binaries")
end
