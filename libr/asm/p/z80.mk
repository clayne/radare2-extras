OBJ_Z80=arch_z80.o

STATIC_OBJ+=${OBJ_Z80}
TARGET_Z80=arch_z80.${LIBEXT}

ifeq ($(WITHPIC),1)
ALL_TARGETS+=${TARGET_Z80}

${TARGET_Z80}: ${OBJ_Z80}
	${CC} $(call libname,arch_z80) ${LDFLAGS} ${CFLAGS} -o ${TARGET_Z80} ${OBJ_Z80}
endif
