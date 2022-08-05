//Черепухин Евгений Сергеевич. Итоговый проект 8 спринт.
#pragma once
#include <algorithm>
#include <cstdlib>
#include <execution>
#include <map>
#include <mutex>
#include <set>
#include <vector>
#include <future>
#include <numeric>
#include <random>
#include <string>

using namespace std::string_literals;

//Создадим структуру Mutex_Map, в которой будем хранить словарь и mutex.
template <typename Key, typename Value>
struct Mutex_Map {
    std::map<Key, Value> data;
    std::mutex mutex_data;
};

template <typename Key, typename Value>
class ConcurrentMap {
public:
//static_assert не даёт программе скомпилироваться при попытке использовать в качестве ключа
//что-либо, кроме целых чисел
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

//В структуру Access дополнительно добавим поле lock_guard, для блокировки mutex. 
//Структура предоставляет ссылку на значение словаря и обеспечивает синхронизацию
//доступа к нему.
    struct Access {
        std::lock_guard<std::mutex> value_guard;
        Value& ref_to_value;
    };
    //Объявляем конструктор, оператор [], методы Erase и BuildOrdinaryMap
    explicit ConcurrentMap(size_t bucket_count = 1);

    Access operator[](const Key& key);

    auto Erase(const Key& key);

    std::map<Key, Value> BuildOrdinaryMap();

private:
//В приватной части разместим вектор структуры Mutex_Map
    std::vector<Mutex_Map<Key, Value>> mutex_maps_;
};

//Конструктор класса принимает количество подсловарей, на которое нужно разбить 
//всё пространство ключей.
template <typename Key, typename Value>
ConcurrentMap<Key, Value>:: ConcurrentMap(size_t bucket_count)
        : mutex_maps_(bucket_count) {}

//operator[] возвращает объект класса Access, содержащий ссылку на соответствующее значение
template <typename Key, typename Value>
typename ConcurrentMap<Key, Value>::Access ConcurrentMap<Key, Value>::operator[](const Key& key) {
        uint64_t tmp_key = static_cast<uint64_t>(key) % mutex_maps_.size();
        return { std::lock_guard(mutex_maps_[tmp_key].mutex_data), mutex_maps_[tmp_key].data[key] };
}

//Метод Erase удаляет элемент из словаря по ключу.
template <typename Key, typename Value>
auto ConcurrentMap<Key, Value>::Erase(const Key& key) {
    uint64_t tmp_key = static_cast<uint64_t>(key) % mutex_maps_.size();
    std::lock_guard guard(mutex_maps_[tmp_key].mutex_data);
    return mutex_maps_[tmp_key].data.erase(key);
}

//Метод BuildOrdinaryMap() сливает вместе части словаря и возвращает весь словарь целиком.
//Потокобезопасен, т.е. работает корректно, когда другие потоки выполняют операции с ConcurrentMap.
template <typename Key, typename Value>
std::map<Key, Value> ConcurrentMap<Key, Value>:: BuildOrdinaryMap() {
    std::map<Key, Value> main_map;
    std::for_each(mutex_maps_.begin(), mutex_maps_.end(),
        [&main_map](auto& part_map) {
            std::lock_guard<std::mutex> guard(part_map.mutex_data);
            main_map.insert(part_map.data.begin(), part_map.data.end());
        });
    return main_map;
}