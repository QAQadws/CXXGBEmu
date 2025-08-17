add_rules("mode.debug", "mode.release")
includes("@builtin/xpack")
set_project("GameBoy_emulator")
set_version("1.0.0")
set_languages("c99", "cxx20")
set_warnings("all")
add_cxflags("/utf-8", {tools = "msvc"})
includes("src","Modules")

-- xpack("GameBoy_emulator")
--     set_formats("zip", "nsis")
--     set_basename("GameBoy_emulator-source")
--     add_targets("GBemu")
--     set_title("CXXGBEMU")                           -- 安装包标题
--     set_author("adws")                          -- 作者信息
--     set_description("A Game Boy emulator.")        -- 包描述
--     set_homepage("https://github.com/QAQadws/CXXGBEmu")    
--     -- 添加源码文件
--     add_installfiles("src/**.cpp", {prefixdir = "src"})
--     add_installfiles("src/include/**.h", {prefixdir = "src/include"})
--         -- 添加构建文件
--     add_installfiles("xmake.lua", {prefixdir = ""})
--     add_installfiles("src/xmake.lua", {prefixdir = "src"})
--     add_installfiles("Modules/xmake.lua", {prefixdir = "Modules"})
--         -- 添加文档
--     add_installfiles("README.md", {prefixdir = ""})
--     add_installfiles(".gitignore", {prefixdir = ""})
--     add_installfiles("roms/**.gb", {prefixdir = "roms"})
--     after_installcmd(function (package, batchcmds)
--         batchcmds:mkdir(package:installdir("build"))  -- 创建build目录
--         print("GameBoy Emulator source package created!")
--     end)

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
--    -- add macro definition
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

