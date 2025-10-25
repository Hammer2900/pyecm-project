from setuptools import setup, Extension

ext_modules = [
    Extension(
        "pyecm",
        sources=[
            "src/pyecm/pyecm.pyx",
            "src/pyecm/c_src/ecm_wrapper.c",
            "src/pyecm/c_src/unecm_wrapper.c",
        ],
        include_dirs=["src/pyecm/c_src"],
        language="c",
    )
]

setup(
    ext_modules=ext_modules,
)