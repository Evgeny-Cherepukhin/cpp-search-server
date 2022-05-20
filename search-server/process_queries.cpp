//��������� ������� ���������. �������� ������ 8 �����.
#include "process_queries.h"

//���������� ������� ProcessQueries.
std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    //������ ������ result �������� queries.size().
    std::vector<std::vector<Document>> result(queries.size());
    //��������� transform, ������� ���������� � result ���������
    //������ FindTopDocuments ��� ������� querie
    std::transform(std::execution::par,
        queries.begin(), queries.end(),
        result.begin(),
        [&search_server](const std::string& querie) { return search_server.FindTopDocuments(querie); }
    );
    //���������� ��������� ������ �������
    return result;
}

//���������� ������� ProcessQueriesJoined.
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
