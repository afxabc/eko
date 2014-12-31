# Add inputs and outputs from these tool invocations to the build variables 

-include ../common.mak

TARGET = $(TARGETPATH)/test

CPP_SRCS = $(wildcard ./*.cpp)
TEMP = $(subst .cpp,.o,$(CPP_SRCS))
OBJS = $(subst ./,$(OUTPATH)/,$(TEMP))
CPP_DEPS = $(subst .o,.d,$(OBJS))

#boost
INCPATH += \
-I../../ \
-I../ 

LIBPATH := \
-L"$(TARGETPATH)" 

LIBS := \
-lbase \
#-lrt \
#-lpthread 

GCC := $(CROSSBUILD)g++
AR := $(CROSSBUILD)ar

# Each subdirectory must supply rules for building sources it contributes
$(OUTPATH)/%.o $(OUTPATH)/%.d: ./%.cpp
	@echo 'Building file: $<'
	$(GCC) $(INCPATH) $(FLAGS) -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
	

# All Target
all: $(TARGET)

# Tool invocations
$(TARGET): $(OBJS) 
	echo 'Building target: $@'
	$(GCC) -o $(TARGET) $(OBJS) $(LIBPATH) $(LIBS) $(LFLAGS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OUTPATH)/*.o $(OUTPATH)/*.d $(TARGET)
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
