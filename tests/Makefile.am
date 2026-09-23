EXTRA_DIST = README.md unit-tests.sh proxy-tests.sh LICENSE

# In a transport-only build (configure --enable-transport-only) the native socket
# constructors are compiled out, so the socket-using tests can't link. Build only
# the transport-only loopback test, which drives two transport-only contexts
# through a socketpair.
if TRANSPORT_ONLY
noinst_PROGRAMS = unit-test-transport-loopback
TESTS = unit-test-transport-loopback
else
noinst_PROGRAMS = \
	bandwidth-server-one \
	bandwidth-server-many-up \
	bandwidth-client \
	random-test-server \
	random-test-client \
	unit-test-server \
	unit-test-client \
	unit-test-transport \
	unit-test-transport-only \
	proxy-test-server \
	proxy-test-client \
	version
noinst_SCRIPTS = unit-tests.sh
TESTS = ./unit-tests.sh unit-test-transport unit-test-transport-only
endif

common_ldflags = \
	$(top_builddir)/src/libmodbus.la

unit_test_transport_loopback_SOURCES = unit-test-transport-loopback.c
unit_test_transport_loopback_LDADD = $(common_ldflags)

bandwidth_server_one_SOURCES = bandwidth-server-one.c
bandwidth_server_one_LDADD = $(common_ldflags)

bandwidth_server_many_up_SOURCES = bandwidth-server-many-up.c
bandwidth_server_many_up_LDADD = $(common_ldflags)

bandwidth_client_SOURCES = bandwidth-client.c
bandwidth_client_LDADD = $(common_ldflags)

random_test_server_SOURCES = random-test-server.c
random_test_server_LDADD = $(common_ldflags)

random_test_client_SOURCES = random-test-client.c
random_test_client_LDADD = $(common_ldflags)

unit_test_server_SOURCES = unit-test-server.c unit-test.h
unit_test_server_LDADD = $(common_ldflags)

unit_test_client_SOURCES = unit-test-client.c unit-test.h
unit_test_client_LDADD = $(common_ldflags)

unit_test_transport_SOURCES = unit-test-transport.c
unit_test_transport_LDADD = $(common_ldflags)

unit_test_transport_only_SOURCES = unit-test-transport-only.c
unit_test_transport_only_LDADD = $(common_ldflags)

proxy_test_server_SOURCES = proxy-test-server.c
proxy_test_server_LDADD = $(common_ldflags)

proxy_test_client_SOURCES = proxy-test-client.c unit-test.h
proxy_test_client_LDADD = $(common_ldflags)

version_SOURCES = version.c
version_LDADD = $(common_ldflags)

AM_CPPFLAGS = \
    -include $(top_builddir)/config.h \
    -DSYSCONFDIR=\""$(sysconfdir)"\" \
    -DLIBEXECDIR=\""$(libexecdir)"\" \
    -I${top_srcdir}/src \
    -I${top_builddir}/src

AM_CFLAGS = $(LIBMODBUSCFLAGS) $(WARNING_CFLAGS)

CLEANFILES = *~ *.log
