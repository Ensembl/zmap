#
# Build the aceconn package when a zmap make is executed.
#

include $(top_srcdir)/Makefile.common

SUBDIRS = .

ACECONN = AceConn
ACECONN_DIR = $(ACECONN)

all-local:
	@echo "Started building $(ACECONN)...."
	@cd $(top_srcdir)/libAceConn/$(ACECONN_DIR) ; \
	if ! test -f Makefile ; then \
	echo 'No Makefile, creating Makefile....' ; \
	autogen.sh ; \
	./configure --prefix=$(abs_top_builddir) ; \
	fi ; \
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



