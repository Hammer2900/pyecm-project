### Шпаргалка: Публикация новой версии Python-пакета

#### Шаг 1: Подготовка

1.  **Внесите изменения** в код (`.py`, `.pyx`, `.c` и т.д.).
2.  **Обновите версию** в файле `pyproject.toml`.
    ```toml
    # pyproject.toml
    version = "1.0.1" # было 1.0.0
    ```
3.  **Сохраните всё в Git** (хорошая практика).
    ```bash
    git add .
    git commit -m "feat: Add new feature for version 1.0.1"
    git push
    ```

#### Шаг 2: Сборка пакета

1.  **Активируйте** ваше виртуальное окружение.
    ```bash
    source .venv/bin/activate
    ```
2.  **Очистите** старые сборки.
    ```bash
    rm -rf dist/ wheelhouse/ build/ src/*.egg-info
    ```
3.  **Соберите исходный дистрибутив (`sdist`).**
    ```bash
    uv build --sdist -p .venv/bin/python
    ```
4.  **Соберите бинарные `wheel`-файлы для Linux.**
    ```bash
    cibuildwheel --platform linux
    ```
    *(Для Windows/macOS нужно запускать `cibuildwheel` на соответствующей ОС).*

#### Шаг 3: Публикация

1.  **Загрузите ВСЁ** (sdist и wheels) на сервер.

    *   **Для TestPyPI (тренировка):**
        ```bash
        twine upload --repository testpypi wheelhouse/* dist/*
        ```

    *   **Для настоящего PyPI (боевой режим):**
        ```bash
        twine upload wheelhouse/* dist/*
        ```

2.  **Введите данные для входа**, когда `twine` попросит:
    *   **Username:** `__token__`
    *   **Password:** `[вставьте ваш API-токен]`

---
### Итог (только команды)

```bash
# 1. Поднять версию в pyproject.toml

# 2. Сохранить в Git
git add .
git commit -m "Bump version to X.Y.Z"
git push

# 3. Активировать окружение
source .venv/bin/activate

# 4. Очистить
rm -rf dist/ wheelhouse/ build/ src/*.egg-info

# 5. Собрать
uv build --sdist -p .venv/bin/python
cibuildwheel --platform linux

# 6. Опубликовать
twine upload wheelhouse/* dist/*
```