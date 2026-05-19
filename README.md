# my-top

## Требования

**Backend:**
- C++17
- CMake 3.14
- Git

**Frontend:**
- Node.js
- TypeScript (`tsc`)

## Установка

**Backend:**

```bash
curl -fsSL https://raw.githubusercontent.com/sandrocaster16/my-top/main/install-backend.sh | bash
```

**Frontend:**

```bash
curl -fsSL https://raw.githubusercontent.com/sandrocaster16/my-top/main/install-frontend.sh | bash
```

## Сборка вручную

**Backend:**

```bash
cd backend
cmake -B build
cmake --build build
```

**Frontend:**

```bash
cd frontend
tsc
```

## Запуск

```bash
my-top [-p <port>] [-d <delay>]
```

- `-p`,     `--port` — порт сервера (default: `12345`)
- `-d`,     `--delay` — интервал обновления в ms (default: `100`)
- `-pwc`,   `--ping-websocket` — интервал пинга websocket в секундах (default: `30`)
- `-v`,     `--version` — версия
- `-h`,     `--help` — help

Результаты my-top доступны при открытии страницы:<br>
[frontend/index.html](frontend/index.html)
