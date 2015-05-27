#!/bin/bash

rm out.png
ffmpeg -vcodec rawvideo -f rawvideo -pix_fmt nv12 -s 1024x768 -i out.bin -f image2 -pix_fmt rgb24 out.png || true
feh ./out.png
