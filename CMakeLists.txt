cmake_minimum_required(VERSION 3.14)
project(transinterpreter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    main.cpp
    FileReader.cpp
    LexicalAnalyzer.cpp
    SyntacticalAnalyzer.cpp
    RPNTranslator.cpp
    RPNInterpreter.cpp
)

add_executable(transinterpreter ${SOURCES})
