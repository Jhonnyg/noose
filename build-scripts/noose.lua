NOOSE_ROOT_PATH  = (path.getabsolute("") .. "/") .. "../"
NOOSE_BUILD_PATH = path.join(NOOSE_ROOT_PATH,"build")
NOOSE_BIN_PATH   = path.join(NOOSE_ROOT_PATH,"bin")
NOOSE_SRC_PATH   = path.join(NOOSE_ROOT_PATH,"src")

solution "noose"
    language       ( "C++" )
    location       ( NOOSE_BUILD_PATH )

    configurations { "Debug", "Release" }
    platforms      { "x64" }
    flags          { "NoPCH" } -- "FatalWarnings",
    buildoptions   { "-Wno-switch"}

    configuration "Debug"
        defines { "DEBUG" }
        flags   { "Symbols" }

    configuration "Release"
        defines { "NDEBUG" }
        flags   { "Optimize" }

project "noose"
    objdir      ( NOOSE_BUILD_PATH )
    kind        ( "ConsoleApp" )
    targetname  ( "noose" )
    targetdir   ( NOOSE_BIN_PATH )
    files       { path.join(NOOSE_SRC_PATH, "**.cpp") }
    includedirs { NOOSE_SRC_PATH }

print("ello govenor")
print(NOOSE_ROOT_PATH)
