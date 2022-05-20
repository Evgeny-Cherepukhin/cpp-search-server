//��������� ������� ���������. �������� ������ 8 ������.
#include<algorithm>

#include"string_processing.h"

using namespace std;

VectorStringView SplitIntoWords(string_view text) {
    VectorStringView words;

/* ��������� ����� �� �����, ��� ���� ������� "������" � ��������� � ������
 * ����� �� 0 ������� �� �������.*/
    while (true) {
        const auto space = text.find(' ');
        words.push_back(text.substr(0, space));
        if (space == text.npos) {
            break;
        }
        else {
/* ���������� �������� �� ��������� �� �������� ������.*/
            text.remove_prefix(space + 1);
        }
    }
    return words;
}