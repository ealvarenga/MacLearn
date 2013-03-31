#-----Macros---------------------------------
MKLROOT = /opt/intel/mkl

LibsIntel =  -Wl,--start-group  $(MKLROOT)/lib/intel64/libmkl_intel_lp64.a $(MKLROOT)/lib/intel64/libmkl_gnu_thread.a $(MKLROOT)/lib/intel64/libmkl_core.a -Wl,--end-group -ldl -lpthread -lm
OptsIntel =  -fopenmp -m64 -I$(MKLROOT)/include

LibsGSL = -lgslcblas
OptsGSL = 

LibsBLAS = $(LibsIntel)
OptsBLAS = $(OptsIntel)

LIBS = -lMacLearn -lgsl -lrt $(LibsBLAS)
INCLUDES = 

# set up compiler and options
CXX = @gcc
CXXFLAGS = -O3 -Wall $(INCLUDES) -fms-extensions $(OptsBLAS)

#-----Suffix Rules---------------------------
# set up C++ suffixes and relationship between .cc and .o files

.SUFFIXES: .c.cpp

.c.o:
	@echo $<
	$(CXX) $(CXXFLAGS) -c $< -o $@

.cpp.o:
	@echo $<
	$(CXX) $(CXXFLAGS) -c $< -o $@

.c :
	@echo $<
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY : clean all

all: digits conversion binops

digits: Digits/Digits.o
	@echo Linking $@...
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
	@echo Done!

conversion: Conversion/Conversion.o
	@echo Linking $@...
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
	@echo Done!

binops: BinOps/BinOps.o
	@echo Linking $@...
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
	@echo Done!

#-----Other stuff----------------------------
clean:
	@echo Cleaning...
	@rm -f digits conversion binops
	@rm -f Digits/Digits.o Conversion/Conversion.o BinOps/BinOps.o
