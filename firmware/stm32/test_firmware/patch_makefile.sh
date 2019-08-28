#!/bin/bash

echo "Disabling hardware FPU..."
sed -i -e 's/-mfloat-abi=hard/-mfloat-abi=soft/' Makefile

echo "Switching to build with clang..."
sed -i -e 's/$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)\/$(notdir $(<:.c=.lst)) $< -o $@/$(LLVM_CC) -c -target arm-none-eabi $(CLANG_FLAGS) $(CFLAGS) $< -o $@/' Makefile

echo "Copying GR files into source..."
cp -n ../../../instrumenter/instrumentation_snippets/*.c Src/

for f in ../../../instrumenter/instrumentation_snippets/*.c;
do
	f="$(basename -- $f)"
	if ! grep -q "$f" Makefile; then
		echo "Adding $f to Makefile...";
		sed -i -e "/C_SOURCES =  \\\\/a\\
Src\\/$f \\\\
" Makefile
	fi
done;

echo "Adding source rewriter..."
if ! grep -q "GR_ENUM_ARG" Makefile; then
	sed  -i -e '/# Generate dependency information/i \
GR_ENUM_ARG := $(addprefix -extra-arg-before=,${CFLAGS})

' Makefile

	sed -i -e $'/$(LLVM_CC)/i \
\\\tgp-source-rewriter $(GR_ENUM_ARG) $(GR_ENUM_FLAGS) $<
' Makefile

fi


#C_SOURCES =  \
