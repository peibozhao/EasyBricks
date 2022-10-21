#!/usr/bin/env bash

adb shell screencap -p > test.png
ffmpeg -y -v error -i test.png test.jpg
rm test.png
./paddleocr.py test.jpg
rm test.jpg
xdg-open output.jpg

