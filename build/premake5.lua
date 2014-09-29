solution "transm"
    configurations { "debug", "release" }
    platforms { "x64" }
    location ("./" .. _ACTION)

project "transm"
    kind "ConsoleApp"
    language "C++"
    targetdir "../bin"
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
       "../source/**.hpp",
       "../source/**.cpp"
    }
    excludes {
       "../source/utility/platform/**"
    }
    includedirs {
       "../source",
       "../third-party/boost",
       "../third-party/rana/include",
       "../third-party/cepac/src",
       "../third-party/pugixml/src",
       "../third-party/sqlite",
       "../third-party/tclap/include",
       "../third-party/xlnt/include"
    }
    libdirs {
       "../third-party/boost/lib64-msvc-12.0"
    }
    configuration "debug"
        flags { "FatalWarnings" }
    configuration "release"
        flags { "LinkTimeOptimization" }
    configuration "windows"
        files { "../source/utility/platform/windows/**.cpp" }
        defines { "_SCL_SECURE_NO_WARNINGS" }
	files { "resources/resource.rc" }
    configuration "not windows"
        files { "../source/utility/platform/posix/**.cpp" }

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
    defines {
        "SQLITE_THREADSAFE=0",
	"SQLITE_OMIT_LOAD_EXTENSION"
    }
    warnings "Off"
    files {
       "../third-party/sqlite/sqlite3.c",
       "../third-party/pugixml/src/pugixml.cpp",
       "../third-party/cepac/src/*.cpp",
       "../third-party/xlnt/source/**.hpp",
       "../third-party/xlnt/source/**.cpp",
       "../third-party/xlnt/third-party/miniz/miniz.c"
    }
    includedirs {
       "../third-party/pugixml/src",
       "../third-party/sqlite",
       "../third-party/tclap/include",
       "../third-party/xlnt/include"
    }
    configuration "release"
        flags { "LinkTimeOptimization" }
