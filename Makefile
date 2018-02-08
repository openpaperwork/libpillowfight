# If you want to build in MSYS2 on Windows
# export CMAKE_OPTS=-G "MSYS Makefiles"

VERSION_FILE = src/pillowfight/_version.h

build: build_c build_py

install: install_py install_c

uninstall: uninstall_py

build_py: ${VERSION_FILE}
	python3 ./setup.py build

build_c: build/libpillowfight.so

build/libpillowfight.so: ${VERSION_FILE} build/CMakeLists.txt
	(cd build && make -j4)

build/CMakeLists.txt:
	mkdir -p build
	(cd build && cmake ${CMAKE_OPTS} ..)

${VERSION_FILE}:
	echo -n "#define INTERNAL_PILLOWFIGHT_VERSION \"" >| $@
	echo -n $(shell git describe --always) >> $@
	echo "\"" >> $@

doc: install_py
	(cd doc && make html)
	doxygen doc/doxygen.conf
	cp doc/index.html doc/build/index.html

clean:
	rm -rf doc/build
	rm -rf build dist *.egg-info
	rm -f ${VERSION_FILE}

install_py:
	python3 ./setup.py install ${PIP_ARGS}

install_c:
	(cd build && make install)

uninstall_py:
	pip3 uninstall -y pypillowfight

help:
	@echo "make build || make build_c || make build_py"
	@echo "make doc"
	@echo "make help: display this message"
	@echo "make install || make install_c || make install_py"
	@echo "make uninstall || make uninstall_py"

.PHONY: help build install uninstall exe build_c build_py install_c install_py doc
