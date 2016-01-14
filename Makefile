# Makefile

# Define the CPU and TOOL
CPU = CPU32
TOOL = gnu

# Include all the stadard files required by tornado and the rules file
include $(WIND_BASE)/target/h/make/defs.bsp
include $(WIND_BASE)/target/h/make/make.$(CPU)$(TOOL)
include $(WIND_BASE)/target/h/make/defs.$(WIND_HOST_TYPE)
# include $(PROJECT)\Tools\rules
include rules.spi

# List of object files to be made
OBJECTS = \
	spiLib.o \
	spiGlobal.o \
	spiLtc1598.o \
	spiTempSensor.o

# Include files.
EXTRA_INCLUDE	= \
	-I$(PROJECT)/CP/Mux/Hdr \
	-I$(PROJECT)/CP/360/Hdr \
	-I$(PROJECT)/CP/360/Config

# Name of the library to be created - This has the format $(Task).a
LIBRARY = Spi.a

# Default rule to Tornado's make
# Compile the specified source files, make a library, move it to 'Lib'
# directory 

exe : $(OBJECTS) $(LIBRARY) 
	$(MV) $(LIBRARY) ..\Lib


