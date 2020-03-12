#!/usr/bin/env python3

import setuptools
import numpy as np
from distutils.core import Extension

# ---------------------------------------------------------------------------- #

shadowmap = Extension('c_shadowmap',
                      include_dirs=[np.get_include()],
                      sources = ['shadows/shadowmap.c'])

# ---------------------------------------------------------------------------- #

with open('README.md', 'r') as fh:
    long_description = fh.read()

setuptools.setup(name='shadow-mapper-andrea.pinna',
                 version='0.0.1',
                 author='Andrea Pinna',
                 author_email='andreapinna@gmail.com',
                 description = 'Compute the shading from an elevation raster',
                 long_description=long_description,
                 long_description_content_type="text/markdown",
                 url="https://github.com/pinno/shadow-mapper",
                 packages=setuptools.find_packages(),
                 classifiers=["Programming Language :: Python :: 3",
                              "License :: OSI Approved :: MIT License",
                              "Operating System :: Linux"],
                 python_requires='>=3.7',
                 include_package_data=True,
                 ext_modules = [shadowmap])

# ---------------------------------------------------------------------------- #
