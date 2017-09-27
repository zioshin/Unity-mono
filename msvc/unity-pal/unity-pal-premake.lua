-- unity-pal-premake.lua
workspace "unity-pal"
   configurations { "Debug", "Release" }

project "unity-pal"
   kind "StaticLib"
   language "C++"
   targetdir "../build/unity-pal/bin/%{cfg.buildcfg}"

   local libIl2cppDir = "../../../../il2cpp/build/libil2cpp";

   includedirs { libIl2cppDir, libIl2cppDir .. "/os/c-api" }

   files {  libIl2cppDir .. "/os/**.h", libIl2cppDir .. "/os/**.cpp" }
   excludes { libIl2cppDir .. "**COM.cpp",
   "../Generic/WindowsRuntime.cpp",
   "../Messages.cpp"
   }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"