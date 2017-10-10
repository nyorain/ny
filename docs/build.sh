#!/usr/bin/env bash
# script helping with packaging and installing native 
# android app.
# Most helpful source: http://www.hanshq.net/command-line-android.html

# signing the apk requires that key was locally initialized like this 
# keytool -genkeypair -keystore keystore.jks -alias androidkey \
      # -validity 10000 -keyalg RSA -keysize 2048 \
      # -storepass android -keypass android

SDK="/opt/android-sdk"
BT="${SDK}/build-tools/25.0.2"
PLATFORM="${SDK}/platforms/android-24"

NAME="ny_basic"

# generate the unsigned apk
"${BT}/aapt" package -f -M AndroidManifest.xml \
      -I "${PLATFORM}/android.jar" \
      -F build/${NAME}.unsigned.apk build/apk/

# add the libraries
"${BT}/aapt" add build/${NAME}.unsigned.apk lib/*/*

# sign the apk
"${BT}/apksigner" sign --ks keystore.jks \
      --ks-key-alias androidkey --ks-pass pass:android \
      --key-pass pass:android --out build/${NAME}.apk \
      build/${NAME}.unsigned.apk

# install
adb install -r build/${NAME}.apk
adb logcat -s "ny" # as long as there are no AppContextSettings
