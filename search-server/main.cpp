//Final project of 4-th split. Cherepukhin Evgeny
#include<string>
#include<iostream>
#include"search_server.h"
#include"request_queue.h"
#include"paginator.h"
#include"document.h"
#include"read_input_functions.h"
#include"string_processing.h"


using namespace std;
int main() {
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail", DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "curly dog and fancy collar", DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat fancy collar ", DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog sparrow Eugene", DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog sparrow Vasiliy", DocumentStatus::ACTUAL, { 1, 1, 1 });

   
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request");
    }
    
    request_queue.AddFindRequest("curly dog");
    
    request_queue.AddFindRequest("big collar");
    
    request_queue.AddFindRequest("sparrow");
    std::cout << "Total empty requests: " << request_queue.GetNoResultRequests() << std::endl;
    return 0;
}