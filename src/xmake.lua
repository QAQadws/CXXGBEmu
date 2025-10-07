target("GBemu")
    set_kind("binary")
    add_files("*.cpp")
    add_packages("libsdl3", "asio")
    add_includedirs("include")
    add_syslinks("ws2_32")
    set_targetdir("$(projectdir)")  -- 设置输出到项目根目录
    
