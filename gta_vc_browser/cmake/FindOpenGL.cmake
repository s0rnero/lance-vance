# Shim Emscripten: OpenGL = WebGL2 via port GLFW + FULL_ES3. No hay libGL nativa.
# Crea OpenGL::OpenGL para que vendor/librw/src/CMakeLists.txt quede satisfecho.
if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "FindOpenGL shim solo valido bajo Emscripten")
endif()

if(NOT TARGET OpenGL::OpenGL)
  add_library(OpenGL__OpenGL INTERFACE IMPORTED)
  add_library(OpenGL::OpenGL ALIAS OpenGL__OpenGL)
  target_compile_options(OpenGL__OpenGL INTERFACE "-sFULL_ES3=1")
  target_link_options(OpenGL__OpenGL INTERFACE "-sFULL_ES3=1")
endif()

set(OPENGL_FOUND TRUE)
set(OpenGL_FOUND TRUE)
set(OPENGL_INCLUDE_DIR "")
set(OPENGL_LIBRARIES "")
