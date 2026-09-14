sonicgenerations -- Android project sources
android/   launcher app (Gradle project)
port/      recompiled game (generated C++ + src + *.toml)

Easiest path: push to GitHub, Actions builds the APK for you
(.github/workflows/android.yml -> artifact "SonicGenerations-android";
tags v*.*.* also publish a GitHub Release). Step-by-step: BUILD_ANDROID.md.

Local build: ./android_sdk.sh
  (needs JDK 17 + Android SDK: platform-35, build-tools 35.0.0,
   NDK 27.2.12479018, CMake 3.31.1), then:
  cd android && ./gradlew assembleRelease -PrexName=sonicgenerations \
    -PrexTitle="Sonic Generations" -PrexTitleId=53450848 \
    -PrexPortDir=$PWD/../port -PrexSdkDir=$PWD/sdk/rexglue-sdk
