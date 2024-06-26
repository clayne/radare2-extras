OBJ_BA2=asm_ba2.o
CFLAGS_BA2=$(CFLAGS)
CFLAGS_BA2+=-I../arch/ba2/

STATIC_OBJ+=${OBJ_BA2}
TARGET_BA2=asm_ba2.${LIBEXT}

ALL_TARGETS+=${TARGET_BA2}

$(OBJ_BA2): %.o : %.c
	$(CC) -c $(CFLAGS_BA2) $< -o $@

${TARGET_BA2}: ${OBJ_BA2}
	${CC} -lr_util ${CFLAGS_BA2} -o ${TARGET_BA2} ${OBJ_BA2}
