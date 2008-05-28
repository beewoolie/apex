# Makefile
#  for packaging.  It assumes that the current directory is a git
#  repository from which the source may be cloned.

PKGBASE := apex-$(APEXRELEASE)

.PHONY: tgz.pkg
tgz.pkg tgz:
ifneq "$(wildcard $(PKGBASE))" ""
	-rm -rf $(PKGBASE)
endif
	@git-clone -l ./ $(PKGBASE)
	find $(PKGBASE) -depth -type d -name '_intel' -exec rm -rf \{\} \;
	rm -rf $(PKGBASE)/include/splash
	tar zc\
	 --exclude='.git'\
	 --exclude='cc-png.h'\
	 --exclude='pcm*-*.h'\
	 --exclude='colorbars*.png'\
	 -f $(PKGBASE).tar.gz $(PKGBASE)
	@rm -rf $(PKGBASE)

help:
	@echo '  tgz             - Build a tar.gz file of the source'
