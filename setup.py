#!/usr/bin/env python

import os
import platform
import sys

from setuptools import Extension, setup

if os.name == "nt":
    libdep = []
    extra_compile_args = []
else:
    libdep = ["m"]
    if platform.architecture() == ('32bit', 'ELF'):
        extra_compile_args = ['-msse2', '-mfpmath=sse']
    else:
        extra_compile_args = []

try:
    with open("src/pillowfight/_version.h", "r") as file_descriptor:
        version = file_descriptor.read().strip()
        version = version.split(" ")[2][1:-1]
        if "-" in version:
            version = version.split("-")[0]
except FileNotFoundError:
    print("WARNING: version.txt file is missing")
    print("WARNING: Please run 'make version' first")
    sys.exit(1)


setup(
    name="pypillowfight",
    version=version,
    description=("Library containing various image processing algorithms"),
    long_description=("Library containing various image processing algorithms:"
                      " Automatic Color Equalization, Unpaper's algorithms,"
                      " Stroke Width Transformation, etc"),
    keywords="image processing algorithm pillow pil",
    url="https://github.com/openpaperwork/libpillowfight#readme",
    download_url=(
        "https://github.com/openpaperwork/libpillowfight/archive/v0.2.2.zip"
    ),
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
    license="GPLv2",
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
                'src/pillowfight/_version.c',
            ],
            include_dirs=["include"],
            libraries=libdep,
            extra_compile_args=extra_compile_args,
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
