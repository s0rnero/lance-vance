# Shim Emscripten: mpg123 es un port oficial (-sUSE_MPG123=1). Hace sombra al
# Findmpg123.cmake de la raiz (nuestro dir va primero en CMAKE_MODULE_PATH).
if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "Findmpg123 shim solo valido bajo Emscripten")
endif()

if(NOT TARGET MPG123::libmpg123)
  add_library(__libmpg123 INTERFACE)
  target_compile_options(__libmpg123 INTERFACE "-sUSE_MPG123=1")
  target_link_options(__libmpg123 INTERFACE "-sUSE_MPG123=1")
  add_library(MPG123::libmpg123 ALIAS __libmpg123)
endif()

set(mpg123_FOUND TRUE)
set(MPG123_FOUND TRUE)
set(mpg123_INCLUDE_DIR "")
set(mpg123_LIBRARIES "")
set(mpg123_CFLAGS "-sUSE_MPG123=1")
