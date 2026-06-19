REPO_ROOT := $(abspath $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/..)

srpm:
	dnf -y install rpmdevtools
	VERSION=$$(grep '^Version:' $(REPO_ROOT)/ds5-audio-haptics-bt.spec | awk '{print $$2}') && \
	sed "s/^Version:.*/Version:        $$VERSION/" $(REPO_ROOT)/ds5-audio-haptics-bt.spec \
	    > /tmp/ds5-audio-haptics-bt.spec && \
	rpmbuild -D "_srcrpmdir $(outdir)" \
	         -D "_sourcedir /tmp" \
	         --define "debug_package %{nil}" \
	         -bs /tmp/ds5-audio-haptics-bt.spec
