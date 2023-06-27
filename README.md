## Transport catalogue.

Реализация городского транспортного справочника. 
В настоящее время идет разработка приложения.

### Использование.

В настоящее время ввод/вывод осуществляется через main. 
Конструктор JSONReader получает JSON запрос, и хранит справочную информацию.
Класс MapRender по команде Draw выдает информацию в виде SVG разметки.

### Планы по расширению функционала.

1. Включение поддержки файловой системы.
2. Включение поддержки команд на добавление/вывод справочной информации.

### Функционал.

Обработка входные данных в формате JSON.
Выдача по запросу информации о остановках и/или маршрутах в формате JSON или разметки SVG.

### Системные требования.

С++20

Автор: [MrGorkiy](https://github.com/MrGorkiy)
