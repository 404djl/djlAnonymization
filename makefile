# --- Variables ---

# Compiler and C++ Standard
CXX = g++
CXX_STD = -std=c++17 # Use C++17 (or C++14, C++11 if needed)

# Directories
LOG_DIR = log
# Define the output directory
TARGET_DIR = ./test/lib

# Target Shared Library
TARGET = $(TARGET_DIR)/libAnonymization.so

# Source Files
# Find all .cpp files in the current directory (and subdirs if needed)
SRCS_CPP = $(wildcard *.cpp)
# Find all .c files in the log directory
SRCS_C = $(wildcard $(LOG_DIR)/*.c)
# Combine all sources
SRCS = $(SRCS_CPP) $(SRCS_C)

# Object Files (Generate .o from .cpp and .c)
OBJS = $(SRCS_CPP:.cpp=.o) $(SRCS_C:.c=.o)

# OpenCV Configuration (using pkg-config is recommended)
# OPENCV_CFLAGS = $(shell pkg-config --cflags opencv4)
# OPENCV_LIBS = $(shell pkg-config --libs opencv4)
# --- If pkg-config is not available, uncomment and set these manually ---
OPENCV_INCLUDE = /usr/local/include/opencv4
OPENCV_LIB_PATH = /usr/local/lib
OPENCV_LIBS_MANUAL = -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_dnn
OPENCV_CFLAGS = -I$(OPENCV_INCLUDE)
OPENCV_LIBS = -L$(OPENCV_LIB_PATH) $(OPENCV_LIBS_MANUAL)

# Include Paths
INCLUDES = -I$(LOG_DIR) -I. $(OPENCV_CFLAGS)

# Compiler Flags
# -g       : Debugging information
# -fPIC    : Position Independent Code (required for shared libraries)
# -Wall    : Enable most warnings
# -Wextra  : Enable extra warnings
# $(CXX_STD): Set C++ standard
# $(INCLUDES): Add include paths
CXXFLAGS = -g -fPIC -Wall -Wextra $(CXX_STD) $(INCLUDES)

# Linker Flags
LDFLAGS = -shared

# --- Rules ---

# Default Target
all: $(TARGET)

# Link the Shared Library
$(TARGET): $(OBJS)
	@echo "Linking target: $@"
	@mkdir -p $(TARGET_DIR) # Create the target directory if it doesn't exist
	$(CXX) $(LDFLAGS) $^ -o $@ $(OPENCV_LIBS)
	@echo "Successfully built $(TARGET)"

# Compile C++ Source Files (.cpp -> .o)
%.o: %.cpp Anonymization.h YOLOv8_face.h # Add important header dependencies
	@echo "Compiling C++: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile C Source Files (.c -> .o)
# We use g++ (CXX) here as well, it handles C code fine.
# We use CXXFLAGS, but g++ won't apply -std=c++17 to C code.
# If your C code is strict C99, you might need a separate CC=gcc rule.
%.o: %.c log/log.h # Add important header dependencies
	@echo "Compiling C  : $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Phony Targets (Targets that are not actual files)
.PHONY: all clean

# Clean up build files
clean:
	@echo "Cleaning build files..."
	rm -f $(OBJS) $(TARGET)
	@echo "Clean complete."