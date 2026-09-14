sonicgenerations -- Android project sources
android/   launcher app (Gradle project)
port/      recompiled game (generated C++ + src + *.toml)
Build: run tools/android_sdk.sh (needs android/sdk), then
  cd android && ./gradlew assembleRelease -PrexName=sonicgenerations     -PrexPortDir=$PWD/../port -PrexSdkDir=$PWD/sdk/rexglue-sdk
