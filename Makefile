#-----Macros---------------------------------

SRC_DIR?=./src
OBJ_DIR?=./obj
INSTALL_PATH?=/usr/local

INCLUDES = -I. -Iinclude

# set up compiler and options
CXX = @gcc
CXXFLAGS = -O3 -Wall $(INCLUDES) -fms-extensions

ifdef PROFILING
	CXXFLAGS += -DPROFILING
endif

ifdef DEBUG
	CXXFLAGS += -g -DDEBUG -D_DEBUG
	OUT_DIR?=./debug
else
	OUT_DIR?=./release
endif

#-----File Dependencies----------------------
SRC_FILES =	DataSet/ArffDataset.c							\
			DataSet/BatchDataset.c							\
			DataSet/Dataset.c								\
			DataSet/CSVDataset.c							\
			Inducer/FeatInducer.c							\
			Inducer/BooleanInducer.c						\
			Classifier/Classifier.c							\
			Classifier/Committee.c							\
			Classifier/Perceptron.c							\
			Util/MatrixUtil.c								\
			Util/Profiler.c
		
OBJ = $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(basename $(SRC_FILES))))
DEPS = $(OBJ:.o=.depends)

TARGET = $(OUT_DIR)/libMacLearn.a

#-----Build rules ----------------------

.PHONY : clean all

all: $(TARGET)

$(TARGET): $(OBJ)
	@echo Creating lib $@...
	@mkdir -p $(OUT_DIR)
	@ar rcs $@ $^
	@echo Done!

#-----Rules for objects and dependencies----------------------

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo $<
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.depends: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CXX) -M $(CXXFLAGS) $< -MF $@ -MT $(@:.depends=.o)

-include $(DEPS)

#-----Rules for installing, uninstalling and cleaning---------
	
clean:
	@echo Cleaning...
	@rm -rf $(OUT_DIR)
	@rm -rf $(OBJ_DIR)
	@echo Done!

install: $(TARGET)
	cp $(TARGET) $(INSTALL_PATH)/lib
	mkdir -p $(INSTALL_PATH)/include/MacLearn
	cp -r include $(INSTALL_PATH)
	
uninstall:
	rm -f $(INSTALL_PATH)/lib/libMacLearn.a
	rm -rf $(INSTALL_PATH)/include/MacLearn
