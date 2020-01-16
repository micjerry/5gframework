-include modules.inc

all: $(OUR_MODULES)
clean-mod: $(OUR_CLEAN_MODULES) $(OUR_DISABLED_CLEAN_MODULES)
install-mod: $(OUR_INSTALL_MODULES)
uninstall-mod: $(OUR_UNINSTALL_MODULES) $(OUR_DISABLED_UNINSTALL_MODULES)

$(OUR_MODULES) $(OUR_CLEAN_MODULES) $(OUR_INSTALL_MODULES) $(OUR_UNINSTALL_MODULES) $(OUR_DISABLED_MODULES) $(OUR_DISABLED_CLEAN_MODULES) $(OUR_DISABLED_INSTALL_MODULES) $(OUR_DISABLED_UNINSTALL_MODULES):
	@set fnord $$MAKEFLAGS; amf=$$2; \
	target=`echo $@ | sed -e 's|^.*-||'`; \
	modname=`echo $@ | sed -e 's|-.*||' | sed -e 's|^.*/||'`; \
	confmoddir=`cat $(agc_builddir)/modules.conf | sed -e 's| ||' | grep $$modname$$ | sed -e 's|#||' | head -n 1`; \
	if test -z "$$confmoddir" ; then \
		moddir=$@ ; \
		buildmoddir=$(agc_builddir)/src/mod/$@ ;\
	else  \
		if test -d  "$(agc_srcdir)/src/mod/$$confmoddir" ; then \
			moddir="$(agc_srcdir)/src/mod/$$confmoddir" ; \
			buildmoddir="$(agc_builddir)/src/mod/$$confmoddir" ; \
		else \
			moddir="$$confmoddir" ; \
			buildmoddir="$(agc_builddir)/src/mod/$$confmoddir" ; \
		fi ; \
	fi ; \
	if test -z "$$target" ; then target="all" ; fi ; \
	if ! test -f $$moddir/$$modname.c && ! test -f $$moddir/$$modname.cpp && test $$modname != "mod_com_g729" ;	\
	then echo ; echo "WARNING $$modname is not a valid module dir, skipping it..." ; else \
		echo ;\
		echo making $$target $$modname ;\
		test -d "$$buildmoddir" || mkdir -p $$buildmoddir ; \
		(if test -f "$$moddir/Makefile" ; then \
			test -f "$$buildmoddir/Makefile" || cp $$moddir/Makefile $$buildmoddir/Makefile ; \
			cd $$buildmoddir && MODDIR=$$moddir MODNAME=$$modname BASE=$(agc_builddir) $(MAKE) $(AM_MAKEFLAGS) $$target; \
		else\
			cd $$buildmoddir && MODDIR=$$moddir MODNAME=$$modname BASE=$(agc_builddir) $(MAKE) $(AM_MAKEFLAGS) -f $(agc_builddir)/build/modmake.rules $$target ;\
		fi;) || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	fi; \
	test -z "$$fail" ;

.DEFAULT:
	@if test -z "`echo $@ | grep '\-all$$'`"; then  $(MAKE) $(AM_MAKEFLAGS) $@-all ; else echo Unknown target `echo $@ | sed -e 's|-all||'`; exit 1; fi
