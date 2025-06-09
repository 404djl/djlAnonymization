# --- Variables ---

# Compiler and C++ Standard
CXX = g++
CXX_STD = -std=c++17 # Use a modern C++ standard

# Directories
# Assumes libAnonymization.so is in ./lib relative to this Makefile
LIB_DIR = ./lib
# Assumes Anonymization.h is in ../ relative to this Makefile
INC_DIR = ../

# Target Executable Name
TARGET = ./Anonymization_demo # Renamed to avoid confusion with the library

# Source Files (find all .cpp in the current directory)
SRCS = $(wildcard *.cpp)
# Object Files (generate .o from .cpp)
OBJS = $(SRCS:.cpp=.o)

# OpenCV Configuration (using pkg-config is recommended)
# OPENCV_LIBS = $(shell pkg-config --libs opencv4)
# OPENCV_CFLAGS = $(shell pkg-config --cflags opencv4)
# # --- If pkg-config is not available, uncomment and set these manually ---
OPENCV_LIBS = -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_dnn # Add more if needed
OPENCV_CFLAGS = -I/usr/local/include/opencv4

# Include Paths (-I)
INCLUDES = -I$(INC_DIR) $(OPENCV_CFLAGS)

# Compiler Flags (-g for debug, -Wall -Wextra for warnings)
CXXFLAGS = -g -Wall -Wextra $(CXX_STD) $(INCLUDES)

# Linker Flags (-L to specify library path, -Wl,-rpath to embed runtime path)
# -Wl,-rpath=$(LIB_DIR) makes it easier to run the demo without setting LD_LIBRARY_PATH
LDFLAGS = -L$(LIB_DIR) -Wl,-rpath=$(LIB_DIR)

# Libraries to Link (-l)
LDLIBS = -lAnonymization $(OPENCV_LIBS)

# --- Rules ---

# Default Target
all: $(TARGET)

# Link the Executable
$(TARGET): $(OBJS)
	@echo "Linking target: $@"
	$(CXX) $^ -o $@ $(LDFLAGS) $(LDLIBS)
	@echo "Successfully built $(TARGET)"

# Compile C++ Source Files (.cpp -> .o)
%.o: %.cpp
	@echo "Compiling C++: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Phony Targets
.PHONY: all clean

# Clean up build files
clean:
	@echo "Cleaning demo build files..."
	rm -f $(OBJS) $(TARGET)
	@echo "Clean complete."