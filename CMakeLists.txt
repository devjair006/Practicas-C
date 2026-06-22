cmake_minimum_required(VERSION 3.10)
project(PracticasOpenGL VERSION 1.0 LANGUAGES C CXX)

# Estándar de C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(NOMINMAX)

# Directorios de inclusión
include_directories(include include/imgui include/imgui/backends vcpkg/installed/x64-windows/include)

# Buscar GLFW3
if(WIN32)
    set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glfw-win64/glfw-3.4.bin.WIN64")
    include_directories("${GLFW_DIR}/include")
    link_directories("${GLFW_DIR}/lib-vc2022")
    set(GLFW_LIBRARIES glfw3)
    
    # Buscar FreeGLUT para Windows
    set(GLUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/freeglut-MSVC/freeglut")
    include_directories("${GLUT_DIR}/include")
    link_directories("${GLUT_DIR}/lib/x64")
    set(GLUT_LIBRARIES freeglut)

    # Buscar ASSIMP (VCPKG pre-installed locally)
    set(ASSIMP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows")
    include_directories("${ASSIMP_DIR}/include")
    link_directories("${ASSIMP_DIR}/lib")
    set(ASSIMP_LIBRARIES assimp-vc143-mt)
else()
    find_package(glfw3 CONFIG REQUIRED)
    set(GLFW_LIBRARIES glfw)

    # Buscar ASSIMP para macOS/Linux (brew install assimp)
    find_package(assimp REQUIRED)
    # brew exporta assimp como target moderno; ASSIMP_INCLUDE_DIRS no se setea automáticamente
    get_target_property(ASSIMP_INC_DIR assimp::assimp INTERFACE_INCLUDE_DIRECTORIES)
    include_directories(BEFORE ${ASSIMP_INC_DIR})
    set(ASSIMP_LIBRARIES assimp::assimp)

    # GLUT no se usa en el código; en macOS usamos los frameworks del sistema
    set(GLUT_LIBRARIES "")
endif()

# Fuentes
set(SOURCES
    src/main.cpp
    src/game_state.cpp
    src/gameplay.cpp
    src/localization.cpp
    src/animated_entity.cpp
    src/glad.c
    include/imgui/imgui.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/imgui_widgets.cpp
    include/imgui/backends/imgui_impl_glfw.cpp
    include/imgui/backends/imgui_impl_opengl3.cpp
)

# Ejecutable
add_executable(app ${SOURCES})

# Vincular librerías
if(WIN32)
    target_link_libraries(app ${GLFW_LIBRARIES} ${GLUT_LIBRARIES} ${ASSIMP_LIBRARIES} opengl32 user32 gdi32 shell32)
    
    # --- COPIAR DLLS AUTOMÁTICAMENTE ---
    # Esto evita que tengas que mover los archivos a mano y soluciona errores de ejecución
    add_custom_command(TARGET app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/freeglut.dll"
        $<TARGET_FILE_DIR:app>
    )
    add_custom_command(TARGET app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/glfw3.dll"
        $<TARGET_FILE_DIR:app>
    )
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows/bin")
        file(GLOB VCPKG_DLLS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/*.dll")
        foreach(DLL_FILE ${VCPKG_DLLS})
            add_custom_command(TARGET app POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL_FILE}"
                $<TARGET_FILE_DIR:app>
            )
        endforeach()
    endif()
else()
    target_link_libraries(app ${GLFW_LIBRARIES} ${ASSIMP_LIBRARIES})
endif()

# Configuración específica para macOS
if(APPLE)
    target_link_libraries(app "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
endif()

# Configuración específica para Windows (MSVC)
if(MSVC)
    target_link_libraries(app opengl32.lib)
endif()
