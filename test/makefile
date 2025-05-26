INCLUDE_PATH = ../
CC = g++

LIB_PATH = ./lib
TARGET = ./Anonymization

CFLAGS = -g -Wall
LDFLAGS = -L$(LIB_PATH) -lAnonymization -lopencv_calib3d -lopencv_core -lopencv_dnn -lopencv_features2d -lopencv_flann -lopencv_gapi -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_ml -lopencv_objdetect -lopencv_photo -lopencv_stitching -lopencv_video -lopencv_videoio
SRCS = $(wildcard *.c)
OBJS = *.o

.PHONY: target clean

target: $(SRCS)
	$(CC) -w -c $(SRCS) $(CFLAGS)  -std=c99
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)
	rm -f $(OBJS)

clean:
	rm -f $(OBJS) $(TARGET)