set_project("DirectX12Gadget")
set_languages("c++20")

add_rules("mode.debug", "mode.release")
add_requires("glfw", "directxtk", "tinyobjloader")

add_repositories("my-repo myrepo")
add_requires("directxtk12")

if is_mode("debug") then
    add_defines("DEBUG")
end

add_files("src/*.cpp")
add_syslinks("d3d12", "dxgi", "d3dcompiler")
add_packages("glfw", "directxtk12", "tinyobjloader")

target("Box")
    set_kind("binary")
    add_files("src/Box/*.cpp")
    add_defines("SHADER_DIR=L\"" .. path.join(os.projectdir(), "src/Box/shaders"):gsub("\\", "/") .. "\"" )

target("PBR")
    set_kind("binary")
    add_files("src/PBR/*.cpp")
    add_defines("SHADER_DIR=L\"" .. path.join(os.projectdir(), "src/PBR/shaders"):gsub("\\", "/") .. "\"" )

target("Cloth")
    set_kind("binary")
    add_files("src/Cloth/*.cpp")
    add_defines("SHADER_DIR=L\"" .. path.join(os.projectdir(), "src/Cloth/shaders"):gsub("\\", "/") .. "\"" )
    add_defines("MODEL_DIR=\"" .. path.join(os.projectdir(), "src/Cloth/models"):gsub("\\", "/") .. "\"" )

target("Shadow")
    set_kind("binary")
    add_files("src/Shadow/*.cpp")
    add_defines("SHADER_DIR=L\"" .. path.join(os.projectdir(), "src/Shadow/shaders"):gsub("\\", "/") .. "\"" )
    add_defines("TEXTURE_DIR=L\"" .. path.join(os.projectdir(), "src/Shadow/textures"):gsub("\\", "/") .. "\"" )

--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro defination
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--

