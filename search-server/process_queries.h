//Черепухин Евгений Сергеевич. Итоговый проект 8 сплит.
#pragma once

#include <execution>
#include <algorithm>
#include <string>
#include <numeric>
#include <list>

#include "search_server.h"

//Объявляем функцию ProcessQueries, распараллеливающую обработку
//нескольких запрсов к поисковой системе
std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries);

//Объявляем функцию ProcessQueriesJoined, распараллеливающую обработку
//нескольких запросов к поисковой системе. Функция возвращает
//набор документов в плоском виде
std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries);
