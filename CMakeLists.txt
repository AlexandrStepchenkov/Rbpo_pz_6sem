cmake_minimum_required(VERSION 3.20)

project(Rbpo_pz_6sem LANGUAGES CXX RC)

if(NOT CMAKE_GENERATOR_PLATFORM)
    set(CMAKE_GENERATOR_PLATFORM x64)
endif()

if(MSVC)
    add_compile_options(/W4 /WX- /EHsc)
endif()

set(RBPO_COMMON_INCLUDES
    Rbpo_pz_6sem
    Rbpo_pz_6sem/common
    Rbpo_pz_6sem/rpc
)

add_executable(Rbpo_pz_6sem WIN32
    Rbpo_pz_6sem/Rbpo_pz_6sem.cpp
    Rbpo_pz_6sem/Rbpo_pz_6sem.h
    Rbpo_pz_6sem/framework.h
    Rbpo_pz_6sem/targetver.h
    Rbpo_pz_6sem/Resource.h
    Rbpo_pz_6sem/Rbpo_pz_6sem.rc
)

target_include_directories(Rbpo_pz_6sem PRIVATE ${RBPO_COMMON_INCLUDES})
target_compile_features(Rbpo_pz_6sem PRIVATE cxx_std_20)

target_compile_definitions(Rbpo_pz_6sem PRIVATE
    _WINDOWS
    UNICODE
    _UNICODE
)

target_compile_options(Rbpo_pz_6sem PRIVATE /Oy-)

target_link_libraries(Rbpo_pz_6sem PRIVATE
    user32
    shell32
    gdi32
    comctl32
    rpcrt4
    advapi32
)

add_executable(RbpoPz6SemService
    Rbpo_pz_6sem/service/RbpoPzService.cpp
)

target_include_directories(RbpoPz6SemService PRIVATE ${RBPO_COMMON_INCLUDES})
target_compile_features(RbpoPz6SemService PRIVATE cxx_std_20)

target_compile_definitions(RbpoPz6SemService PRIVATE
    UNICODE
    _UNICODE
)

target_compile_options(RbpoPz6SemService PRIVATE /Oy-)

target_link_libraries(RbpoPz6SemService PRIVATE
    rpcrt4
    wtsapi32
    userenv
    advapi32
    winhttp
    iphlpapi
    crypt32
)

set(RBPO_DATA_DIR ${CMAKE_SOURCE_DIR}/Rbpo_pz_6sem/data)
foreach(target_name IN ITEMS Rbpo_pz_6sem RbpoPz6SemService)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>/data
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${RBPO_DATA_DIR}/default.avdb
            $<TARGET_FILE_DIR:${target_name}>/data/default.avdb
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${RBPO_DATA_DIR}/public_cert.pem
            $<TARGET_FILE_DIR:${target_name}>/data/public_cert.pem
    )
endforeach()
