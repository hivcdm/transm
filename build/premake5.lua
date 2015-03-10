solution "transm"
    configurations { "debug", "release" }
    platforms { "x64" }
    location ("./" .. _ACTION)

project "transm"
    kind "ConsoleApp"
    language "C++"
    targetname "transm"
    targetsuffix ("-v" .. os.outputof("cat ../VERSION"))
    links { "third-party" }
    flags { 
       "Symbols",
       "Unicode",
       "NoPCH"
    }
    warnings "Extra"
    files {
       "../source/**.h",
       "../source/**.cpp"
    }
    excludes {
       "../source/utility/platform/**"
    }
    includedirs {
       "../source",
       "../third-party/boost/include",
       "../third-party/cepac/src",
       "../third-party/pugixml/src",
       "../third-party/tclap/include"
    }
    configuration "debug"
        flags { "FatalWarnings" }
	optimize "Off"
	targetdir "../bin/debug"
    configuration "release"
        flags { "LinkTimeOptimization" }
	optimize "Full"
	targetdir "../bin/release"
    configuration "windows"
        files { "../source/utility/platform/windows/**.cpp" }
        defines { "_SCL_SECURE_NO_WARNINGS" }
	files { "resources/resource.rc" }
    configuration "not windows"
        files { "../source/utility/platform/posix/**.cpp" }
	buildoptions { "-Wno-unknown-pragmas" }
    configuration "macosx"
        buildoptions { "-Wno-deprecated-register" }

project "third-party"
    kind "StaticLib"
    language "C++"
    targetname "third-party"
    targetsuffix ("-v" .. os.outputof("cat ../VERSION"))
    flags { 
       "Symbols",
       "Unicode",
       "NoPCH"
    }
    warnings "Off"
    files {
       "../third-party/pugixml/src/pugixml.cpp",
       "../third-party/cepac/src/*.cpp"
    }
    includedirs {
       "../third-party/pugixml/src",
       "../third-party/tclap/include"
    }
    configuration "debug"
        optimize "Off"
	targetdir "../bin/debug"
    configuration "release"
        flags { "LinkTimeOptimization" }
        optimize "Full"
	targetdir "../bin/release"
