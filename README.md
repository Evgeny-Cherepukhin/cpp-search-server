SearchServer

Проект курса Яндекс Практикума «Разработчик С++»

SearchServer  -  система поиска документов по ключевым словам.

Основные функции:
o	ранжирование результатов поиска по релевантности, рассчитанной методом TF-IDF;
o	обработка стоп-слов (не учитываются поисковой системой и не влияют на результаты поиска);
o	обработка минус слов (документы, содержащие минус-слова, не будут включены в результаты поиска);
o	создание и обработка очереди запросов;
o	удаление дубликатов документов;
o	разделение результатов поиска на страницы;
o	возможность осуществлять работу сервера в многопоточном режиме.

Принцип работы

Конструктор принимает строку со стоп-словами, разделенными пробелами. Вместо строки может принять произвольный контейнер (требования к контейнеру: последовательный доступ к элементам с возможностью использования в range-based for цикле).
Метод AddDocument добавляет документы в поисковый сервер. В метод передаются: id документа, статус, рейтинг, и сам документ в формате строки. 
Метод FindTopDocuments возвращает вектор документов, соответствующих переданным ключевым словам. Результаты отсортированы по релевантности запроса. Возможна дополнительная фильтрация документов по id, статусу и рейтингу. Метод реализован в двух версиях: однопоточной и многопоточной.
Класс RequestQueue создаёт очередь запросов к поисковому серверу с сохранением результатов поиска.

Сборка и установка
Сборка осуществляется при помощи любой IDE, возможна сборка из командной строки.
Системные требования
Компилятор С++ с поддержкой стандарта С++17 и выше.






