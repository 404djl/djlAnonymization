CXX = g++
CXX_STD = -std=c++17
TARGET_EXEC = anonymization_tester

# Path to your Anonymization SDK header and library
# Adjust if your test harness is not in the same dir as your demo
SDK_INC_DIR = ../  # Assuming Anonymization.h is in the parent dir
SDK_LIB_DIR = ./lib # Assuming libAnonymization.so is there

SRCS = $(wildcard *.cpp) # Assuming test harness is one .cpp file
OBJS = $(SRCS:.cpp=.o)

# OPENCV_CFLAGS = $(shell pkg-config --cflags opencv4)
# OPENCV_LIBS = $(shell pkg-config --libs opencv4)
OPENCV_LIBS = -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_dnn # Add more if needed
OPENCV_CFLAGS = -I/usr/local/include/opencv4

INCLUDES = -I$(SDK_INC_DIR) $(OPENCV_CFLAGS)
CXXFLAGS = -g -Wall -Wextra $(CXX_STD) $(INCLUDES) -pthread # -pthread if chrono uses it

LDFLAGS = -L$(SDK_LIB_DIR) -Wl,-rpath=$(SDK_LIB_DIR)
LDLIBS = -lAnonymization $(OPENCV_LIBS)

all: $(TARGET_EXEC)

$(TARGET_EXEC): $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET_EXEC) test_results_log.csv
	rm -rf output_results/