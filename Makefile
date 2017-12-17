# Directories
SRCDIR := src
BUILDDIR := build
RUNDIR := bin
TESTDIR := test

# Sources
TARGET := sam_sampler
SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name '*.$(SRCEXT)')
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))

# Compiler
CC := g++
CFLAGS := -g -std=c++11 -Wall -pedantic
LIB := -lopencv_core -lopencv_imgcodecs -lopencv_video -lopencv_imgproc -lopencv_videoio -lopencv_highgui -lpthread
INC?="-I/usr/local/include/opencv -I/usr/local/include"


$(RUNDIR)/$(TARGET): $(OBJECTS)
	@echo "Linking..."
	@mkdir -p $(RUNDIR)
	@echo "  $(CC) $^ -o $(RUNDIR)/$(TARGET) $(LIB:%=% )"; $(CC) $^ -o $(RUNDIR)/$(TARGET) $(LIB:%=% )

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@echo "Compile: $@"
	@mkdir -p $(BUILDDIR)
	@echo "  $(CC) $(CFLAGS:%=% ) $(INC:%=% ) -c -o $@ $<"; $(CC) $(CFLAGS:%=% ) $(INC:%=% ) -c -o $@ $<

clean:
	@echo "Cleaning...";
	@echo "  $(RM) -r $(BUILDDIR) $(RUNDIR)"; $(RM) -r $(BUILDDIR) $(RUNDIR) || :

test: $(RUNDIR)/$(TARGET) test/run_with_test_video.py
	@python test/run_with_test_video.py $(RUNDIR)/$(TARGET)

.PHONY: clean
