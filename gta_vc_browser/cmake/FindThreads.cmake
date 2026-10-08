# Shim Emscripten: pthreads se activan con -pthread (equivale a -sUSE_PTHREADS=1).
# Evita que FindThreads nativo falle bajo emcc.
# Con REVC_NO_THREADS=ON (build monohilo on-demand) no se añade -pthread.
if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "FindThreads shim solo valido bajo Emscripten")
endif()

option(REVC_NO_THREADS "Build web monohilo (sin pthreads)" OFF)

set(THREADS_FOUND TRUE)
set(Threads_FOUND TRUE)
set(THREADS_PREFER_PTHREAD_FLAG ON)

if(REVC_NO_THREADS)
  set(CMAKE_THREAD_LIBS_INIT "")
  set(CMAKE_USE_PTHREADS_INIT 0)
else()
  set(CMAKE_THREAD_LIBS_INIT "-pthread")
  set(CMAKE_USE_PTHREADS_INIT 1)
endif()

if(NOT TARGET Threads::Threads)
  add_library(Threads__Threads INTERFACE IMPORTED)
  add_library(Threads::Threads ALIAS Threads__Threads)
  if(NOT REVC_NO_THREADS)
    target_compile_options(Threads__Threads INTERFACE "-pthread")
    target_link_options(Threads__Threads INTERFACE "-pthread")
  endif()
endif()
