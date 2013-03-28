#
# Build the aceconn package when a zmap make is executed.
#

include $(top_srcdir)/Makefile.common

SUBDIRS = .

ACECONN = AceConn
ACECONN_DIR = $(ACECONN)


# Sadly this does not work: ./configure --prefix=$(abs_top_builddir)
# because abs_top_builddir is only found in autoconf 2.59 onwards...
# so I do a hack using the current directory....
all-local:
	@echo "Started building $(ACECONN)...."
	@export TMP_BUILD_DIR=`dirname $(PWD)` ; \
	cd $(top_srcdir)/libAceConn/$(ACECONN_DIR) ; \
	cd $(top_srcdir)/libAceConn/$(ACECONN_DIR) ; \
	echo 'No Makefile, creating Makefile....' ; \
	./autogen.sh ; \
	./configure --prefix=$$TMP_BUILD_DIR ; \
	echo "Running make for $(ACECONN)" ; \
	make ; \
	make install
	@echo "Finished building $(ACECONN)...."


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/libAceConn/Makefile.in


# We really need a maintainer-clean-local target here to clear up aceconn....
maintainer-clean-local:
	@echo "maintainer-clean-local: Cleaning up $(ACECONN)...."
	@cd $(top_srcdir)/libAceConn/$(ACECONN_DIR) ; \
	make maintainer-clean
	@echo "maintainer-clean-local: Finished cleaning up $(ACECONN)...."



