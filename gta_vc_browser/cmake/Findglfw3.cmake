# Shim Emscripten: GLFW3 llega como port (-sUSE_GLFW=3), no como paquete CMake.
# Solo se usa en builds web (gta_vc_browser/build.* lo antepone en CMAKE_MODULE_PATH).
if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "Findglfw3 shim solo valido bajo Emscripten")
endif()

if(NOT TARGET glfw)
  add_library(glfw INTERFACE IMPORTED)
  target_compile_options(glfw INTERFACE "-sUSE_GLFW=3")
  target_link_options(glfw INTERFACE "-sUSE_GLFW=3")
endif()

set(glfw3_FOUND TRUE)
set(GLFW3_FOUND TRUE)
