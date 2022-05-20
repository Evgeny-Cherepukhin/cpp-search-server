//Черепухин Евгений Сергеевич. Итоговый проект 8 спринт.
#pragma once

#include <iostream>
#include <set>
#include <string>
#include <string_view>
#include <vector>

using VectorStringView = std::vector<std::string_view>;

using SetString = std::set<std::string, std::less<>>;

//Создаёт словарь из не пустых строк
template <typename StringContainer>
inline SetString MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    SetString non_empty_strings;
    for (const std::string_view str : strings) {
        if (!str.empty()) {
            non_empty_strings.emplace(str);
        }
    }
    return non_empty_strings;
}
//Разбивает строку на вектор слов
VectorStringView SplitIntoWords(std::string_view text);