SHELL = /bin/bash

default:;
	@echo "TARGETS: all build-configs update-inla update-R update-r-inla.org update-build-yourself www Rinla-local Rinla-install Rinla-testing"

all:;
	$(MAKE) build-configs update-inla update-R update-r-inla.org update-build-yourself

build-configs :;
	./make-builds

update-inla :;
	-tilix --title="laptop-g"  -e "make -C /home/hrue/p/inla/builds/my-laptop-g update"
	-tilix --title="laptop-O2" -e "make -C /home/hrue/p/inla/builds/my-laptop-O2 update"
	-tilix --title="mac64"     -e "ssh -x -t $$INLA_MAC   export INLA_TAG=$$INLA_TAG\; make -C builds/build64 update"
	-tilix --title="$$INLA_LINUX 64"        -e "ssh -p $$INLA_LINUX_PORT -x -t $$INLA_LINUX export INLA_TAG=$$INLA_TAG\; make -C builds/build64 update"
	-tilix --title="$$INLA_LINUX static"    -e "ssh -p $$INLA_LINUX_PORT -x -t $$INLA_LINUX export INLA_TAG=$$INLA_TAG\; make -C builds/build.static update"
	-tilix --title="$$INLA_LINUX windows64" -e "ssh -p $$INLA_LINUX_PORT -x -t $$INLA_LINUX export INLA_TAG=$$INLA_TAG\; make -C builds/windows64 update"
	-tilix --title="$$INLA_INLA 64" -e "ssh -x -t $$INLA_INLA export INLA_TAG=$$INLA_TAG\; make -C builds/build64 update"
	-tilix --title="$$INLA_INLA O2" -e "ssh -x -t $$INLA_INLA export INLA_TAG=$$INLA_TAG\; make -C builds/build.O2 update"
	-tilix --title="$$INLA_INLA g"  -e "ssh -x -t $$INLA_INLA export INLA_TAG=$$INLA_TAG\; make -C builds/build.debug update"
	-tilix --title="$$INLA_INLA static" -e "ssh -x -t $$INLA_INLA export INLA_TAG=$$INLA_TAG\; make -C builds/build.static update"
	@echo -en "\n\nType YES to proceeed: "
	@read ans && test $$ans == "YES" || exit 1
	@echo -e "Proceed with the build...\n\n"

update-R :;
	make -C R updateREPO

update-REPO:;
	make -C R updateREPO

update-r-inla.org :;
	-rsync -auvL --delete r-inla/r-inla.org/. www/r-inla.org/.

update-build-yourself :;
	-rsync -auv --exclude 'mc.tgz' --exclude 'my-laptop.tgz' build-yourself/.  www/build-yourself/.

www :;
	$(MAKE) update-r-inla.org
	-rsync -auv --delete www/. ~/www/www.inla/. 

Rinla-local :;
	make update-inla
	make update-R
	make -C R local

Rinla-local-Ronly :;
	make update-R
	make -C R local

Rinla-install:;
	make update-inla
	make update-R
	make -C R install
	make www
	-do-rsync-www
	-tilix -e 'ssh -x -t hrue@inla.math.ntnu.no "source .bash_profile; source .bashrc; sudo ls; cd; cd tmp; test -f INLA.tgz && rm INLA.tgz; wget http://www.math.ntnu.no/inla/binaries/INLA.tgz; sudo R CMD INSTALL --library=/usr/local/lib/R/site-library INLA.tgz; sudo ~hrue/bin/fix-permissions /usr/local/lib/R/site-library"'

Rinla-testing :;
	make update-inla
	make update-R
	make -C R testing
	make www
	-do-rsync-www
	-tilix -e 'ssh -x -t hrue@inla.math.ntnu.no "source .bash_profile; source .bashrc; sudo ls; cd; cd tmp; test -f INLA.tgz && rm INLA.tgz; wget http://www.math.ntnu.no/inla/binaries/testing/INLA.tgz; sudo R CMD INSTALL --library=/usr/local/lib/R/site-library INLA.tgz; sudo ~hrue/bin/fix-permissions /usr/local/lib/R/site-library"'

Rinla-testing-Ronly :;
	make update-REPO
	make -C R testing
	make www
	-do-rsync-www
	-tilix -e 'ssh -x -t hrue@inla.math.ntnu.no "source .bash_profile; source .bashrc; sudo ls; cd; cd tmp; test -f INLA.tgz && rm INLA.tgz; wget http://www.math.ntnu.no/inla/binaries/testing/INLA.tgz; sudo R CMD INSTALL --library=/usr/local/lib/R/site-library INLA.tgz; sudo ~hrue/bin/fix-permissions /usr/local/lib/R/site-library"'

.PHONY: build-configs update-inla update-R update-REPO update-r-inla.org update-build-yourself all default www Rinla-testing Rinla Rinla-install Rinla-local Rinla-local-Ronly


