# Parameters
# SRC_CS: The source C files to compile
# SRC_CPPS: The source CPP files to compile
# EXEC: The executable name

ifeq ($(SRC_CS) $(SRC_CPPS),)
  $(error No source files specified)
endif

ifeq ($(EXEC),)
  $(error No executable file specified)
endif

CC                  ?= gcc
CXX                 ?= g++

OPENCV_ROOT	    ?= /usr/local
PUREGEV_ROOT        ?= ../../..
PV_LIBRARY_PATH      =$(PUREGEV_ROOT)/lib

CFLAGS              += -I$(PUREGEV_ROOT)/include -I$(OPENCV_ROOT)/include -I$(MESASR_ROOT)/include
CPPFLAGS            += -I$(PUREGEV_ROOT)/include -I$(OPENCV_ROOT)/include -I$(MESASR_ROOT)/include
ifdef _DEBUG
    CFLAGS    += -g -D_DEBUG
    CPPFLAGS  += -g -D_DEBUG
else
    CFLAGS    += -O3
    CPPFLAGS  += -O3
endif
CFLAGS    += -D_UNIX_ -D_LINUX_
CPPFLAGS  += -D_UNIX_ -D_LINUX_

LDFLAGS             += -L$(PUREGEV_ROOT)/lib -L$(OPENCV_ROOT)/lib -L$(MESASR_ROOT)/lib       \
                        -lPvBase                     \
                        -lPvDevice                   \
                        -lPvBuffer                   \
                        -lPvGUIUtils                 \
                        -lPvPersistence              \
                        -lPvGenICam                  \
                        -lPvStreamRaw                \
                        -lPvStream                   \
                        -lPvTransmitterRaw           \
                        -lPvVirtualDevice	     \
			-lopencv_core		     \
			-lopencv_highgui  \
			-lmesasr


# Conditional linking and usage of the GUI on the sample only when available
ifeq ($(wildcard $(PUREGEV_ROOT)/lib/libPvGUI.so),)
    CFLAGS    += -DPV_GUI_NOT_AVAILABLE
    CPPFLAGS  += -DPV_GUI_NOT_AVAILABLE
else
    LDFLAGS   += -lPvGUI
endif 

# We want all samples to prevent using deprecated eBUS SDK functions, macros and types
CFLAGS    += -DPV_NO_GEV1X_PIXEL_TYPES -DPV_NO_DEPRECATED_PIXEL_TYPES
CPPFLAGS  += -DPV_NO_GEV1X_PIXEL_TYPES -DPV_NO_DEPRECATED_PIXEL_TYPES

# Configure Genicam
GEN_LIB_PATH = $(PUREGEV_ROOT)/lib/genicam/bin/Linux32_i86
LDFLAGS      += -L$(GEN_LIB_PATH)


# Configure Qt compilation if any
SRC_MOC              =
MOC			         =
RCC					 =
FILES_QTGUI          = $(shell grep -l QtGui *)
ifneq ($(FILES_QTGUI),)
	# This is a sample compiling Qt code
    HAVE_QT=$(shell which qmake-qt4 &>/dev/null ; echo $?)
    ifeq ($(HAVE_QT),1)
		# We cannot compile the sample without the Qt SDK!
 		$(error The sample $(EXEC) requires the Qt SDK to be compiled. See share/samples/Readme.txt for more details)
    endif

	# Query qmake to find out the folder required to compile
	QT_SDK_BIN        = $(shell qmake-qt4 -query QT_INSTALL_BINS)
	QT_SDK_LIB        = $(shell qmake-qt4 -query QT_INSTALL_LIBS)
	QT_SDK_INC        = $(shell qmake-qt4 -query QT_INSTALL_HEADERS)
	
	# We have a full Qt SDK installed, so we can compile the sample
	CFLAGS 	         += -I$(QT_SDK_INC)
	CPPFLAGS         += -I$(QT_SDK_INC)
	LDFLAGS          += -L$(QT_SDK_LIB)

    QT_LIBRARY_PATH   = $(QT_SDK_LIB)

    FILES_MOC            = $(shell grep -l Q_OBJECT *)
    ifneq ($(FILES_MOC),)
	    SRC_MOC           = $(FILES_MOC:%h=moc_%cxx)
	    FILES_QRC         = $(shell ls *.qrc)
	    SRC_QRC           = $(FILES_QRC:%qrc=qrc_%cxx)

	    OBJS             += $(SRC_MOC:%.cxx=%.o)
	    OBJS		     += $(SRC_QRC:%.cxx=%.o)

        MOC               = $(QT_SDK_BIN)/moc
  	    RCC               = $(QT_SDK_BIN)/rcc
    endif
endif

# Configure FFmpeg compilation if any
FILES_FFMPEG              = $(shell grep -l avcodec *)
ifneq ($(FILES_FFMPEG),)
    HAVE_FFMPEG = $(shell which ffmpeg &>/dev/null && echo "1")
    ifneq ($(HAVE_FFMPEG),1)
        # We cannot compile the sample without the ffmpeg!
 		$(error The sample $(EXEC) requires ffmpeg to be compiled. See share/samples/Readme.txt for more details)  
    endif

    # Ensure the proper version of ffmpeg
    FFMPEG_VERSION=$(shell ffmpeg -version)) 
    ifeq (,$(findstring ffmpeg version 0.11.,$(FFMPEG_VERSION)))
	    $(error The sample $(EXEC) requires FFmpeg 0.11.x to be compiled. See share/samples/Readme.txt for more details)
    endif

    # We need to add the library for ffmpeg and we assumed that the PC is configure the find them...
    LDFLAGS          += -lavformat			\
                        -lavcodec 			\
                        -lswscale  
endif

LD_LIBRARY_PATH       = $(PV_LIBRARY_PATH):$(QT_LIBRARY_PATH):$(GEN_LIB_PATH)
export LD_LIBRARY_PATH

OBJS      += $(SRC_CPPS:%.cpp=%.o)
OBJS      += $(SRC_CS:%.c=%.o)

all: $(EXEC)

clean:
	rm -rf $(OBJS) $(EXEC) $(SRC_MOC) $(SRC_QRC)

moc_%.cxx: %.h
	$(MOC) $< -o $@ 

qrc_%.cxx: %.qrc
	$(RCC) $< -o $@

%.o: %.cxx
	$(CXX) -c $(CPPFLAGS) -o $@ $<

%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(EXEC): $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $@

.PHONY: all clean
