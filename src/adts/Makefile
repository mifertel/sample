# ======================================================
# Makefile
#    |
#    +----- Makefile.files // Source Code
#    +----- Makefile.rules // Common compulation rules
#
# ======================================================


# Files specified separetely.
# ======================================================
include Makefile.files.mk
H_FILES += $(xH_FILES)
C_FILES += $(xC_FILES)


# Rules are common to all compilation units.	
# ======================================================
include ../Makefile.rules.mk
CC      = $(xCC)
CFLAGS += $(xCFLAGS)
CFLAGS += -c    # Compile only - No linking
CFLAGS += -fPIC # Position independent code - dynamic linking | shared library


# Everything below this line is standard Makefile logic:
# ======================================================
DEPS    += $(H_FILES)
OBJECTS += $(patsubst %.c,%.o,$(C_FILES))    


# Declare non-files
# ======================================================
.PHONY: all 


# "all" should always be the first defined target to ensure default build
# ======================================================
all: clean \
     build   \
     cleanup

clean: 
	@echo ""
	@echo "Clean:"
	@echo "======"
	rm -fr *.[i,s,o]
	rm -fr *.bc
	rm -fr bin/

build:
	@echo ""
	@echo "Compile Shared Library:"
	@echo "======================="
	$(CC) -I ${PWD} $(CFLAGS) $(C_FILES) 
	$(CC) -I ${PWD} -shared -o libadts.so $(OBJECTS) -lrt

cleanup:
	@echo ""
	@echo "Cleanup:"
	@echo "========"
	mkdir -p bin
	mv *.[i,s,o,] bin/
	mv *.bc bin/
	mv *.so bin/
