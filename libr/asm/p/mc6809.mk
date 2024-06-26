OBJ_MC6809=asm_mc6809.o

STATIC_OBJ+=${OBJ_MC6809}

TARGET_MC6809=asm_mc6809.${LIBEXT}

ALL_TARGETS+=${TARGET_MC6809}

$(OBJ_MC6809): %.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

${TARGET_MC6809}: ${OBJ_MC6809}
	${CC} $(call libname,asm_mc6809) ${LDFLAGS} \
		${CFLAGS} -o asm_mc6809.${LIBEXT} ${OBJ_MC6809}
