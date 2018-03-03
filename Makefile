# If you want to build in MSYS2 on Windows
# export CMAKE_OPTS=-G "MSYS Makefiles"

VERSION_FILE = src/pillowfight/_version.h
PYTHON = python3

build: build_c build_py

install: install_py install_c

uninstall: uninstall_py

build_py: ${VERSION_FILE}
	${PYTHON} ./setup.py build

build_c: ${VERSION_FILE} build/libpillowfight.so

build/libpillowfight.so: ${VERSION_FILE} build/Makefile
	(cd build && make -j4)

build/Makefile:
	mkdir -p build
	(cd build && cmake ${CMAKE_OPTS} ..)

${VERSION_FILE}:
	echo -n "#define INTERNAL_PILLOWFIGHT_VERSION \"" >| $@
	echo -n $(shell git describe --always) >> $@
	echo "\"" >> $@

version: ${VERSION_FILE}

doc: install_py
	(cd doc && make html)
	doxygen doc/doxygen.conf
	cp doc/index.html doc/build/index.html

check:
	flake8
# 	pydocstyle src

test: build_py
	tox

linux_exe:

windows_exe:

release:
ifeq (${RELEASE}, )
	@echo "You must specify a release version (make release RELEASE=1.2.3)"
else
	@echo "Will release: ${RELEASE}"
	@echo "Checking release is in ChangeLog ..."
	grep ${RELEASE} ChangeLog | grep -v "/xx"
	@echo "Releasing ..."
	git tag -a ${RELEASE} -m ${RELEASE}
	git push origin ${RELEASE}
	make clean
	make version
	${PYTHON} ./setup.py sdist upload
	@echo "All done"
endif

clean:
	rm -rf doc/build
	rm -rf build dist *.egg-info
	rm -f ${VERSION_FILE}

install_py: ${VERSION_FILE}
	${PYTHON} ./setup.py install ${PIP_ARGS}

install_c: build/Makefile ${VERSION_FILE}
	(cd build && make install)

uninstall_py:
	pip3 uninstall -y pypillowfight

uninstall_c:
	echo "Can't uninstall C library. Sorry"

help:
	@echo "make build || make build_c || make build_py"
	@echo "make check"
	@echo "make doc"
	@echo "make help: display this message"
	@echo "make install || make install_c || make install_py"
	@echo "make release"
	@echo "make test"
	@echo "make uninstall || make uninstall_py"

.PHONY: \
	build \
	build_c \
	build_py \
	check \
	doc \
	exe \
	exe \
	help \
	install \
	install_c \
	install_py \
	release \
	test \
	uninstall \
	uninstall_c \
	version
