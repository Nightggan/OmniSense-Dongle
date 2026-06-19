REPO_ROOT := $(abspath $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/..)

# COPR passes: outdir=<result dir>  spec=<path to spec file>
SPEC ?= $(REPO_ROOT)/ds5-audio-haptics-bt.spec

srpm:
	rpmbuild -D "_srcrpmdir $(outdir)" \
	         -D "_sourcedir $(REPO_ROOT)" \
	         --define "debug_package %{nil}" \
	         -bs $(SPEC)
