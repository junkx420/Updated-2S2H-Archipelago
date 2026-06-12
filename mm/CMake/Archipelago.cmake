# ==============================================================================
# Archipelago networking dependencies (apclientpp + wswrap stack)
#
# Adapted from Shipwright (SoH) `archipelago` branch, soh/CMakeLists.txt.
# Include this from mm/CMakeLists.txt AFTER add_executable(${PROJECT_NAME} ...)
# and after the main target_include_directories(...) block, e.g.:
#
#   include(${CMAKE_CURRENT_SOURCE_DIR}/CMake/Archipelago.cmake)
#
# Requires OpenSSL on the system:
#   - Linux:   libssl-dev (deb) / openssl-devel (rpm)
#   - macOS:   brew install openssl@3
#   - Windows: vcpkg install openssl:x64-windows-static (or match your triplet)
# ==============================================================================

option(BUILD_ARCHIPELAGO "Build with Archipelago multiworld support" ON)

if (BUILD_ARCHIPELAGO)
    include(FetchContent)

    # asio is used standalone (no boost), both by wswrap and apclientpp.
    # _WEBSOCKETPP_CPP11_STL_ makes websocketpp use the C++11 STL instead of
    # falling back to boost (random.hpp etc. #include boost headers otherwise).
    # Target-scoped on purpose: this module is include()d after
    # add_executable, so directory-scoped defines are unreliable here.
    target_compile_definitions(${PROJECT_NAME} PRIVATE ASIO_STANDALONE _WEBSOCKETPP_CPP11_STL_)

    # CRITICAL (Windows): 2S2H force-includes a precompiled header into every
    # C++ TU (mm/CMakeLists.txt: target_precompile_headers). That PCH pulls in
    # Windows.h -> WinSock.h (v1) before the first line of our code, which
    # makes asio fail with "WinSock.h has already been included" — asio
    # requires WinSock2 to come first. Opt the apclientpp TU out of the PCH
    # (same idiom 2S2H already uses for 2s2h/mixer.c) so its own
    # WIN32_LEAN_AND_MEAN/winsock2 prologue is actually first.
    set_source_files_properties(2s2h/Network/Archipelago/Archipelago.cpp
        PROPERTIES SKIP_PRECOMPILE_HEADERS ON
    )
    if (MSVC)
        # The whole apclientpp/asio/websocketpp template stack lives in this
        # one TU and overflows the default COFF section limit (C1128).
        set_property(SOURCE 2s2h/Network/Archipelago/Archipelago.cpp
            APPEND PROPERTY COMPILE_OPTIONS /bigobj
        )
    endif()

    # --------------------------------------------------------------------------
    # valijson — JSON-schema validation, required by apclientpp's DataPackage
    # handling
    # --------------------------------------------------------------------------
    FetchContent_Declare(
        valijson
        GIT_REPOSITORY https://github.com/tristanpenman/valijson.git
        GIT_TAG v1.0.6
    )
    FetchContent_MakeAvailable(valijson)
    target_include_directories(${PROJECT_NAME} PRIVATE ${valijson_SOURCE_DIR}/include)

    # --------------------------------------------------------------------------
    # asio — networking backbone for websocketpp
    # --------------------------------------------------------------------------
    FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-30-2
    )
    FetchContent_MakeAvailable(asio)
    target_include_directories(${PROJECT_NAME} PRIVATE ${asio_SOURCE_DIR}/asio/include)

    # --------------------------------------------------------------------------
    # websocketpp — header-only; its own CMakeLists is ancient, so we only
    # download it and add the include path ourselves (same approach as SoH).
    # FetchContent_Populate is deprecated as of CMake 3.30; allow the old
    # behavior explicitly.
    # --------------------------------------------------------------------------
    if (POLICY CMP0169)
        cmake_policy(SET CMP0169 OLD)
    endif()
    FetchContent_Declare(
        websocketpp
        GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
        GIT_TAG bce258674b2d544f981da4bf149244a76e8126c9
    )
    FetchContent_GetProperties(websocketpp)
    if (NOT websocketpp_POPULATED)
        FetchContent_Populate(websocketpp)
    endif()
    target_include_directories(${PROJECT_NAME} PRIVATE ${websocketpp_SOURCE_DIR})

    # --------------------------------------------------------------------------
    # wswrap — black-sliver's thin websocket abstraction used by apclientpp
    # --------------------------------------------------------------------------
    FetchContent_Declare(
        wswrap
        GIT_REPOSITORY https://github.com/black-sliver/wswrap.git
        GIT_TAG d0505e0ec53a26743f11051949a0dc66bcf44951
    )
    FetchContent_MakeAvailable(wswrap)
    target_include_directories(${PROJECT_NAME} PRIVATE ${wswrap_SOURCE_DIR}/include)

    # --------------------------------------------------------------------------
    # apclientpp — the Archipelago client protocol implementation (header-only)
    # --------------------------------------------------------------------------
    FetchContent_Declare(
        apclientpp
        GIT_REPOSITORY https://github.com/black-sliver/apclientpp.git
        GIT_TAG 7f33a3849887983378258c2fe8fc3887f687c430
    )
    set(APCLIENTPP_BUILD_TESTING OFF CACHE BOOL "Don't build apclientpp's tests")
    FetchContent_MakeAvailable(apclientpp)
    target_include_directories(${PROJECT_NAME} PRIVATE ${apclientpp_SOURCE_DIR})

    # --------------------------------------------------------------------------
    # OpenSSL — needed for wss:// (archipelago.gg uses TLS)
    # --------------------------------------------------------------------------
    set(OPENSSL_USE_STATIC_LIBS ON)
    if (MSVC)
        # 2S2H builds with a static CRT (/MT, /MTd — see mm/CMakeLists.txt
        # MSVC_RUNTIME_LIBRARY). Tell FindOpenSSL to pick the lib/VC/x64/MT*
        # flavors of the slproweb OpenSSL install; the default MD* ones
        # cause LNK2038 RuntimeLibrary mismatches.
        set(OPENSSL_MSVC_STATIC_RT TRUE)
    endif()
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Some distros (notably Fedora) don't ship static OpenSSL libs
        file(STRINGS /etc/os-release distro REGEX "^NAME=")
        string(REGEX REPLACE "NAME=\"(.*)\"" "\\1" distro "${distro}")
        if (${distro} MATCHES "Fedora Linux")
            set(OPENSSL_USE_STATIC_LIBS OFF)
        endif()
    endif()
    find_package(OpenSSL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)

    # CA bundle so TLS cert validation works out of the box on every platform.
    # Installed next to the executable as networking/cacert.pem; the client
    # locates it via Ship::Context::LocateFileAcrossAppDirs.
    FetchContent_Declare(
        sslCertStore
        URL https://curl.se/ca/cacert.pem
        DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}/networking
        DOWNLOAD_NO_EXTRACT TRUE
    )
    FetchContent_MakeAvailable(sslCertStore)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/networking/cacert.pem
            DESTINATION ./networking COMPONENT 2s2h)

    # Gate all AP code behind this define so BUILD_ARCHIPELAGO=OFF still builds.
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_ARCHIPELAGO)
endif()
