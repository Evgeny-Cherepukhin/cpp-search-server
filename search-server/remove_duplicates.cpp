//Final project of 5-th split. Cherepukhin Evgeny
#include"remove_duplicates.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    vector<int> del;
    std::set<std::set<std::string>> uniq;
    for (const int document_id : search_server) {
        auto words = search_server.GetDocumentWords(document_id);
         std::set<std::string> set(words.begin(), words.end());
         if (!uniq.count(set)) {
             uniq.insert(set);
         }
         else {
             del.push_back(document_id);
         }
    }
    for (auto document_id : del) {
        search_server.RemoveDocument(document_id);
    }
}