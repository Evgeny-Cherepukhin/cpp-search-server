//��������� ������� ���������. �������� ������ 8 �����.
#pragma once

#include <execution>
#include <algorithm>
#include <string>
#include <numeric>
#include <list>

#include "search_server.h"

//��������� ������� ProcessQueries, ������������������ ���������
//���������� ������� � ��������� �������
std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries);

//��������� ������� ProcessQueriesJoined, ������������������ ���������
//���������� �������� � ��������� �������. ������� ����������
//����� ���������� � ������� ����
std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries);
