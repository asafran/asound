file(GLOB SOURCES
    include/*.h
    src/*.cpp
)

find_package(OpenAL REQUIRED)
find_package(OpusFile CONFIG)
if(NOT TARGET OpusFile::opusfile)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(OpusFile REQUIRED IMPORTED_TARGET opusfile)
  set_target_properties(PkgConfig::OpusFile PROPERTIES IMPORTED_GLOBAL TRUE)
  add_library(OpusFile::opusfile ALIAS PkgConfig::OpusFile)
endif()

add_library(asound ${SOURCES})

target_link_libraries(asound ${OPENAL_LIBRARY})
target_include_directories(asound PRIVATE ${OPENAL_INCLUDE_DIR})
target_include_directories(asound PUBLIC include)

set_target_properties( asound
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}../../lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}../../lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}../../bin"
)

target_link_libraries(asound filesystem tools Journal Qt6::Core OpusFile::opusfile)
