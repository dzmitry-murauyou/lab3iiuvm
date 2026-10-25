# Лабораторная работа №3 — KeyNest / Lab work #3 — KeyNest

Документирование бизнес-требований курсового проекта **KeyNest** — настольного менеджера паролей (Java 17 + JavaFX, offline-first). Документы заполнены по шаблону `srs/srs-template.md` из репозитория лабораторной работы №2 (`SDLC`) на **русском и английском языках**; графическая часть выполнена на языке **SysML v2** и сохранена в **PNG**.

Business requirements documentation for the **KeyNest** course project — a desktop password manager (Java 17 + JavaFX, offline-first). The lab #2 SRS template is filled in **Russian and English**; the graphical part is modelled in **SysML v2** and exported to **PNG**.

## Состав / Contents

| Файл / File | Назначение / Purpose |
|-------------|----------------------|
| [business-requirements.md](business-requirements.md) | Выбор проекта, участники, бизнес-цели и BR-01…BR-10 (RU) |
| [business-requirements-en.md](business-requirements-en.md) | Project choice, participants, business goals and BR-01…BR-10 (EN) |
| [srs-template.md](srs-template.md) | Заполненный шаблон SRS, русская версия |
| [srs-template-en.md](srs-template-en.md) | Filled SRS template, English version |
| [sysml/keynest-model.sysml](sysml/keynest-model.sysml) | Семантическая модель SysML v2: контекст, структура, UC, BR, REQ |
| [sysml/keynest-views.sysml](sysml/keynest-views.sysml) | Графические представления: `@DiagramNode`, `@DiagramEdge`, `@GuiScreen`, `@Widget` |
| [tools/render_sysml.py](tools/render_sysml.py) | Детерминированный рендерер SysML v2-аннотаций в PNG (Pillow) |
| [img/](img/) | 3 диаграммы + 4 макета GUI в формате PNG |

### Изображения / Images

- [img/context-diagram.png](img/context-diagram.png) — контекст системы / system context
- [img/use-case-diagram.png](img/use-case-diagram.png) — варианты использования / use cases
- [img/requirements-diagram.png](img/requirements-diagram.png) — трассировка требований / requirements trace
- [img/gui-unlock.png](img/gui-unlock.png) — вход и 2FA / unlock and 2FA
- [img/gui-vault.png](img/gui-vault.png) — хранилище / main vault
- [img/gui-record-editor.png](img/gui-record-editor.png) — запись и генератор / record editor and generator
- [img/gui-autofill.png](img/gui-autofill.png) — автозаполнение / autofill

## Как пересобрать PNG / How to rebuild the PNGs

Из корня репозитория (from the repository root):

```bash
python3 -m venv .venv
.venv/bin/pip install pillow
.venv/bin/python srs/tools/render_sysml.py
```

Скрипт читает `srs/sysml/keynest-views.sysml` и перезаписывает 7 файлов в `srs/img/`. Дополнительные зависимости не нужны; для кириллицы используются системные шрифты DejaVu Sans.

SysML v2 не имеет стандартного типа диаграммы «макет GUI», поэтому экраны описаны как элементы модели с аннотациями `@GuiScreen` / `@Widget`; рендерер — это учебный, детерминированный визуализатор этих аннотаций, а не замена официальному инструменту SysML v2.

## Соглашения / Conventions

- Идентификаторы: `BR-NN` — бизнес-требования, `REQ-[AREA]-[NNN]` — требования SRS, `UC-NN` — варианты использования, `R1/R2/R3` — инкременты.
- Ключевые слова: «должен / shall», «следует / should», «может / may».
- Все требования трассируются к бизнес-требованиям (приложение B SRS) и имеют метод верификации (раздел 4 SRS).
- Учебные персонажи (Заказчик и напарник) вымышлены; фактические ФИО и даты утверждаются преподавателем.
