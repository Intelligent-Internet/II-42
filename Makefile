EXTENSION = psql_bm25s
MODULE_big = psql_bm25s

PG_CONFIG ?= pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)

ICU_CPPFLAGS := $(shell \
    if pkg-config --exists icu-i18n icu-uc 2>/dev/null; then \
        pkg-config --cflags icu-i18n icu-uc; \
    elif [ -d /opt/homebrew/opt/icu4c@78/include ]; then \
        echo -I/opt/homebrew/opt/icu4c@78/include; \
    elif [ -d /opt/homebrew/opt/icu4c/include ]; then \
        echo -I/opt/homebrew/opt/icu4c/include; \
    else \
        echo; \
    fi)

ICU_LIBS := $(shell \
    if pkg-config --exists icu-i18n icu-uc 2>/dev/null; then \
        pkg-config --libs icu-i18n icu-uc; \
    elif [ -d /opt/homebrew/opt/icu4c@78/lib ]; then \
        echo -L/opt/homebrew/opt/icu4c@78/lib \
            -licui18n -licuuc -licudata; \
    elif [ -d /opt/homebrew/opt/icu4c/lib ]; then \
        echo -L/opt/homebrew/opt/icu4c/lib \
            -licui18n -licuuc -licudata; \
    else \
        echo -licui18n -licuuc -licudata; \
    fi)

HOST_CPU_AVX2_STATUS := $(shell \
    if command -v lscpu >/dev/null 2>&1; then \
        lscpu 2>/dev/null | awk '\
            /Flags:/ { \
                for (i = 2; i <= NF; i++) { \
                    if ($$i == "avx2") { \
                        print "yes"; \
                        found = 1; \
                        exit; \
                    } \
                } \
            } \
            END { \
                if (!found) { \
                    print "no"; \
                } \
            }'; \
    elif command -v sysctl >/dev/null 2>&1; then \
        if sysctl -n machdep.cpu.features machdep.cpu.leaf7_features \
            2>/dev/null | tr '[:upper:]' '[:lower:]' | grep -qw avx2; then \
            echo yes; \
        else \
            echo no; \
        fi; \
    else \
        echo unknown; \
    fi)

ifeq ($(PSQL_BM25S_FORCE_SCALAR),1)
PG_CPPFLAGS += -DPSQL_BM25S_FORCE_SCALAR=1
$(warning Building psql_bm25s with AVX2 dispatch disabled; query scoring \
may be slower on this machine.)
else ifeq ($(HOST_CPU_AVX2_STATUS),no)
$(warning Building psql_bm25s on a host without AVX2 support; the \
extension will build and run, but this machine will use the scalar \
scoring path and may be slower.)
else ifeq ($(HOST_CPU_AVX2_STATUS),unknown)
$(warning Could not detect host AVX2 support; psql_bm25s will still \
build, but may fall back to the scalar scoring path on this machine.)
endif

OBJS = \
    src/psql_bm25s_am.o \
    src/psql_bm25s_pg.o \
    src/psql_bm25s_pg_common.o \
    src/psql_bm25s_core.o \
    src/psql_bm25s_query.o \
    src/psql_bm25s_storage.o \
    src/psql_bm25s_stem.o
OBJS += src/psql_bm25s_text.o

DATA = $(wildcard sql/psql_bm25s--*.sql)
REGRESS = psql_bm25s_integration

PG_CPPFLAGS = -I$(CURDIR)/src $(ICU_CPPFLAGS)
SHLIB_LINK += $(ICU_LIBS)

include $(PGXS)
