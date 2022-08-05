//Черепухин Евгений Сергеевич. Итоговый проект 8 спринт.
#include<algorithm>

#include"string_processing.h"

using namespace std;

VectorStringView SplitIntoWords(string_view text) {
    VectorStringView words;

/* Разбиваем текст на слова, для чего находим "пробел" и добавляем в вектор
 * слово от 0 символа до пробела.*/
    while (true) {
        const auto space = text.find(' ');
        words.push_back(text.substr(0, space));
        if (space == text.npos) {
            break;
        }
        else {
/* Перемещаем итератор на следующий за пробелом символ.*/
            text.remove_prefix(space + 1);
        }
    }
    return words;
}