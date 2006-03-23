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
	tar zc\
	 --exclude='cc-png.h'\
	 --exclude='pcm*-*.h'\
	 -f $(PKGBASE).tar.gz $(PKGBASE)
	@rm -rf $(PKGBASE)

help:
	@echo '  tgz             - Build a tar.gz file of the source'
