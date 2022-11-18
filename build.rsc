[1] # Version number

configurations
    Debug
    Release

project swordsmangame
    type "ConsoleApp"
    outputdir "run_tree"
    objdir "run_tree"
    outputname "%{ProjectName}"
    
    staticruntime "on"

    pchheader "pch.h"
    pchsource "src/pch.cpp"

    defines {
        # UNICODE
        # _UNICODE
    }

    cfiles {
        src/main.cpp
        src/general.cpp
        src/os_win32.cpp
        src/render_d3d11.cpp
        src/bitmap.cpp
        src/shader_registry.cpp
        src/texture_registry.cpp
        src/animation_registry.cpp
        src/font.cpp
        src/draw_help.cpp
        src/hud.cpp
        src/entity_manager.cpp
        src/entities.cpp
        src/text_file_handler.cpp
        src/animation.cpp
        src/main_menu.cpp
        src/camera.cpp
        src/cursor.cpp
        src/keymap.cpp
        src/variable_service.cpp
        src/editor.cpp
    }

    includedirs {
        external/include
        src/
    }

    libdirs {
        external/lib
    }

    libs {
        user32.lib
        gdi32.lib
        d3d11.lib
        dxgi.lib
        d3dcompiler.lib
        freetype.lib
    }

    filter "configuration:Debug"
        debugsymbols "on"
        optimize "off"
        runtime "Debug"
        defines {
            _DEBUG
            DEBUG
        }

    filter "configuration:Release"
        debugsymbols "off"
        optimize "on"
        runtime "Release"
        defines {
            NDEBUG
            RELEASE
        }