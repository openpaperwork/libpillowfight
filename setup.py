#!/usr/bin/env python

from setuptools import Extension, setup

setup(
    name="pypillowfight",
    # Don't forget to update src/pyocr/pyocr.py:VERSION as well
    # and download_url
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
        'pypillowfight.ace',
        'pypillowfight.blackfilter',
    ],
    package_dir={
        'pypillowfight': 'src/pypillowfight',
        'pypillowfight.ace': 'src/pypillowfight/ace',
        'pypillowfight.blackfilter': 'src/pypillowfight/blackfilter',
    },
    ext_modules=[
        Extension(
            'pypillowfight.ace._ace', [
                'src/pypillowfight/util.c',
                'src/pypillowfight/ace/_ace.c',
            ],
            include_dirs=[
                'src/pypillowfight'
            ],
            libraries=['m'],
            extra_compile_args=[],
            undef_macros=['NDEBUG'],
        ),
        Extension(
            'pypillowfight.blackfilter._blackfilter', [
                'src/pypillowfight/util.c',
                'src/pypillowfight/blackfilter/_blackfilter.c',
            ],
            include_dirs=[
                'src/pypillowfight'
            ],
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
)
