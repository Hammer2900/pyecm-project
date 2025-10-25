from setuptools import setup, Extension, find_packages

extensions = [
    Extension(
        "pyecm.pyecm",
        sources=[
            "src/pyecm/pyecm.pyx",
            "src/pyecm/c_src/ecm_wrapper.c",
            "src/pyecm/c_src/unecm_wrapper.c",
        ],
        include_dirs=[
            "src/pyecm",
            "src/pyecm/c_src"
        ],
        language="c",
    )
]

setup(
    packages=find_packages(where="src"),
    package_dir={"": "src"},
    package_data={
        "pyecm": ["*.pxd", "c_src/*.h", "c_src/*.c"],
    },
    ext_modules=extensions,
    setup_requires=['cython'],
    zip_safe=False,
)