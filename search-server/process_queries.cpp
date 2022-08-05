//Черепухин Евгений Сергеевич. Итоговый проект 8 сплит.
#include "process_queries.h"

//Реализация функции ProcessQueries.
std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    //Создаём вектор result размером queries.size().
    std::vector<std::vector<Document>> result(queries.size());
    //Применяем transform, который записывает в result результат
    //вызова FindTopDocuments для запроса querie
    std::transform(std::execution::par,
        queries.begin(), queries.end(),
        result.begin(),
        [&search_server](const std::string& querie) { return search_server.FindTopDocuments(querie); }
    );
    //Возвращаем результат работы функции
    return result;
}

//Реализация функции ProcessQueriesJoined.
std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::list<Document> result;
    for (const auto& local_documents : ProcessQueries(search_server, queries)) {
        for (auto& document : local_documents) {
            result.push_back(document);
        }        
    }
    return result;
}
