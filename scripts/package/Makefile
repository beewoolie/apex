# Makefile
#  for packaging

REPOS := "file:///svn/tools/trunk/apex"

PKGBASE := apex-$(APEXRELEASE)

.PHONY: tgz.pkg
tgz.pkg tgz:
ifneq "$(wildcard $(PKGBASE))" ""
	-rm -rf $(PKGBASE)
endif
	svn export -q $(REPOS) $(PKGBASE)
	tar zcf $(PKGBASE).tar.gz $(PKGBASE)\
	 --exclude='cc-png.h'\
	 --exclude='pcm*-*.h'
	@rm -rf $(PKGBASE)

help:
	@echo '  tgz             - Build a tar.gz file of the source'
