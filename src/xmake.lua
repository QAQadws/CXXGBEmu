target("GBemu")
    set_kind("binary")
    add_files("*.cpp")
    add_packages("libsdl3")
    add_includedirs("include")
    set_targetdir("$(projectdir)")  -- 设置输出到项目根目录
    
