# zephyr-demo - Demo project

Демонстрационный проект прошивки на Zephyr RTOS.

# Установка зависимостей

Следовать этой [инструкции](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies)
из документации Zephyr.

# Развертывание workspace

В этой папке выполняем следующие команды:

### С помощью pip

#### Linux/Mac

```sh
python -m venv .venv
. .venv/bin/activate
```

#### Windows

```sh
python -m venv .venv
.venv\Scripts\activate.bat
```

## Скачивание дистрибутива и модулей Zephyr


### С помощью pip

```sh
pip install west
west update
pip install -r deps/zephyr/zephyr/scripts/requirements.txt
west update
```

## Установка Zephyr SDK через west

```sh
west sdk install
```

Zephyr SDK скачается и установится в папку `$HOME/zephyr-sdk-<версия>/`.


# Образ прошивки

## Сборка

```sh
west build -b <плата> app
```

Собранный файл находится по пути: `build/zephyr/zephyr.bin`

## Прошивка

```sh
west flash
```

