#pragma once
#include <string>

// Статический класс для чтения исходных файлов
class FileReader {
public:
    // Считывает файл и удаляет символы \t и \r
    static std::string read(const std::string& filePath);
};
