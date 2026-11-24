cmake_minimum_required(VERSION 3.20)
project(KuroCat LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(KuroCat WIN32
    src/main.cpp
    app.rc
)

target_include_directories(KuroCat PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(KuroCat PRIVATE WINVER=0x0A00 _WIN32_WINNT=0x0A00)
if(MSVC)
    target_compile_options(KuroCat PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/O2;/utf-8;/EHsc;/W3;/MT>")
    target_link_options(KuroCat PRIVATE /MANIFEST:NO)
else()
    target_compile_options(KuroCat PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-O2;-municode>")
    target_link_options(KuroCat PRIVATE -municode -static)
endif()

target_link_libraries(KuroCat PRIVATE user32 gdi32 gdiplus shell32 advapi32 ole32)
