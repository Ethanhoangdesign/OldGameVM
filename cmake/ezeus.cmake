if(EZEUS_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "BUILD_EZEUS=ON requires -DEZEUS_SOURCE_DIR=/path/to/eZeus")
endif()
if(NOT IS_DIRECTORY "${EZEUS_SOURCE_DIR}")
    message(FATAL_ERROR "EZEUS_SOURCE_DIR does not exist: ${EZEUS_SOURCE_DIR}")
endif()

include(FetchContent)
set(EZEUS_SDL2_LIBRARY "${SDL2_LIBRARY}")
set(SDL2_DIR "${CMAKE_BINARY_DIR}/lib-sdl2/lib/cmake/SDL2" CACHE PATH "" FORCE)

if(ANDROID)
    set(SDL2_SHARED ON)
    set(BUILD_SHARED_LIBS ON)
else()
    set(BUILD_SHARED_LIBS OFF)
endif()
set(SDL2TTF_VENDORED ON CACHE BOOL "" FORCE)
set(SDL2TTF_HARFBUZZ OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_VENDORED ON CACHE BOOL "" FORCE)
set(SDL2IMAGE_AVIF OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_JXL OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_TIF OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_WEBP OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_VENDORED ON CACHE BOOL "" FORCE)
set(SDL2MIXER_FLAC OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_MIDI OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_MOD OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_OPUS OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_WAVPACK OFF CACHE BOOL "" FORCE)
set(SDL2TTF_BUILD_SHARED_LIBS ${ANDROID} CACHE BOOL "" FORCE)
set(SDL2IMAGE_BUILD_SHARED_LIBS ${ANDROID} CACHE BOOL "" FORCE)
set(SDL2MIXER_BUILD_SHARED_LIBS ${ANDROID} CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL2_ttf
    URL https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.24.0/SDL2_ttf-2.24.0.tar.gz
    URL_HASH SHA256=0b2bf1e7b6568adbdbc9bb924643f79d9dedafe061fa1ed687d1d9ac4e453bfd
)
FetchContent_Declare(
    SDL2_image
    URL https://github.com/libsdl-org/SDL_image/releases/download/release-2.8.12/SDL2_image-2.8.12.tar.gz
    URL_HASH SHA256=393f5efb50536ec13ca4f4affb69cc9966d3c3f969e6c5e701faddf9f9785381
)
FetchContent_Declare(
    SDL2_mixer
    URL https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.8.2/SDL2_mixer-2.8.2.tar.gz
    URL_HASH SHA256=938dff531d00ace2296557a6599abe6f34599e2f34f0a4a08a397e2ccac8b8f7
)
FetchContent_MakeAvailable(SDL2_ttf SDL2_image SDL2_mixer)

# Reuse upstream's explicit source manifest. Parsing it avoids a glob that could
# silently pull tests or a second main() into the Android library.
file(READ "${EZEUS_SOURCE_DIR}/CMakeLists.txt" EZEUS_CMAKE)
string(REGEX MATCH "add_executable\\(eZeus([^\\)]*)\\)" EZEUS_MANIFEST "${EZEUS_CMAKE}")
string(REGEX MATCHALL "[A-Za-z0-9_./-]+\\.cpp" EZEUS_RELATIVE_SOURCES "${EZEUS_MANIFEST}")
if(NOT EZEUS_RELATIVE_SOURCES)
    message(FATAL_ERROR "Could not read the eZeus source manifest")
endif()

set(EZEUS_EXCLUDED_SOURCES
    main.cpp
    egamedir.cpp
    engine/emapgenerator.cpp
    widgets/eboardsettingsmenu.cpp
)
set(EZEUS_SOURCES)
foreach(SOURCE IN LISTS EZEUS_RELATIVE_SOURCES)
    if(NOT SOURCE IN_LIST EZEUS_EXCLUDED_SOURCES)
        list(APPEND EZEUS_SOURCES "${EZEUS_SOURCE_DIR}/${SOURCE}")
    endif()
endforeach()

# ponytail: Android map generation stays disabled until libnoise has a native
# target; saved games and bundled adventures do not need it for startup.
set(EZEUS_OVERLAY_DIR "${CMAKE_CURRENT_BINARY_DIR}/ezeus-overlay")
foreach(SOURCE engine/emapgenerator.cpp widgets/eboardsettingsmenu.cpp)
    get_filename_component(SOURCE_DIR "${SOURCE}" DIRECTORY)
    file(MAKE_DIRECTORY "${EZEUS_OVERLAY_DIR}/${SOURCE_DIR}")
    file(READ "${EZEUS_SOURCE_DIR}/${SOURCE}" SOURCE_CONTENT)
    string(REPLACE "#ifdef __unix__" "#if defined(__unix__) && !defined(__ANDROID__)" SOURCE_CONTENT "${SOURCE_CONTENT}")
    file(WRITE "${EZEUS_OVERLAY_DIR}/${SOURCE}" "${SOURCE_CONTENT}")
    list(APPEND EZEUS_SOURCES "${EZEUS_OVERLAY_DIR}/${SOURCE}")
endforeach()
file(MAKE_DIRECTORY "${EZEUS_OVERLAY_DIR}/engine")
file(READ "${EZEUS_SOURCE_DIR}/engine/emapgenerator.h" EZEUS_MAP_GENERATOR_HEADER)
string(REPLACE "#ifdef __unix__" "#if defined(__unix__) && !defined(__ANDROID__)" EZEUS_MAP_GENERATOR_HEADER "${EZEUS_MAP_GENERATOR_HEADER}")
file(WRITE "${EZEUS_OVERLAY_DIR}/engine/emapgenerator.h" "${EZEUS_MAP_GENERATOR_HEADER}")

add_library(ezeus SHARED
    ${EZEUS_SOURCES}
    "${CMAKE_SOURCE_DIR}/android/ezeus/ezeus_android_main.cpp"
    "${CMAKE_SOURCE_DIR}/android/ezeus/egamedir.cpp"
)

target_compile_features(ezeus PRIVATE cxx_std_17)
target_compile_options(ezeus PRIVATE "SHELL:-include vector" "SHELL:-include cmath")
target_include_directories(ezeus PRIVATE
    "${EZEUS_OVERLAY_DIR}"
    "${EZEUS_SOURCE_DIR}"
    "${EZEUS_SOURCE_DIR}/engine"
    "${EZEUS_SOURCE_DIR}/widgets"
    "${SDL2_INCLUDE_DIR}"
    "${sdl2_ttf_SOURCE_DIR}"
    "${sdl2_image_SOURCE_DIR}/include"
    "${sdl2_mixer_SOURCE_DIR}/include"
)
# eZeus includes every SDL header as SDL2/SDL_*.h.
file(MAKE_DIRECTORY "${EZEUS_OVERLAY_DIR}/SDL2")
file(GLOB SDL2_HEADERS "${SDL2_INCLUDE_DIR}/*.h")
foreach(HEADER_SOURCE IN LISTS SDL2_HEADERS)
    get_filename_component(HEADER "${HEADER_SOURCE}" NAME)
    configure_file("${HEADER_SOURCE}" "${EZEUS_OVERLAY_DIR}/SDL2/${HEADER}" COPYONLY)
endforeach()
foreach(HEADER SDL_ttf.h SDL_image.h SDL_mixer.h)
    if(HEADER STREQUAL "SDL_ttf.h")
        set(HEADER_SOURCE "${sdl2_ttf_SOURCE_DIR}/${HEADER}")
    elseif(HEADER STREQUAL "SDL_image.h")
        set(HEADER_SOURCE "${sdl2_image_SOURCE_DIR}/include/${HEADER}")
    else()
        set(HEADER_SOURCE "${sdl2_mixer_SOURCE_DIR}/include/${HEADER}")
    endif()
    configure_file("${HEADER_SOURCE}" "${EZEUS_OVERLAY_DIR}/SDL2/${HEADER}" COPYONLY)
endforeach()
target_link_libraries(ezeus PRIVATE
    ${EZEUS_SDL2_LIBRARY}
    SDL2_ttf
    SDL2_image
    SDL2_mixer
)

if(ANDROID)
    target_link_libraries(ezeus PRIVATE android log)
endif()

set_target_properties(ezeus PROPERTIES OUTPUT_NAME ezeus)
message(STATUS "eZeus full source: ${EZEUS_SOURCE_DIR}")
