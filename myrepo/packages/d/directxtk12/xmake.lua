package("directxtk12")

    set_homepage("https://github.com/microsoft/DirectXTK12")
    set_description("This package contains the \"DirectX Tool Kit\", a collection of helper classes for writing Direct3D 12 C++ code for Universal Windows Platform (UWP) apps for Windows 11 and Windows 10, game titles for Xbox Series X|S and Xbox One, and Win32 desktop applications for Windows 11 and Windows 10.")

    set_urls("https://github.com/microsoft/DirectXTK12/archive/$(version).zip",
             "https://github.com/microsoft/DirectXTK.git",
            {version = function(version)
                local versions = {
                    ["23.02.0"] = "feb2023",
                    ["22.12.0"] = "dec2022",
                    ["22.10.0"] = "oct2022",
                }
                return versions[tostring(version)];
            end})

    add_versions("23.02.0", "f1ed66f02ce9aeb2722015f11ddb06bf95a67475c8e68308ae9aeb3260b1bef8")
    add_versions("22.12.0", "abb3a7eea95ede901e9d13430a0a6d7a220927c6664cd6b938f97f568bca24bb")
    add_versions("22.10.0", "9ccae858a19eff3f250c4b9af96bf0d09c571f6accfe35016df9af9c4a7a5412")

    on_install("windows", function (package)
        local configs = {}
        local vs_sdkver = import("core.tool.toolchain").load("msvc"):config("vs_sdkver")
        if vs_sdkver then
            local build_ver = string.match(vs_sdkver, "%d+%.%d+%.(%d+)%.?%d*")
            assert(tonumber(build_ver) >= 19041, "DirectXTK12 requires Windows SDK to be at least 10.0.19041.0")
            table.insert(configs, "-DCMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION=" .. vs_sdkver)
            table.insert(configs, "-DCMAKE_SYSTEM_VERSION=" .. vs_sdkver)
        end
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        -- fix path issue with spaces
        io.replace("Src/Shaders/CompileShaders.cmd", " %1.hlsl ", " \"%1.hlsl\" ", {plain = true})
        io.replace("Src/Shaders/CompileShaders.cmd", " %1.fx ", " \"%1.fx\" ", {plain = true})
        import("package.tools.cmake").install(package, configs)
        os.cp("Inc/*", package:installdir("include"))
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            void test()
            {
                DirectX::SimpleMath::Vector3 eye(0.0f, 0.7f, 1.5f);
                DirectX::SimpleMath::Vector3 at(0.0f, -0.1f, 0.0f);
                auto lookAt = DirectX::SimpleMath::Matrix::CreateLookAt(eye, at, DirectX::SimpleMath::Vector3::UnitY);
            }
        ]]}, {configs = {languages = "c++11"}, includes = { "windows.h", "SimpleMath.h" } }))
    end)
