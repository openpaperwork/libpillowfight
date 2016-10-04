#!/usr/bin/env python

import os

from setuptools import Extension, setup

if os.name == "nt":
    libdep = []
else:
    libdep = ["m"]

setup(
    name="pypillowfight",
    version="0.2.1",
    description=("Library containing various image processing algorithms"),
    long_description=("Library containing various image processing algorithms:"
                      " Automatic Color Equalization, Unpaper's algorithms,"
                      " Stroke Width Transformation, etc"),
    keywords="image processing algorithm pillow pil",
    url="https://github.com/jflesch/libpillowfight#readme",
    download_url="https://github.com/jflesch/libpillowfight/archive/v0.1.0.zip",
    classifiers=[
        "Intended Audience :: Developers",
        "License :: OSI Approved :: GNU General Public License v3 or later"
        " (GPLv3+)",
        "Operating System :: POSIX :: Linux",
        "Programming Language :: Python",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.5",
        "Topic :: Multimedia :: Graphics :: Graphics Conversion",
    ],
    license="GPLv3+",
    author="Jerome Flesch",
    author_email="jflesch@gmail.com",
    packages=[
        'pillowfight',
    ],
    package_dir={
        'pillowfight': 'src/pillowfight',
    },
    ext_modules=[
        Extension(
            'pillowfight._clib', [
                'src/pillowfight/util.c',
                'src/pillowfight/_ace.c',
                'src/pillowfight/_blackfilter.c',
                'src/pillowfight/_blurfilter.c',
                'src/pillowfight/_border.c',
                'src/pillowfight/_canny.c',
                'src/pillowfight/_compare.c',
                'src/pillowfight/_gaussian.c',
                'src/pillowfight/_grayfilter.c',
                'src/pillowfight/_masks.c',
                'src/pillowfight/_noisefilter.c',
                'src/pillowfight/_pymod.c',
                'src/pillowfight/_sobel.c',
                'src/pillowfight/_swt.c',
            ],
            include_dirs=["include"],
            libraries=libdep,
            extra_compile_args=[],
            undef_macros=['NDEBUG'],
        ),
    ],
    data_files=[],
    scripts=[],
    install_requires=[
        "Pillow",
    ],
    setup_requires=['nose>=1.0'],
)
