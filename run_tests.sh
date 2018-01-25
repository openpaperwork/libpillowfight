#!/bin/sh

ret=0


for pyver in python3 ; do
    echo "# Testing with ${pyver}"
    rm -rf venv-${pyver}
    virtualenv -p ${pyver} --system-site-packages venv-${pyver}
    . venv-${pyver}/bin/activate
    if ! ${pyver} ./setup.py install ; then
        echo "Install failed"
        exit 1
    fi

    if ! ${pyver} ./setup.py nosetests ; then
        echo "Tests failed"
        ret=1
    fi
done

exit ${ret}
