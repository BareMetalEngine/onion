# Inferno Engine v4
# Written by Tomasz Jonarski (Rex Dex)

cmake_minimum_required(VERSION 3.16)

project(onion)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)
set(CMAKE_CONFIGURATION_TYPES "Relase")

include_directories("src/")

list(APPEND FILE_SOURCES "src/codeParser.cpp")
list(APPEND FILE_SOURCES "src/common.cpp")
list(APPEND FILE_SOURCES "src/configuration.cpp")
list(APPEND FILE_SOURCES "src/configurationList.cpp")
list(APPEND FILE_SOURCES "src/configurationInteractive.cpp")
list(APPEND FILE_SOURCES "src/externalLibrary.cpp")
list(APPEND FILE_SOURCES "src/externalLibraryRepository.cpp")
list(APPEND FILE_SOURCES "src/fileGenerator.cpp")
list(APPEND FILE_SOURCES "src/main.cpp")
list(APPEND FILE_SOURCES "src/moduleManifest.cpp")
list(APPEND FILE_SOURCES "src/moduleRepository.cpp")
list(APPEND FILE_SOURCES "src/moduleConfiguration.cpp")
list(APPEND FILE_SOURCES "src/project.cpp")
list(APPEND FILE_SOURCES "src/projectCollection.cpp")
list(APPEND FILE_SOURCES "src/projectManifest.cpp")
list(APPEND FILE_SOURCES "src/solutionGenerator.cpp")
list(APPEND FILE_SOURCES "src/solutionGeneratorCMAKE.cpp")
list(APPEND FILE_SOURCES "src/solutionGeneratorVS.cpp")
list(APPEND FILE_SOURCES "src/toolMake.cpp")
list(APPEND FILE_SOURCES "src/toolReflection.cpp")
list(APPEND FILE_SOURCES "src/toolEmbed.cpp")
list(APPEND FILE_SOURCES "src/toolConfigure.cpp")
list(APPEND FILE_SOURCES "src/toolBuild.cpp")
list(APPEND FILE_SOURCES "src/utils.cpp")

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

add_executable(onion ${FILE_SOURCES})
target_link_libraries(onion ${CURSES_LIBRARIES})

set_target_properties(onion
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin"
)

