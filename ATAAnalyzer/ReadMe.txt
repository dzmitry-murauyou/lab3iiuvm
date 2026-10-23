ATAAnalyzer - ЛР 3, вариант А7 (низкоуровневый ATA через порты ввода-вывода)
========================================================================
Visual Studio 2008, Win32 (x86), Windows XP x32.

СБОРКА
  1. Открыть ATAAnalyzer.sln в Visual Studio 2008.
  2. Конфигурация: Release | Win32.
  3. Собрать решение (F7). ATAAnalyzer.exe появится в папке Release,
     HexIO.sys скопируется туда автоматически (post-build).
  CRT статическая (/MT) - VS2008 Redistributable на XP не нужен.

ЗАПУСК
  - Только Windows XP x86, от имени администратора.
  - HexIO.sys должен лежать рядом с ATAAnalyzer.exe.
  - Нужен IDE-контроллер (в VirtualBox - PIIX3/PIIX4).

ФАЙЛЫ
  main.cpp / MainWindow.*  - главное окно (простая тёмная тема)
  AtaController.*          - порты 0x1F0-0x1F7 / 0x170-0x177, SRST,
                             IDENTIFY DEVICE (0xEC), парсинг ответа
  hexioctrl.h, hexiosupp.lib, HexIO.sys - драйвер доступа к портам

Отчёт сохраняется кнопкой "Сохранить отчёт" в ATA_Report.txt.
Подробнее - в README.md в корне репозитория.
