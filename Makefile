# Makefile to package SDF
# By Steven Smith

CC= clang
ANALYZE= clang --analyze
WARNS= -W -Werror -Wall
CFLAGS= -g $(WARNS)
LFLAGS= 
# Frameworks are a part of OS X compilation
FRAMEWORKS= 
LIBS= 
SRC= src
BUILD= build

# Sources
SDF_S= sdf.c
SDF_H= sdf.h

# Test sources
SDF_T_S= sdf_test.c

# Test executable
SDF_T_E= sdftest

# Tarball archive
SDF_AR= sdf.tar

# Helper function
getobjs= $(patsubst %.c,$(BUILD)/%.o,$(1))
cond_= $(if $(1), $(2) $(1))
cond= $(call cond_, $(wildcard $(1)), $(2))

# Derivative values
FWS= $(foreach fw,$(FRAMEWORKS),-framework $(fw))
LBS= $(foreach lb,$(LIBS),-l$(lb))

package: $(SDF_AR)

test: $(SDF_T_E)

$(SDF_T_E): $(call getobjs, $(SDF_S) $(SDF_T_S))
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^ $(LBS) $(FWS)

$(BUILD)/$(SDF_H): $(SRC)/$(SDF_H) | $(BUILD)
	cp $^ $@

$(SDF_AR): $(call getobjs, $(SDF_S)) $(BUILD)/$(SDF_H)
	tar -cf $@ -C$(BUILD) $(notdir $^)

$(BUILD):
	mkdir $(BUILD)

$(BUILD)/%.o: $(SRC)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -I $(SRC) -c -o $@ $<

clean:
	$(call cond, $(BUILD)/*.o, $(RM))
	$(call cond, $(BUILD)/$(SDF_H), $(RM))
	$(call cond, $(BUILD), rmdir)
	$(call cond, $(SDF_AR), $(RM))

.PHONY: clean

