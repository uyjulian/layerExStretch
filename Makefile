
INCFLAGS += -Iexternal/simde

SOURCES += main.cpp
SOURCES += ComplexRect.cpp
SOURCES += ResampleImage.cpp
SOURCES += WeightFunctor.cpp
SOURCES += ResampleImageSSE2.cpp
SOURCES += x86simdutil.cpp
SOURCES += roundevenf.c
SOURCES += LayerBitmapUtility.cpp
SOURCES += tvpgl.cpp
PROJECT_BASENAME = layerExStretch

RC_LEGALCOPYRIGHT ?= Copyright (C) 2021-2021 Julian Uy; See details of license at license.txt, or the source code location.

include external/ncbind/Rules.lib.make
