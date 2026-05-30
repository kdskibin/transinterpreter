cmake_minimum_required(VERSION 3.14)
project(transinterpreter LANGUAGES CXX)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    main.cpp
    FileReader.cpp
    LexicalAnalyzer.cpp
    LL1parser.cpp
    Interpreter.cpp
)

add_executable(transinterpreter ${SOURCES})