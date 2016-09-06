#!/usr/bin/env python

from setuptools import Extension, setup

setup(
    name="pillowfight",
    version="0.1.0",
    description=("Extra image processing algorithms on Pillow images"),
    keywords="image processing algorithm pillow pil",
    url="https://github.com/jflesch/libpillowfight",
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
                'src/pillowfight/_gaussian.c',
                'src/pillowfight/_grayfilter.c',
                'src/pillowfight/_masks.c',
                'src/pillowfight/_noisefilter.c',
                'src/pillowfight/_pymod.c',
                'src/pillowfight/_sobel.c',
                'src/pillowfight/_swt.c',
            ],
            include_dirs=["include"],
            libraries=['m'],
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
