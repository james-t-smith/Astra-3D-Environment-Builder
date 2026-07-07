cmake_minimum_required(VERSION 3.16)
project(AstraStreamer LANGUAGES C)
 
set(_projname "AstraStreamer")
 
set(ASTRA_SDK_DIR    "${CMAKE_SOURCE_DIR}/third_party/AstraSDK")
set(ASTRA_BIN_DIR    "${ASTRA_SDK_DIR}/bin")
set(ASTRA_LIB_DIR    "${ASTRA_SDK_DIR}/lib")
set(ASTRA_INCLUDE_DIR "${ASTRA_SDK_DIR}/include")
 
# ── ZMQ via vcpkg ──────────────────────────────────────────────────────────
# vcpkg installs ZeroMQ under the name "ZeroMQ" (capital Z, capital MQ)
find_package(ZeroMQ CONFIG REQUIRED)
 
# ── Sources ────────────────────────────────────────────────────────────────
set(${_projname}_SOURCES
    src/main.c
)
 
add_executable(${_projname} ${${_projname}_SOURCES})
 
# ── Include dirs ───────────────────────────────────────────────────────────
target_include_directories(${_projname} PRIVATE
    ${ASTRA_INCLUDE_DIR}
    ${ASTRA_SDK_DIR}/samples/common
)
 
# ── Link libraries ─────────────────────────────────────────────────────────
target_link_libraries(${_projname} PRIVATE
    "${ASTRA_LIB_DIR}/astra_core_api.lib"
    "${ASTRA_LIB_DIR}/astra_core.lib"
    "${ASTRA_LIB_DIR}/astra.lib"
    libzmq          # vcpkg ZeroMQ target (dynamic)
)
 
# ── Copy Astra runtime DLLs next to the .exe ──────────────────────────────
add_custom_command(TARGET ${_projname} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${ASTRA_BIN_DIR}
        $<TARGET_FILE_DIR:${_projname}>
)