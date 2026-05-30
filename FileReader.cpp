#include "FileReader.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

std::string FileReader::read(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open())
        throw std::runtime_error("Не удалось открыть файл: " + filePath);

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string text = ss.str();

    // Удаляем символы табуляции и возврата каретки
    text.erase(std::remove(text.begin(), text.end(), '\t'), text.end());
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());

    return text;
}