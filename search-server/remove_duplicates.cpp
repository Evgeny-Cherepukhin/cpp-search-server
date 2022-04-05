//Final project of 5-th split. Cherepukhin Evgeny
#include"remove_duplicates.h"

using namespace std;

// Не смог разобраться как применить std::transform
void RemoveDuplicates(SearchServer& search_server) {
    vector<int> del;
    std::set<std::set<std::string>> unique;
    for (const int document_id : search_server) {
         auto words = search_server.GetWordFrequencies(document_id);
         set<string> doc_words;
         for (const auto& word : words) {
             doc_words.insert(word.first);
         }         
         if (!unique.count(doc_words)) {
             unique.insert(doc_words);
         }
         else {
             del.push_back(document_id);
         }
    }
    for (auto document_id : del) {
        search_server.RemoveDocument(document_id);
    }
}