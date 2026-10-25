// =====================================================================
// KeyNest - SysML v2 graphical views (textual notation)
// File: srs/sysml/keynest-views.sysml
//
// The semantic model is in keynest-model.sysml. This file declares the
// GRAPHICAL part of the SRS as SysML v2 view/model elements annotated with
// structured metadata (@DiagramNode, @DiagramEdge, @GuiScreen, @Widget).
// SysML v2 has no standard "GUI mock-up" diagram type, therefore a small
// deterministic renderer (tools/render_sysml.py) reads these annotations
// and produces the PNG files in ../img/. Re-run it after any model change.
//
// Diagrams: context, usecase, requirements.
// GUI screens: gui-unlock, gui-vault, gui-record-editor, gui-autofill.
// Text uses "|" as an explicit line break; RU/EN are kept side by side.
// =====================================================================

package KeyNestViews {
    private import ScalarValues::*;
    private import KeyNestModel::*;

    // -----------------------------------------------------------------
    // Rendering metadata definitions (SysML v2 metadata def / usage)
    // -----------------------------------------------------------------
    metadata def DiagramNode {
        attribute id      : ScalarValues::String;
        attribute diagram : ScalarValues::String;
        attribute x       : ScalarValues::Integer;
        attribute y       : ScalarValues::Integer;
        attribute w       : ScalarValues::Integer;
        attribute h       : ScalarValues::Integer;
        attribute label   : ScalarValues::String;
        attribute style   : ScalarValues::String;
    }
    metadata def DiagramEdge {
        attribute id      : ScalarValues::String;
        attribute diagram : ScalarValues::String;
        attribute src     : ScalarValues::String;
        attribute dst     : ScalarValues::String;
        attribute label   : ScalarValues::String;
        attribute style   : ScalarValues::String;
    }
    metadata def GuiScreen {
        attribute id       : ScalarValues::String;
        attribute title    : ScalarValues::String;
        attribute subtitle : ScalarValues::String;
        attribute file     : ScalarValues::String;
        attribute w        : ScalarValues::Integer;
        attribute h        : ScalarValues::Integer;
    }
    metadata def Widget {
        attribute id    : ScalarValues::String;
        attribute kind  : ScalarValues::String;
        attribute x     : ScalarValues::Integer;
        attribute y     : ScalarValues::Integer;
        attribute w     : ScalarValues::Integer;
        attribute h     : ScalarValues::Integer;
        attribute text  : ScalarValues::String;
        attribute style : ScalarValues::String;
    }

    // =================================================================
    // VIEW 1. System context diagram -> img/context-diagram.png
    // =================================================================
    part def ContextView {
        @DiagramNode { id = "c_owner"; diagram = "context"; x = 70;   y = 390; w = 230; h = 160; label = "Vault Owner|Владелец хранилища"; style = "actor"; }
        part ctxOwner : VaultOwner;
        @DiagramNode { id = "c_clock"; diagram = "context"; x = 70;   y = 700; w = 260; h = 110; label = "OS Clock / NTP|Время для TOTP"; style = "external"; }
        part ctxClock : OperatingSystemClock;
        @DiagramNode { id = "c_app"; diagram = "context"; x = 590;  y = 330; w = 400; h = 270; label = "KeyNest Desktop App|Java 17 + JavaFX|Offline-first vault"; style = "system"; }
        part ctxApp : KeyNestApp;
        @DiagramNode { id = "c_vault"; diagram = "context"; x = 620;  y = 720; w = 340; h = 130; label = "vault.knx|AES-256-GCM records"; style = "storage"; }
        part ctxVault : EncryptedVaultFile;
        @DiagramNode { id = "c_target"; diagram = "context"; x = 1210; y = 120; w = 320; h = 120; label = "Target App / Browser|Целевое приложение"; style = "external"; }
        part ctxTarget : TargetApplication;
        @DiagramNode { id = "c_keychain"; diagram = "context"; x = 1210; y = 300; w = 320; h = 120; label = "OS Keychain|Хранилище ключей ОС"; style = "external"; }
        part ctxKeychain : OSKeychain;
        @DiagramNode { id = "c_hibp"; diagram = "context"; x = 1210; y = 490; w = 320; h = 120; label = "Pwned Passwords API|HIBP - optional"; style = "external_dashed"; }
        part ctxHibp : PwnedPasswordsService;
        @DiagramNode { id = "c_updates"; diagram = "context"; x = 1210; y = 680; w = 320; h = 120; label = "GitHub Releases|Updates - optional"; style = "external_dashed"; }
        part ctxUpdates : UpdateService;

        @DiagramEdge { id = "ce1"; diagram = "context"; src = "c_owner"; dst = "c_app"; label = "master password + 2FA"; style = "solid"; }
        part ce1;
        @DiagramEdge { id = "ce2"; diagram = "context"; src = "c_app"; dst = "c_target"; label = "autofill by hotkey / clipboard"; style = "solid"; }
        part ce2;
        @DiagramEdge { id = "ce3"; diagram = "context"; src = "c_app"; dst = "c_keychain"; label = "wrapped vault key"; style = "solid"; }
        part ce3;
        @DiagramEdge { id = "ce4"; diagram = "context"; src = "c_app"; dst = "c_vault"; label = "encrypted read / write"; style = "solid"; }
        part ce4;
        @DiagramEdge { id = "ce5"; diagram = "context"; src = "c_app"; dst = "c_hibp"; label = "SHA-1 prefix only"; style = "dashed"; }
        part ce5;
        @DiagramEdge { id = "ce6"; diagram = "context"; src = "c_app"; dst = "c_updates"; label = "version check"; style = "dashed"; }
        part ce6;
        @DiagramEdge { id = "ce7"; diagram = "context"; src = "c_clock"; dst = "c_app"; label = "time for TOTP"; style = "dashed"; }
        part ce7;
    }

    // =================================================================
    // VIEW 2. Use case diagram -> img/use-case-diagram.png
    // =================================================================
    part def UseCaseView {
        @DiagramNode { id = "u_boundary"; diagram = "usecase"; x = 410; y = 100; w = 1120; h = 770; label = "KeyNest 1.0 - system boundary|Границы системы"; style = "boundary"; }
        part ucBoundary : KeyNestApp;
        @DiagramNode { id = "u_actor"; diagram = "usecase"; x = 70; y = 390; w = 250; h = 180; label = "Vault Owner|Владелец хранилища"; style = "actor"; }
        part ucActor : VaultOwner;

        @DiagramNode { id = "uc1"; diagram = "usecase"; x = 465;  y = 190; w = 300; h = 120; label = "UC-01 Unlock vault|Разблокировка + 2FA"; style = "usecase"; }
        part uc1;
        @DiagramNode { id = "uc2"; diagram = "usecase"; x = 820;  y = 190; w = 300; h = 120; label = "UC-02 Generate password|Генерация пароля"; style = "usecase"; }
        part uc2;
        @DiagramNode { id = "uc3"; diagram = "usecase"; x = 1175; y = 190; w = 300; h = 120; label = "UC-03 Create / edit|Создать запись"; style = "usecase"; }
        part uc3;
        @DiagramNode { id = "uc4"; diagram = "usecase"; x = 465;  y = 380; w = 300; h = 120; label = "UC-04 Categories and notes|Категории и заметки"; style = "usecase"; }
        part uc4;
        @DiagramNode { id = "uc5"; diagram = "usecase"; x = 820;  y = 380; w = 300; h = 120; label = "UC-05 Search and filter|Поиск и фильтры"; style = "usecase"; }
        part uc5;
        @DiagramNode { id = "uc6"; diagram = "usecase"; x = 1175; y = 380; w = 300; h = 120; label = "UC-06 Autofill|Автозаполнение"; style = "usecase"; }
        part uc6;
        @DiagramNode { id = "uc7"; diagram = "usecase"; x = 465;  y = 570; w = 300; h = 120; label = "UC-07 TOTP codes|Коды 2FA"; style = "usecase"; }
        part uc7;
        @DiagramNode { id = "uc8"; diagram = "usecase"; x = 820;  y = 570; w = 300; h = 120; label = "UC-08 Security audit|Аудит паролей"; style = "usecase"; }
        part uc8;
        @DiagramNode { id = "uc9"; diagram = "usecase"; x = 1175; y = 570; w = 300; h = 120; label = "UC-09 Backup / import|Резервная копия"; style = "usecase"; }
        part uc9;
        @DiagramNode { id = "u_hibp"; diagram = "usecase"; x = 1175; y = 878; w = 300; h = 72; label = "HIBP API - optional|Внешний сервис"; style = "external_dashed"; }
        part ucHibp : PwnedPasswordsService;

        @DiagramEdge { id = "ue1";  diagram = "usecase"; src = "u_actor"; dst = "uc1"; label = ""; style = "bus"; }
        part ue1;
        @DiagramEdge { id = "ue2";  diagram = "usecase"; src = "u_actor"; dst = "uc2"; label = ""; style = "bus"; }
        part ue2;
        @DiagramEdge { id = "ue3";  diagram = "usecase"; src = "u_actor"; dst = "uc3"; label = ""; style = "bus"; }
        part ue3;
        @DiagramEdge { id = "ue4";  diagram = "usecase"; src = "u_actor"; dst = "uc4"; label = ""; style = "bus"; }
        part ue4;
        @DiagramEdge { id = "ue5";  diagram = "usecase"; src = "u_actor"; dst = "uc5"; label = ""; style = "bus"; }
        part ue5;
        @DiagramEdge { id = "ue6";  diagram = "usecase"; src = "u_actor"; dst = "uc6"; label = ""; style = "bus"; }
        part ue6;
        @DiagramEdge { id = "ue7";  diagram = "usecase"; src = "u_actor"; dst = "uc7"; label = ""; style = "bus"; }
        part ue7;
        @DiagramEdge { id = "ue8";  diagram = "usecase"; src = "u_actor"; dst = "uc8"; label = ""; style = "bus"; }
        part ue8;
        @DiagramEdge { id = "ue9";  diagram = "usecase"; src = "u_actor"; dst = "uc9"; label = ""; style = "bus"; }
        part ue9;
        @DiagramEdge { id = "ue10"; diagram = "usecase"; src = "uc6"; dst = "uc1"; label = "include"; style = "dashed"; }
        part ue10;
        @DiagramEdge { id = "ue11"; diagram = "usecase"; src = "uc8"; dst = "u_hibp"; label = "extend - optional"; style = "dashed"; }
        part ue11;
    }

    // =================================================================
    // VIEW 3. Business requirements trace -> img/requirements-diagram.png
    // =================================================================
    part def RequirementsView {
        @DiagramNode { id = "h1"; diagram = "requirements"; x = 60;   y = 115; w = 350; h = 62; label = "BUSINESS GOALS|Бизнес-цели"; style = "header"; }
        part h1;
        @DiagramNode { id = "h2"; diagram = "requirements"; x = 470;  y = 115; w = 640; h = 62; label = "BUSINESS REQUIREMENTS (BR-01..BR-10)|Бизнес-требования"; style = "header"; }
        part h2;
        @DiagramNode { id = "h3"; diagram = "requirements"; x = 1180; y = 115; w = 350; h = 62; label = "RELEASE INCREMENTS|Релизы"; style = "header"; }
        part h3;

        @DiagramNode { id = "g1"; diagram = "requirements"; x = 60; y = 235; w = 350; h = 105; label = "BG-01 Protect secrets|Не допускать утечки паролей"; style = "goal"; }
        part g1;
        @DiagramNode { id = "g2"; diagram = "requirements"; x = 60; y = 375; w = 350; h = 105; label = "BG-02 Save login time|Ускорить ежедневный вход"; style = "goal"; }
        part g2;
        @DiagramNode { id = "g3"; diagram = "requirements"; x = 60; y = 515; w = 350; h = 105; label = "BG-03 Stay independent|Не зависеть от облака"; style = "goal"; }
        part g3;
        @DiagramNode { id = "g4"; diagram = "requirements"; x = 60; y = 655; w = 350; h = 105; label = "BG-04 Fit the course|Реализуемо за семестр"; style = "goal"; }
        part g4;

        @DiagramNode { id = "r1";  diagram = "requirements"; x = 470; y = 220; w = 300; h = 80; label = "BR-01 Encrypted local vault|Зашифрованное хранилище"; style = "req"; }
        part r1;
        @DiagramNode { id = "r2";  diagram = "requirements"; x = 790; y = 220; w = 300; h = 80; label = "BR-02 Password generation|Генерация паролей"; style = "req"; }
        part r2;
        @DiagramNode { id = "r3";  diagram = "requirements"; x = 470; y = 320; w = 300; h = 80; label = "BR-03 Credential records|Записи с реквизитами"; style = "req"; }
        part r3;
        @DiagramNode { id = "r4";  diagram = "requirements"; x = 790; y = 320; w = 300; h = 80; label = "BR-04 Autofill|Автозаполнение"; style = "req"; }
        part r4;
        @DiagramNode { id = "r5";  diagram = "requirements"; x = 470; y = 420; w = 300; h = 80; label = "BR-05 Categories and search|Категории и поиск"; style = "req"; }
        part r5;
        @DiagramNode { id = "r6";  diagram = "requirements"; x = 790; y = 420; w = 300; h = 80; label = "BR-06 Secure notes|Заметки"; style = "req"; }
        part r6;
        @DiagramNode { id = "r7";  diagram = "requirements"; x = 470; y = 520; w = 300; h = 80; label = "BR-07 2FA and TOTP|Двухфакторная защита"; style = "req"; }
        part r7;
        @DiagramNode { id = "r8";  diagram = "requirements"; x = 790; y = 520; w = 300; h = 80; label = "BR-08 Backup / import|Резервная копия и импорт"; style = "req"; }
        part r8;
        @DiagramNode { id = "r9";  diagram = "requirements"; x = 470; y = 620; w = 300; h = 80; label = "BR-09 Password audit|Аудит паролей"; style = "req"; }
        part r9;
        @DiagramNode { id = "r10"; diagram = "requirements"; x = 790; y = 620; w = 300; h = 80; label = "BR-10 Offline-first|Работа без сети"; style = "req"; }
        part r10;

        @DiagramNode { id = "rel1"; diagram = "requirements"; x = 1180; y = 250; w = 350; h = 120; label = "R1 - MVP|unlock, generate, CRUD, categories, notes"; style = "release"; }
        part rel1;
        @DiagramNode { id = "rel2"; diagram = "requirements"; x = 1180; y = 470; w = 350; h = 120; label = "R2 - Security|autofill, 2FA/TOTP, backup, import"; style = "release"; }
        part rel2;
        @DiagramNode { id = "rel3"; diagram = "requirements"; x = 1180; y = 690; w = 350; h = 120; label = "R3 - Quality|audit, HIBP check, polish, RU/EN"; style = "release"; }
        part rel3;
        @DiagramNode { id = "legend"; diagram = "requirements"; x = 60; y = 820; w = 1030; h = 100; label = "Solid line = goal to business requirement  |  Dashed line = requirement allocated to a release  |  Priority: MUST is bold  |  Source: Customer (E. Sokolova)"; style = "legend"; }
        part legend;

        @DiagramEdge { id = "t1";  diagram = "requirements"; src = "g1"; dst = "r1";  label = ""; style = "trace"; }
        part t1;
        @DiagramEdge { id = "t2";  diagram = "requirements"; src = "g1"; dst = "r7";  label = ""; style = "trace"; }
        part t2;
        @DiagramEdge { id = "t3";  diagram = "requirements"; src = "g1"; dst = "r9";  label = ""; style = "trace"; }
        part t3;
        @DiagramEdge { id = "t4";  diagram = "requirements"; src = "g2"; dst = "r2";  label = ""; style = "trace"; }
        part t4;
        @DiagramEdge { id = "t5";  diagram = "requirements"; src = "g2"; dst = "r4";  label = ""; style = "trace"; }
        part t5;
        @DiagramEdge { id = "t6";  diagram = "requirements"; src = "g2"; dst = "r5";  label = ""; style = "trace"; }
        part t6;
        @DiagramEdge { id = "t7";  diagram = "requirements"; src = "g3"; dst = "r3";  label = ""; style = "trace"; }
        part t7;
        @DiagramEdge { id = "t8";  diagram = "requirements"; src = "g3"; dst = "r6";  label = ""; style = "trace"; }
        part t8;
        @DiagramEdge { id = "t9";  diagram = "requirements"; src = "g3"; dst = "r8";  label = ""; style = "trace"; }
        part t9;
        @DiagramEdge { id = "t10"; diagram = "requirements"; src = "g3"; dst = "r10"; label = ""; style = "trace"; }
        part t10;
        @DiagramEdge { id = "t11"; diagram = "requirements"; src = "g4"; dst = "r10"; label = ""; style = "trace"; }
        part t11;
        @DiagramEdge { id = "t12"; diagram = "requirements"; src = "g4"; dst = "r2";  label = ""; style = "trace"; }
        part t12;

        @DiagramEdge { id = "a1";  diagram = "requirements"; src = "r1";  dst = "rel1"; label = ""; style = "alloc"; }
        part a1;
        @DiagramEdge { id = "a2";  diagram = "requirements"; src = "r2";  dst = "rel1"; label = ""; style = "alloc"; }
        part a2;
        @DiagramEdge { id = "a3";  diagram = "requirements"; src = "r3";  dst = "rel1"; label = ""; style = "alloc"; }
        part a3;
        @DiagramEdge { id = "a4";  diagram = "requirements"; src = "r4";  dst = "rel2"; label = ""; style = "alloc"; }
        part a4;
        @DiagramEdge { id = "a5";  diagram = "requirements"; src = "r5";  dst = "rel1"; label = ""; style = "alloc"; }
        part a5;
        @DiagramEdge { id = "a6";  diagram = "requirements"; src = "r6";  dst = "rel1"; label = ""; style = "alloc"; }
        part a6;
        @DiagramEdge { id = "a7";  diagram = "requirements"; src = "r7";  dst = "rel2"; label = ""; style = "alloc"; }
        part a7;
        @DiagramEdge { id = "a8";  diagram = "requirements"; src = "r8";  dst = "rel2"; label = ""; style = "alloc"; }
        part a8;
        @DiagramEdge { id = "a9";  diagram = "requirements"; src = "r9";  dst = "rel3"; label = ""; style = "alloc"; }
        part a9;
        @DiagramEdge { id = "a10"; diagram = "requirements"; src = "r10"; dst = "rel3"; label = ""; style = "alloc"; }
        part a10;
    }

    // =================================================================
    // VIEW 4. GUI mock-up: unlock / 2FA -> img/gui-unlock.png
    // =================================================================
    @GuiScreen { id = "unlock"; title = "KeyNest - Unlock Vault"; subtitle = "SysML v2 GUI mock-up - RU locale - screenshot 1 of 4"; w = 1440; h = 900; file = "gui-unlock.png"; }
    part def UnlockScreen {
        @Widget { id = "un01"; kind = "titlebar"; x = 0;   y = 0;   w = 1440; h = 60;  text = "KeyNest|v0.1 - local vault"; style = "dark"; }
        part un01;
        @Widget { id = "un02"; kind = "panel";    x = 0;   y = 60;  w = 520;  h = 790; text = ""; style = "brand"; }
        part un02;
        @Widget { id = "un03"; kind = "logo";     x = 70;  y = 130; w = 90;   h = 90;  text = "K"; style = "accent"; }
        part un03;
        @Widget { id = "un04"; kind = "label";    x = 70;  y = 250; w = 410;  h = 130; text = "Ваши ключи -|под вашим замком"; style = "hero_white"; }
        part un04;
        @Widget { id = "un05"; kind = "label";    x = 70;  y = 410; w = 410;  h = 190; text = "AES-256-GCM для записей|Argon2id для мастер-пароля|TOTP / 2FA без облака|Никакой телеметрии|Один файл - одна копия"; style = "white"; }
        part un05;
        @Widget { id = "un06"; kind = "chip";     x = 70;  y = 720; w = 380;  h = 46;  text = "OFFLINE FIRST - сеть не требуется"; style = "ok"; }
        part un06;

        @Widget { id = "un07"; kind = "label";    x = 610; y = 125; w = 760;  h = 60;  text = "Разблокировать хранилище"; style = "title"; }
        part un07;
        @Widget { id = "un08"; kind = "label";    x = 610; y = 190; w = 760;  h = 32;  text = "Файл: C:/Users/student/KeyNest/vault.knx"; style = "muted"; }
        part un08;
        @Widget { id = "un09"; kind = "input";    x = 610; y = 240; w = 760;  h = 88;  text = "Мастер-пароль|................"; style = "password"; }
        part un09;
        @Widget { id = "un10"; kind = "chip";     x = 1220; y = 266; w = 130; h = 36; text = "Caps Lock"; style = "warn"; }
        part un10;
        @Widget { id = "un11"; kind = "label";    x = 610; y = 352; w = 760;  h = 32;  text = "Код двухфакторной аутентификации (TOTP)"; style = "bold"; }
        part un11;
        @Widget { id = "un12"; kind = "code";     x = 610; y = 392; w = 390;  h = 74;  text = "482190"; style = "accent"; }
        part un12;
        @Widget { id = "un13"; kind = "countdown"; x = 1020; y = 392; w = 74; h = 74; text = "23"; style = "accent"; }
        part un13;
        @Widget { id = "un14"; kind = "label";    x = 1110; y = 405; w = 260; h = 48; text = "сек до смены кода"; style = "muted"; }
        part un14;
        @Widget { id = "un15"; kind = "checkbox"; x = 610; y = 492; w = 430;  h = 36;  text = "Запомнить путь к хранилищу"; style = "checked"; }
        part un15;
        @Widget { id = "un16"; kind = "button";   x = 610; y = 550; w = 760;  h = 66;  text = "Разблокировать"; style = "primary"; }
        part un16;
        @Widget { id = "un17"; kind = "button";   x = 610; y = 632; w = 370;  h = 50;  text = "Ключ восстановления"; style = "secondary"; }
        part un17;
        @Widget { id = "un18"; kind = "button";   x = 1000; y = 632; w = 370; h = 50;  text = "RU / EN"; style = "secondary"; }
        part un18;
        @Widget { id = "un19"; kind = "note";     x = 610; y = 702; w = 760;  h = 76;  text = "Неверный мастер-пароль или код 2FA. Осталось 4 попытки до задержки 30 с."; style = "warn"; }
        part un19;
        @Widget { id = "un20"; kind = "label";    x = 610; y = 798; w = 760;  h = 34;  text = "Автоблокировка: 5 мин простоя - журнал событий включён"; style = "muted"; }
        part un20;
    }

    // =================================================================
    // VIEW 5. GUI mock-up: main vault -> img/gui-vault.png
    // =================================================================
    @GuiScreen { id = "vault"; title = "KeyNest - My Vault"; subtitle = "SysML v2 GUI mock-up - RU locale - screenshot 2 of 4"; w = 1440; h = 900; file = "gui-vault.png"; }
    part def VaultScreen {
        @Widget { id = "va01"; kind = "titlebar"; x = 0;   y = 0;   w = 1440; h = 60; text = "KeyNest|Хранилище разблокировано"; style = "dark"; }
        part va01;
        @Widget { id = "va02"; kind = "sidebar";  x = 24;  y = 84;  w = 270;  h = 770; text = "#ХРАНИЛИЩЕ|Все записи (128)|Избранное (12)|#КАТЕГОРИИ|Работа (54)|Личное (38)|Финансы (21)|Заметки (15)|#ИНСТРУМЕНТЫ|Аудит безопасности|Генератор паролей|Настройки"; style = "dark"; }
        part va02;
        @Widget { id = "va03"; kind = "search";   x = 320; y = 92;  w = 690;  h = 52; text = "Поиск по названию, логину или сайту"; style = "default"; }
        part va03;
        @Widget { id = "va04"; kind = "button";   x = 1030; y = 92; w = 370; h = 52; text = "+ Добавить запись"; style = "primary"; }
        part va04;
        @Widget { id = "va05"; kind = "chip";     x = 320; y = 158; w = 340;  h = 34; text = "Ctrl+Shift+K - автозаполнение"; style = "accent"; }
        part va05;
        @Widget { id = "va06"; kind = "chip";     x = 680; y = 158; w = 220;  h = 34; text = "Сортировка: недавние"; style = "muted"; }
        part va06;
        @Widget { id = "va07"; kind = "list";     x = 320; y = 206; w = 560;  h = 592; text = "GitHub - user@company.dev|Почта - elena@mail.dev|Банк - e.sokolova|Wi-Fi роутер - admin|Staging сервер - deploy|Облачный диск - elena.s|VPN офиса - e.sokolova|Личный кабинет - elena"; style = "records"; }
        part va07;

        @Widget { id = "va08"; kind = "card";     x = 900; y = 206; w = 500;  h = 592; text = ""; style = "default"; }
        part va08;
        @Widget { id = "va09"; kind = "label";    x = 926; y = 228; w = 450;  h = 48;  text = "GitHub"; style = "title"; }
        part va09;
        @Widget { id = "va10"; kind = "chip";     x = 926; y = 282; w = 110;  h = 34;  text = "Работа"; style = "accent"; }
        part va10;
        @Widget { id = "va11"; kind = "chip";     x = 1046; y = 282; w = 100; h = 34;  text = "TOTP"; style = "ok"; }
        part va11;
        @Widget { id = "va12"; kind = "input";    x = 926; y = 330; w = 448;  h = 64;  text = "Логин|user@company.dev"; style = "filled"; }
        part va12;
        @Widget { id = "va13"; kind = "input";    x = 926; y = 404; w = 448;  h = 64;  text = "Пароль|................"; style = "password"; }
        part va13;
        @Widget { id = "va14"; kind = "button";   x = 926; y = 478; w = 216;  h = 46;  text = "Копировать"; style = "secondary"; }
        part va14;
        @Widget { id = "va15"; kind = "button";   x = 1158; y = 478; w = 216; h = 46;  text = "Показать"; style = "secondary"; }
        part va15;
        @Widget { id = "va16"; kind = "label";    x = 926; y = 540; w = 448;  h = 30;  text = "Код 2FA (TOTP)"; style = "bold"; }
        part va16;
        @Widget { id = "va17"; kind = "code";     x = 926; y = 574; w = 270;  h = 62;  text = "482190"; style = "accent"; }
        part va17;
        @Widget { id = "va18"; kind = "countdown"; x = 1210; y = 574; w = 62; h = 62; text = "18"; style = "accent"; }
        part va18;
        @Widget { id = "va19"; kind = "note";     x = 926; y = 650; w = 448;  h = 66;  text = "Заметки|Токен для CI обновляется раз в 90 дней"; style = "light"; }
        part va19;
        @Widget { id = "va20"; kind = "button";   x = 926; y = 730; w = 216;  h = 46;  text = "Изменить"; style = "primary"; }
        part va20;
        @Widget { id = "va21"; kind = "button";   x = 1158; y = 730; w = 216; h = 46;  text = "Удалить"; style = "danger"; }
        part va21;
        @Widget { id = "va22"; kind = "label";    x = 320; y = 812; w = 1080; h = 36;  text = "Локальный файл vault.knx - авто-блокировка через 04:12 - сеть отключена"; style = "muted"; }
        part va22;
    }

    // =================================================================
    // VIEW 6. GUI mock-up: record editor + generator -> img/gui-record-editor.png
    // =================================================================
    @GuiScreen { id = "editor"; title = "KeyNest - Record Editor"; subtitle = "SysML v2 GUI mock-up - RU locale - screenshot 3 of 4"; w = 1440; h = 900; file = "gui-record-editor.png"; }
    part def EditorScreen {
        @Widget { id = "ed01"; kind = "titlebar";  x = 0;   y = 0;   w = 1440; h = 60; text = "KeyNest|Новая запись - не сохранено"; style = "dark"; }
        part ed01;
        @Widget { id = "ed02"; kind = "label";     x = 60;  y = 92;  w = 760;  h = 48; text = "Новая запись"; style = "title"; }
        part ed02;
        @Widget { id = "ed03"; kind = "label";     x = 60;  y = 140; w = 760;  h = 28; text = "Все поля шифруются в файле vault.knx"; style = "muted"; }
        part ed03;
        @Widget { id = "ed04"; kind = "input";     x = 60;  y = 178; w = 760;  h = 72; text = "Название|GitHub"; style = "filled"; }
        part ed04;
        @Widget { id = "ed05"; kind = "input";     x = 60;  y = 258; w = 760;  h = 72; text = "Логин|user@company.dev"; style = "filled"; }
        part ed05;
        @Widget { id = "ed06"; kind = "input";     x = 60;  y = 338; w = 760;  h = 72; text = "Сайт (URL)|https://github.com/login"; style = "filled"; }
        part ed06;
        @Widget { id = "ed07"; kind = "input";     x = 60;  y = 418; w = 760;  h = 72; text = "Категория|Работа (изменить)"; style = "filled"; }
        part ed07;
        @Widget { id = "ed08"; kind = "input";     x = 60;  y = 498; w = 560;  h = 72; text = "Пароль|...................."; style = "password"; }
        part ed08;
        @Widget { id = "ed09"; kind = "button";    x = 632; y = 498; w = 188;  h = 72; text = "Сгенерировать"; style = "secondary"; }
        part ed09;
        @Widget { id = "ed10"; kind = "strength";  x = 60;  y = 580; w = 760;  h = 32; text = "Надёжность: высокая - около 119 бит"; style = "ok"; }
        part ed10;
        @Widget { id = "ed11"; kind = "label";     x = 60;  y = 622; w = 760;  h = 30; text = "Двухфакторная аутентификация (TOTP)"; style = "bold"; }
        part ed11;
        @Widget { id = "ed12"; kind = "qr";        x = 60;  y = 658; w = 104;  h = 104; text = "otpauth://totp/GitHub"; style = "accent"; }
        part ed12;
        @Widget { id = "ed13"; kind = "input";     x = 182; y = 658; w = 638;  h = 50; text = "Секрет TOTP|JBSWY3DPEHPK3PXP"; style = "filled"; }
        part ed13;
        @Widget { id = "ed14"; kind = "input";     x = 182; y = 716; w = 638;  h = 46; text = "Проверочный код|482 190"; style = "filled"; }
        part ed14;
        @Widget { id = "ed15"; kind = "note";      x = 60;  y = 780; w = 760;  h = 44; text = "Заметки|Резервный токен лежит в сейфе офиса"; style = "light"; }
        part ed15;
        @Widget { id = "ed16"; kind = "button";    x = 60;  y = 832; w = 370;  h = 44; text = "Сохранить запись"; style = "primary"; }
        part ed16;
        @Widget { id = "ed17"; kind = "button";    x = 450; y = 832; w = 370;  h = 44; text = "Отмена"; style = "secondary"; }
        part ed17;

        @Widget { id = "ed18"; kind = "panel";     x = 850; y = 92;  w = 550;  h = 784; text = ""; style = "light"; }
        part ed18;
        @Widget { id = "ed19"; kind = "label";     x = 880; y = 118; w = 490;  h = 46; text = "Генератор паролей"; style = "title"; }
        part ed19;
        @Widget { id = "ed20"; kind = "card";      x = 880; y = 176; w = 490;  h = 78; text = "X7@pQ2!vN9#Lm4Rt"; style = "mono"; }
        part ed20;
        @Widget { id = "ed21"; kind = "label";     x = 880; y = 270; w = 490;  h = 30; text = "Длина пароля: 20 символов"; style = "bold"; }
        part ed21;
        @Widget { id = "ed22"; kind = "slider";    x = 880; y = 304; w = 490;  h = 34; text = "20"; style = "accent"; }
        part ed22;
        @Widget { id = "ed23"; kind = "checkbox";  x = 880; y = 354; w = 230;  h = 34; text = "A-Z Заглавные"; style = "checked"; }
        part ed23;
        @Widget { id = "ed24"; kind = "checkbox";  x = 880; y = 394; w = 230;  h = 34; text = "a-z Строчные"; style = "checked"; }
        part ed24;
        @Widget { id = "ed25"; kind = "checkbox";  x = 880; y = 434; w = 230;  h = 34; text = "0-9 Цифры"; style = "checked"; }
        part ed25;
        @Widget { id = "ed26"; kind = "checkbox";  x = 880; y = 474; w = 230;  h = 34; text = "!@# Символы"; style = "checked"; }
        part ed26;
        @Widget { id = "ed27"; kind = "checkbox";  x = 1120; y = 354; w = 250; h = 34; text = "Исключать похожие"; style = "checked"; }
        part ed27;
        @Widget { id = "ed28"; kind = "checkbox";  x = 1120; y = 394; w = 250; h = 34; text = "Без повторов"; style = "unchecked"; }
        part ed28;
        @Widget { id = "ed29"; kind = "strength";  x = 880; y = 524; w = 490;  h = 34; text = "Энтропия около 119 бит"; style = "ok"; }
        part ed29;
        @Widget { id = "ed30"; kind = "button";    x = 880; y = 578; w = 490;  h = 52; text = "Перегенерировать"; style = "secondary"; }
        part ed30;
        @Widget { id = "ed31"; kind = "button";    x = 880; y = 642; w = 490;  h = 52; text = "Использовать пароль"; style = "primary"; }
        part ed31;
        @Widget { id = "ed32"; kind = "note";      x = 880; y = 712; w = 490;  h = 132; text = "Генерация выполняется локально на устройстве. Пароль не отправляется в сеть и не попадает в журнал событий."; style = "light"; }
        part ed32;
    }

    // =================================================================
    // VIEW 7. GUI mock-up: autofill overlay -> img/gui-autofill.png
    // =================================================================
    @GuiScreen { id = "autofill"; title = "KeyNest - Autofill"; subtitle = "SysML v2 GUI mock-up - RU locale - screenshot 4 of 4"; w = 1440; h = 900; file = "gui-autofill.png"; }
    part def AutofillScreen {
        @Widget { id = "af01"; kind = "titlebar"; x = 0;   y = 0;   w = 1440; h = 60; text = "KeyNest|Глобальная горячая клавиша активна"; style = "dark"; }
        part af01;
        @Widget { id = "af02"; kind = "window";   x = 60;  y = 100; w = 800;  h = 700; text = "https://github.com/login"; style = "browser"; }
        part af02;
        @Widget { id = "af03"; kind = "label";    x = 100; y = 245; w = 720;  h = 52; text = "Sign in to GitHub"; style = "title"; }
        part af03;
        @Widget { id = "af04"; kind = "input";    x = 100; y = 315; w = 720;  h = 72; text = "Username or email address|"; style = "empty"; }
        part af04;
        @Widget { id = "af05"; kind = "input";    x = 100; y = 402; w = 720;  h = 72; text = "Password|"; style = "empty"; }
        part af05;
        @Widget { id = "af06"; kind = "button";   x = 100; y = 496; w = 720;  h = 56; text = "Sign in"; style = "primary"; }
        part af06;
        @Widget { id = "af07"; kind = "label";    x = 100; y = 580; w = 720;  h = 60; text = "Окно браузера определено по заголовку и домену - KeyNest предлагает подходящую запись"; style = "muted"; }
        part af07;

        @Widget { id = "af08"; kind = "card";     x = 900; y = 130; w = 490;  h = 670; text = ""; style = "overlay"; }
        part af08;
        @Widget { id = "af09"; kind = "label";    x = 930; y = 158; w = 430;  h = 46; text = "KeyNest"; style = "title"; }
        part af09;
        @Widget { id = "af10"; kind = "chip";     x = 930; y = 210; w = 270;  h = 36; text = "Совпадение по домену"; style = "ok"; }
        part af10;
        @Widget { id = "af11"; kind = "card";     x = 930; y = 262; w = 430;  h = 128; text = "GitHub|user@company.dev|................"; style = "light"; }
        part af11;
        @Widget { id = "af12"; kind = "label";    x = 930; y = 402; w = 430;  h = 32; text = "Домен: github.com - запись 1 из 1"; style = "muted"; }
        part af12;
        @Widget { id = "af13"; kind = "button";   x = 930; y = 448; w = 430;  h = 58; text = "Автозаполнить"; style = "primary"; }
        part af13;
        @Widget { id = "af14"; kind = "hotkey";   x = 930; y = 518; w = 430;  h = 46; text = "Ctrl + Shift + K"; style = "default"; }
        part af14;
        @Widget { id = "af15"; kind = "button";   x = 930; y = 580; w = 430;  h = 48; text = "Скопировать пароль"; style = "secondary"; }
        part af15;
        @Widget { id = "af16"; kind = "button";   x = 930; y = 640; w = 430;  h = 48; text = "Открыть запись"; style = "secondary"; }
        part af16;
        @Widget { id = "af17"; kind = "note";     x = 930; y = 704; w = 430;  h = 54; text = "Буфер обмена очистится через 30 с"; style = "warn"; }
        part af17;
        @Widget { id = "af18"; kind = "label";    x = 930; y = 766; w = 430;  h = 30; text = "Пароль не попадает в журнал и телеметрию"; style = "muted"; }
        part af18;
        @Widget { id = "af19"; kind = "label";    x = 60;  y = 820; w = 1330; h = 36; text = "Автозаполнение работает офлайн - ввод через буфер обмена или имитацию нажатий клавиш"; style = "muted"; }
        part af19;
    }
}
