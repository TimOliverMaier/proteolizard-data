from setuptools import setup

with open("../README.md", "r") as fh:
    ld = fh.read()

setup(
    name='pyproteolizard',
    version='0.1.0',
    description='python interface to scalable algorithms based on bruker timsTOF data',
    packages=['pyproteolizard'],
    package_data={'proteolizard': ['proteolizard.so']},
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: GNU General Public License v2 or later (GPLv3+)",
        "Operating System :: Linux"
    ],
    long_description=ld,
    long_description_content_type="text/markdown",
    install_requires=[
        "tensorflow >=2.7",
        "pandas >=1.1",
        "opentims_bruker_bridge",
	"sklearn",
	"tqdm",
	"pytest"
    ]
)
