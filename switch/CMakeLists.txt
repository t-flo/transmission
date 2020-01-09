project(nxTransmission)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)
set(VERSION_MICRO 1)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CURL_INCLUDE_DIRS}
    ${EVENT2_INCLUDE_DIRS}
)

find_package(LIBNX REQUIRED)
if (NOT LIBNX_FOUND)
    cmake_panic("Unable to detect libnx on this system.")
endif ()

set(${PROJECT_NAME}_SOURCES
    nxtransmission.c
)

set(${PROJECT_NAME}_HEADERS
)

add_executable(${PROJECT_NAME}.elf
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_HEADERS}
    ${${PROJECT_NAME}_WIN32_RC_FILE})

target_link_libraries(${PROJECT_NAME}.elf
    ${TR_NAME}
    ${EVENT2_LIBRARIES}
    ${SYSTEMD_DAEMON_LIBRARIES}
)

# Replace this with the name of your application
set(HOMEBREW_APP ${PROJECT_NAME})

# Meta information about the app
set(APP_TITLE ${PROJECT_NAME})
set(APP_AUTHOR "t-flo")
set(APP_ICON $(CMAKE_SOURCE_DIR)/switch/nxtransmission.jpg)
set(APP_VERSION
        "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_MICRO}")
target_compile_definitions(${PROJECT_NAME}.elf PRIVATE VERSION_STRING="${APP_VERSION}")

include(nx-utils)

cmake_info("Building ${APP_TITLE} version ${APP_VERSION}.")

build_switch_binaries(${HOMEBREW_APP})

add_custom_target(create_release_zip 
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/web ${CMAKE_CURRENT_BINARY_DIR}/nxTransmission/web
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/switch/settings.json ${CMAKE_CURRENT_BINARY_DIR}/nxTransmission/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.nro ${CMAKE_CURRENT_BINARY_DIR}/nxTransmission/
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "nxTransmission-${APP_VERSION}.zip" --format=zip
       "${CMAKE_CURRENT_BINARY_DIR}/nxTransmission")

add_dependencies(create_release_zip ${PROJECT_NAME}.nro)