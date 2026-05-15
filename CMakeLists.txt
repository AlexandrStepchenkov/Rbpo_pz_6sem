cmake_minimum_required(VERSION 3.20)

project(Rbpo_pz_6sem LANGUAGES CXX RC)

# Используем x64 по умолчанию
if(NOT CMAKE_GENERATOR_PLATFORM)
    set(CMAKE_GENERATOR_PLATFORM x64)
endif()

add_executable(Rbpo_pz_6sem WIN32
    Rbpo_pz_6sem/Rbpo_pz_6sem.cpp
    Rbpo_pz_6sem/Rbpo_pz_6sem.h
    Rbpo_pz_6sem/framework.h
    Rbpo_pz_6sem/targetver.h
    Rbpo_pz_6sem/Resource.h
    Rbpo_pz_6sem/Rbpo_pz_6sem.rc
)

target_include_directories(Rbpo_pz_6sem PRIVATE Rbpo_pz_6sem)
target_compile_features(Rbpo_pz_6sem PRIVATE cxx_std_20)

target_compile_definitions(Rbpo_pz_6sem PRIVATE
    _WINDOWS
    UNICODE
    _UNICODE
)

target_compile_options(Rbpo_pz_6sem PRIVATE
    /Wall
    /WX-
    /Oy-
)

target_link_libraries(Rbpo_pz_6sem PRIVATE
    user32
    shell32
    gdi32
    comctl32
)
