#!/bin/sh

#x86_64-w64-mingw32-g++ -g -Wno-deprecated-declarations demo_player.cpp -I .\\include -o demo_player.exe -static -L.\\lib -lavformat.dll -lavcodec.dll -lavutil.dll -lavfilter.dll -lswscale.dll -lavdevice.dll -lswresample.dll .\\lib\\SDL2.lib .\\lib\\SDL2main.lib -lm

#g++ -g -Wno-deprecated-declarations demo_player.cpp -I D:\\Tools\\cygwin64\\usr\\include -I .\\include -o demo_player.exe -static -L.\\lib -lavformat.dll -lavcodec.dll -lavutil.dll -lavfilter.dll -lswscale.dll -lavdevice.dll -lswresample.dll -lopencv_core.dll -lopencv_highgui.dll -lopencv_imgproc.dll -lopencv_ml.dll -lopencv_imgcodecs.dll -lm

#g++ -g -Wno-deprecated-declarations demo_player.cpp -o demo_player -lavformat -lavcodec -lavutil -lavfilter -lswscale -lavdevice -lswresample -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_ml -lopencv_imgcodecs -lopencv_features2d -lm

# fitting
g++ -g -Wno-deprecated-declarations fitting.cpp -I /usr/include/opencv4 -o fitting -L /usr/lib/x86_64-linux-gnu -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_ml -lopencv_imgcodecs -lm
