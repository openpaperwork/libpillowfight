#!/usr/bin/env python

from setuptools import Extension, setup

setup(
    name="pypillowfight",
    version="0.1.0",
    description=("Extra image processing algorithms on Pillow images"),
    keywords="image processing algorithm pillow pil",
    url="https://github.com/jflesch/pypillowfight",
    download_url="https://github.com/jflesch/pypillowfight/archive/v0.1.0.zip",
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
        'pypillowfight',
    ],
    package_dir={
        'pypillowfight': 'src/pypillowfight',
    },
    ext_modules=[
        Extension(
            'pypillowfight._pypillowfight', [
                'src/pypillowfight/util.c',
                'src/pypillowfight/_ace.c',
            ],
            include_dirs=[],
            libraries=['m'],
            extra_compile_args=[],
            undef_macros=['NDEBUG'],
        ),
        Extension(
            'pypillowfight._blackfilter', [
                'src/pypillowfight/util.c',
                'src/pypillowfight/_blackfilter.c',
            ],
            include_dirs=[],
            extra_compile_args=[],
            undef_macros=['NDEBUG'],
        ),
        Extension(
            'pypillowfight._grayfilter', [
                'src/pypillowfight/util.c',
                'src/pypillowfight/_grayfilter.c',
            ],
            include_dirs=[],
            extra_compile_args=[],
            undef_macros=['NDEBUG'],
        ),
        Extension(
            'pypillowfight._noisefilter', [
                'src/pypillowfight/util.c',
                'src/pypillowfight/_noisefilter.c',
            ],
            include_dirs=[],
            extra_compile_args=[],
            undef_macros=['NDEBUG'],
        ),
        Extension(
            'pypillowfight._blurfilter', [
                'src/pypillowfight/util.c',
                'src/pypillowfight/_blurfilter.c',
            ],
            include_dirs=[],
            extra_compile_args=[],
            undef_macros=['NDEBUG'],
        ),
        Extension(
            'pypillowfight._masks', [
                'src/pypillowfight/util.c',
                'src/pypillowfight/_masks.c',
            ],
            include_dirs=[],
            extra_compile_args=[],
            undef_macros=['NDEBUG'],
        ),
    ],
    data_files=[],
    scripts=[],
    install_requires=[
        "Pillow",
    ],
)
