# Как собрать APK (Sonic Generations — Android Edition)

Ничего ставить на компьютер не нужно: APK собирает GitHub Actions в облаке.
Бесплатно, ~1–2 часа первая сборка, повторные быстрее (кэш компиляции).

## Вариант 1: сборка в GitHub Actions (рекомендуется)

1. Запушь этот репозиторий на GitHub (если ещё не запушен).
2. Открой вкладку **Actions** → выбери **Android APK (Sonic Generations)**.
   - Сборка стартует автоматически при каждом `push`.
   - Вручную: кнопка **Run workflow** → **Run workflow**.
3. Дождись зелёной галочки ✅.
4. Открой завершённый запуск (run) → внизу раздел **Artifacts** →
   скачай **SonicGenerations-android** (внутри `.apk` + `.sha256`).

Готово: перекинь `.apk` на телефон и установи.

## Вариант 2: Release по тегу (красивая страница релиза)

```bash
git tag v1.0.0
git push origin v1.0.0
```

После сборки APK появится на странице **Releases** репозитория.
Каждый следующий релиз — новый тег (`v1.0.1`, `v1.1.0` …).

## Установка и запуск игры

1. Установи APK (нужен телефон **arm64** с **Vulkan 1.1**, Android 9+).
2. Открой приложение → нажми **Choose ISO / default.xex** и выбери
   свою копию Sonic Generations с Xbox 360 (`.iso` диск или `default.xex`).
   Игра проверяет Title ID (`53450848`) — чужой диск не подойдёт.
3. Нажми **▶ Play**.

APK не содержит файлов игры — только движок, их ты добавляешь сам.

## Своя подпись (чтобы обновляться без переустановки)

По умолчанию каждый запуск Actions подписывает APK новым временным
ключом — такой APK ставится только начисто. Чтобы новые версии
ставились поверх старых, задай постоянный ключ один раз:

1. Создай keystore локально (пароли и alias — строго `rexauto`):
   ```bash
   keytool -genkeypair -keystore ci-release.jks -alias rexauto \
     -keyalg RSA -keysize 2048 -validity 10950 -storetype JKS \
     -storepass rexauto -keypass rexauto \
     -dname "CN=Rexauto, OU=CI, O=Rexauto, L=CI, ST=CI, C=US"
   ```
2. Закодируй в base64: `base64 -w0 ci-release.jks` (Linux) и сохрани строку.
3. В репозитории: **Settings → Secrets and variables → Actions →
   New repository secret**, имя `ANDROID_KEYSTORE_BASE64`, значение — строка
   из шага 2. **Храни файл `ci-release.jks` в надёжном месте** — без него
   обновить приложение поверх не получится.

## Локальная сборка (для разработчиков)

Нужно: JDK 17, Android SDK (`platforms;android-35`,
`build-tools;35.0.0`, `ndk;27.2.12479018`, `cmake;3.31.1`).

```bash
./android_sdk.sh            # качает ReXGlue SDK v0.10.0 + патчи (~700 МБ)
cd android
./gradlew assembleRelease \
  -PrexName=sonicgenerations \
  -PrexTitle="Sonic Generations" \
  -PrexTitleId=53450848 \
  -PrexPortDir=$PWD/../port \
  -PrexSdkDir=$PWD/sdk/rexglue-sdk
# APK: app/build/outputs/apk/release/app-release.apk
```

Ускорить пересборки: `sudo apt install ccache` и
`export REX_CCACHE=$(command -v ccache)` перед Gradle.

## Если сборка упала

- Открой упавший run в Actions, смотри лог шага **Build release APK**.
- Частые причины: кончилось место/время (перезапусти — ccache уже
  прогрет и второй проход будет быстрым), временные сбои сети при
  скачивании SDK/NDK (перезапусти run).
- В конце лога упавшего запуска есть шаг **Disk usage (on failure)** —
  покажет, что съело место.
