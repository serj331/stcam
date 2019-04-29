##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Release
ProjectName            :=stcam
ConfigurationName      :=Release
WorkspacePath          :=/home/serj/codez/work1
ProjectPath            :=/home/serj/codez/work1/stcam
IntermediateDirectory  :=./Release
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=serj
Date                   :=04/29/19
CodeLitePath           :=/home/serj/.codelite
LinkerName             :=/opt/arm-rpi-4.9.3-linux-gnueabihf/bin/arm-linux-gnueabihf-g++
SharedObjectLinkerName :=/opt/arm-rpi-4.9.3-linux-gnueabihf/bin/arm-linux-gnueabihf-g++ -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=$(PreprocessorSwitch)NDEBUG 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="stcam.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch)/home/serj/rpi/libusb_build/include $(IncludeSwitch)/home/serj/rpi/opencv_lib/include/opencv4 $(IncludeSwitch)/home/serj/rpi/jpeg-9c-lib/include 
IncludePCH             := 
RcIncludePath          := 
Libs                   := $(LibrarySwitch)usb-1.0 $(LibrarySwitch)opencv_core $(LibrarySwitch)opencv_imgproc $(LibrarySwitch)jpeg $(LibrarySwitch)pthread 
ArLibs                 :=  "libusb-1.0" "libopencv_core" "libopencv_imgproc" "libjpeg" "pthread" 
LibPath                := $(LibraryPathSwitch). $(LibraryPathSwitch)/home/serj/rpi/libusb_build/lib $(LibraryPathSwitch)/home/serj/rpi/opencv_lib/lib $(LibraryPathSwitch)/home/serj/rpi/jpeg-9c-lib/lib 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /opt/arm-rpi-4.9.3-linux-gnueabihf/bin/arm-linux-gnueabihf-ar rcu
CXX      := /opt/arm-rpi-4.9.3-linux-gnueabihf/bin/arm-linux-gnueabihf-g++
CC       := /opt/arm-rpi-4.9.3-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc
CXXFLAGS :=  -O2 -std=c++11 -Wall $(Preprocessors)
CFLAGS   :=  -O2 -Wall -std=gnu99 $(Preprocessors)
ASFLAGS  := 
AS       := /opt/arm-rpi-4.9.3-linux-gnueabihf/bin/arm-linux-gnueabihf-as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IntermediateDirectory)/iron_pallet.c$(ObjectSuffix) $(IntermediateDirectory)/httpst.c$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

MakeIntermediateDirs:
	@test -d ./Release || $(MakeDirCommand) ./Release


$(IntermediateDirectory)/.d:
	@test -d ./Release || $(MakeDirCommand) ./Release

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/main.cpp$(ObjectSuffix): main.cpp $(IntermediateDirectory)/main.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/serj/codez/work1/stcam/main.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/main.cpp$(DependSuffix): main.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/main.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/main.cpp$(DependSuffix) -MM main.cpp

$(IntermediateDirectory)/main.cpp$(PreprocessSuffix): main.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/main.cpp$(PreprocessSuffix) main.cpp

$(IntermediateDirectory)/iron_pallet.c$(ObjectSuffix): iron_pallet.c $(IntermediateDirectory)/iron_pallet.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/serj/codez/work1/stcam/iron_pallet.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/iron_pallet.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/iron_pallet.c$(DependSuffix): iron_pallet.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/iron_pallet.c$(ObjectSuffix) -MF$(IntermediateDirectory)/iron_pallet.c$(DependSuffix) -MM iron_pallet.c

$(IntermediateDirectory)/iron_pallet.c$(PreprocessSuffix): iron_pallet.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/iron_pallet.c$(PreprocessSuffix) iron_pallet.c

$(IntermediateDirectory)/httpst.c$(ObjectSuffix): httpst.c $(IntermediateDirectory)/httpst.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/serj/codez/work1/stcam/httpst.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/httpst.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/httpst.c$(DependSuffix): httpst.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/httpst.c$(ObjectSuffix) -MF$(IntermediateDirectory)/httpst.c$(DependSuffix) -MM httpst.c

$(IntermediateDirectory)/httpst.c$(PreprocessSuffix): httpst.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/httpst.c$(PreprocessSuffix) httpst.c


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Release/


