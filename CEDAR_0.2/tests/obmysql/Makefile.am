AM_CPPFLAGS = -I${TBLIB_ROOT}/include/tbsys \
			  -I${ONEV_ROOT}/include/onev \
			  -I${top_srcdir}/include \
			  -I${top_srcdir}/src

LDADD = $(top_builddir)/src/obmysql/libobmysql.a \
                $(top_builddir)/src/common/libcommon.a \
		${ONEV_LIB_PATH}/libonev.a\
		${TBLIB_ROOT}/lib/libtbsys.a

AM_LDFLAGS = -lpthread -lc -lm -lrt -lgtest -ldl ${GCOV_LIB} -laio
CXXFLAGS += -D__GNU_SOURCE
if COVERAGE
CXXFLAGS+=-fprofile-arcs -ftest-coverage
AM_LDFLAGS+=-lgcov
endif

bin_PROGRAMS = test_ob_mysql_state test_command_packet

test_command_packet_SOURCES = test_ob_mysql_command_packet.cpp
test_ob_mysql_state_SOURCES = test_ob_mysql_state.cpp
