#
# Build the aceconn package when a zmap make is executed.
#

include $(top_srcdir)/Makefile.common

SUBDIRS = .

ACECONN = AceConn
ACECONN_DIR = $(ACECONN)

all-local:
	@echo "Starting building $(ACECONN)...."
	@cd $(top_srcdir)/libAceConn/$(ACECONN_DIR) ; \
	if ! test -f Makefile ; then \
	echo 'No Makefile, creating Makefile....' ; \
	autoreconf -i -v ; \
	./configure --prefix=$(abs_top_builddir) ; \
	fi ; \
	echo "Running make for $(ACECONN)" ; \
	make ; \
	make install
	@echo "Finished building $(ACECONN)...."
