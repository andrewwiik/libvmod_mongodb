AM_CFLAGS = $(VARNISHAPI_CFLAGS) $(MONGOC_CFLAGS) -Wall -Werror
AM_LDFLAGS = $(VARNISHAPI_LIBS) $(MONGOC_LIBS) $(VMOD_LDFLAGS)

vmod_LTLIBRARIES = libvmod_mongodb.la

libvmod_mongodb_la_SOURCES = \
	vmod_mongodb.c

nodist_libvmod_mongodb_la_SOURCES = \
	vcc_mongodb_if.c \
	vcc_mongodb_if.h

dist_man_MANS = vmod_mongodb.3

@BUILD_VMOD_MONGODB@

AM_TESTS_ENVIRONMENT = \
	PATH="$(VMOD_TEST_PATH)" \
	LD_LIBRARY_PATH="$(VARNISH_LIBRARY_PATH)"
TEST_EXTENSIONS = .vtc
VTC_LOG_COMPILER = varnishtest -v
AM_VTC_LOG_FLAGS = -Dvmod_mongodb="$(VMOD_MONGODB)"

TESTS = @VMOD_TESTS@

EXTRA_DIST = \
	vmod_mongodb.vcc \
	$(VMOD_TESTS)
