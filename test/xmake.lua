-- 定义测试目标
target("GameBoy_test")
    -- 设置目标类型为可执行文件
    set_kind("binary")
    
    -- 添加测试源文件
    add_files("*.cpp")
    
    -- 添加依赖包，只需要基本的 gtest 库
    add_packages("gtest")
    
    -- 如果需要测试 SDL3 相关功能，可以添加 SDL3 包
    -- add_packages("libsdl3", "libsdl3_ttf")
    
    -- 添加源代码头文件路径（用于测试源代码中的类和函数）
    add_includedirs("../src")
    
    -- 如果需要链接源代码文件（排除 main.cpp 避免多个入口点）
    -- add_files("../src/*.cpp")
    -- remove_files("../src/main.cpp")
    
    -- 设置测试组和属性
    set_group("test")                    -- 归类到测试组
    set_default(false)                   -- 默认不构建，需要显式指定
    set_rundir("$(projectdir)")          -- 设置运行目录为项目根目录
    add_tests("default")
    
    -- 调试模式设置
    if is_mode("debug") then
        add_defines("DEBUG")
        set_symbols("debug")
        set_optimize("none")
    end
    
    -- 平台特定配置
    if is_plat("windows") then
        add_defines("WIN32")
    end