cmake_minimum_required(VERSION 3.22.1)
project("app")

# Native Kütüphane Adı (JNI tarafından çağrılacak)
set(NATIVE_LIB_NAME "flame_renderer")

# C++ Kaynak Dosyaları
add_library(
        ${NATIVE_LIB_NAME}
        SHARED
        FlameRenderer.cpp
        FlameRenderer.h
)

# OpenGL ES kütüphanelerini bul ve bağla
find_library(log-lib log)
find_library(GLESv3 GLESv3)

target_link_libraries(
        ${NATIVE_LIB_NAME}
        android
        ${GLESv3}
        ${log-lib}
)