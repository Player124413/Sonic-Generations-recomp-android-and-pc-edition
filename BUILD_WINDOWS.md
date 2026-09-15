# Sonic Generations — сборка для Windows (ПК)

Репозиторий собирает **готовый .zip для Windows 10/11 x64**: `sonicgenerations.exe`
плюс две DLL (`rexruntime`, `rexgpu-xenos`), конфиг и лаунчер. Клавиатура
работает **из коробки** (встроенный драйвер `mnk`), геймпад — через SDL.

## Что нужно

- Windows 10/11 x64.
- [Microsoft Visual C++ Redistributable (x64)](https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist)
  — без него exe не стартует (`VCRUNTIME140.dll missing`).
- Видеодрайвер с **Vulkan 1.2+** (NVIDIA/AMD/Intel — обычный свежий драйвер;
  сборка использует Vulkan-бэкенд, как Android-порт).
- Копия игры: папка с распакованными файлами Xbox 360-версии
  (в корне должен быть `default.xex`). Игра в .zip **не входит**.

## Запуск

1. Скачай `SonicGenerations-windows.zip` из артефактов CI (Actions → Windows)
   или из Releases и распакуй.
2. Распакуй файлы игры рядом, например в папку `game` внутри распакованного
   архива (чтобы было `game\default.xex`).
3. Запусти **`Play.bat`** — он сам передаст путь и ограничит игру родными
   30 FPS (`REX_FPS_CAP=30`). Варианты:
   - перетащить папку с игрой мышью на `Play.bat`;
   - `Play.bat "D:\путь\к\игре"` из командной строки.

Сейвы и кэш шейдеров лежат в стандартной пользовательской папке приложения.

## Управление с клавиатуры (по умолчанию)

| Клавиши | Геймпад |
|---|---|
| WASD | левый стик |
| Стрелки | правый стик |
| Shift + стрелки | крестовина (D-pad) |
| Space | A |
| L | X |
| P | Y |
| ' (апостроф) / ; | B |
| Q / E | левый / правый курок |
| 1 / 3 | LB / RB |
| F / K | нажатие стика L3 / R3 |
| Z или Tab | Back |
| X или Enter | Start |

> Раскладка считывается по **кодам клавиш**, а не по буквам: на русской
> раскладке всё работает так же.

Свои бинды — в `sonicgenerations.toml` (`keybind_*`, через запятую —
альтернативы, `+` — модификаторы), мышь на правый стик — `mnk_mouse = true`.

## Настройки

Всё — в `sonicgenerations.toml` рядом с exe (пары `ключ = значение`):

- `video_mode_width` / `video_mode_height` — размер окна (по умолчанию 1280×720);
- `resolution_scale` — множитель внутреннего разрешения (1 = как в оригинале,
  2 — вдвое чётче, но тяжелее);
- `vsync` — вертикальная синхронизация;
- `log_level = "warning"` — тихий лог (для диагностики ставь `"info"`);
- `protect_zero = false` — спидхак (может падать, включай осознанно).

Лимит FPS задаётся переменной окружения `REX_FPS_CAP` в `Play.bat`
(Generations — нативная 30 FPS-игра, больше ставить не стоит).

## Сборка из исходников

```bat
git clone <repo> && cd <repo>
bash windows_sdk.sh        :: клонирует ReXGlue SDK v0.10.0 + 3 патча (Git Bash)
cmake -S windows -B build -G Ninja -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cmake --install build --prefix dist
```

Нужны: Visual Studio Build Tools (MSVC env), LLVM 18+ (`clang-cl`), Ninja,
CMake 3.25+. Именно этот набор ставит CI (см. `.github/workflows/windows.yml`).

## Типичные проблемы

| Симптом | Причина и лечение |
|---|---|
| `VCRUNTIME140.dll was not found` | поставь VC++ Redistributable (x64) |
| `vulkan-1.dll was not found` | обнови видеодрайвер (Vulkan идёт с ним) |
| Чёрный экран / вылет при старте | проверь, что путь ведёт к папке с `default.xex`; глянь лог при `log_level = "info"` |
| Игра «несётся» или дёргается | запускай через `Play.bat` (лимит 30 FPS) |
| Низкий FPS | `resolution_scale = 1`, закрой фоновые программы; у Generations тяжёлый рендер, слабым iGPU будет трудно |
