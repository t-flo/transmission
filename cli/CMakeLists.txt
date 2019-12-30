project(trcli)

include_directories(${CMAKE_SOURCE_DIR})

find_package(LIBNX REQUIRED)
if (NOT LIBNX_FOUND)
    cmake_panic("Unable to detect libnx on this system.")
endif ()

add_executable(${TR_NAME}-cli.elf cli.c ${${PROJECT_NAME}_WIN32_RC_FILE})
include_directories(${TR_NAME}-cli.elf ${CURL_INCLUDE_DIRS})
target_link_libraries(${TR_NAME}-cli.elf ${TR_NAME} switch::libnx)

# Replace this with the name of your application
set(HOMEBREW_APP ${TR_NAME}-cli)

# Meta information about the app
set(APP_TITLE ${PROJECT_NAME})
set(APP_AUTHOR "t-gabor")
# set(APP_ICON "assets/icon.png")

set(VERSION_MAJOR 2)
set(VERSION_MINOR 9)
set(VERSION_MICRO 4)
set(APP_VERSION
        "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_MICRO}")

include(nx-utils)

cmake_info("Building ${APP_TITLE} version ${APP_VERSION}.")

build_switch_binaries(${HOMEBREW_APP})
