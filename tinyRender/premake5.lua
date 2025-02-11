workspace "MyTinyRender"
    architecture "x64"
    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }
    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
    IncludeDirs = {}

    project "MyTinyRender"
        kind "ConsoleApp"
        language "C++"
        cppdialect "C++17"
        targetdir ("bin/" ..outputdir.. "/%{prj.name}")
        objdir ("bin-int/" ..outputdir.. "/%{prj.name}")

        files
        {
            "src/**.h",
            "src/**.cpp"
        }
        includedirs
        {
            "src"
        }
        filter "system:windows"
            cppdialect "C++17"
            systemversion "latest"
        filter "configurations:Debug"
            symbols "on"
        filter "configurations:Release"
            optimize "on"
        filter "configurations:Dist"
            optimize "on"