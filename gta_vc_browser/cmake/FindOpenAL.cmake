# Shim Emscripten: OpenAL es system lib del SDK (-lopenal). Cabeceras en include del sysroot.
if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "FindOpenAL shim solo valido bajo Emscripten")
endif()

if(NOT TARGET OpenAL::OpenAL)
  add_library(OpenAL__OpenAL INTERFACE IMPORTED)
  add_library(OpenAL::OpenAL ALIAS OpenAL__OpenAL)
  target_link_options(OpenAL__OpenAL INTERFACE "-lopenal")
endif()

set(OPENAL_FOUND TRUE)
set(OPENAL_INCLUDE_DIR "")
set(OPENAL_LIBRARY "openal")
set(OPENAL_LIBRARIES "openal")
set(OPENAL_DEFINITIONS "")
