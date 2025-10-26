# distutils: language = c

from . cimport _cecm
from libc.stdio cimport FILE, fopen, fclose
import os
import traceback

cdef object py_callback = None

cdef void c_progress_callback(unsigned int current, unsigned int total, int type) noexcept with gil:
    try:
        if py_callback is not None:
            py_callback(current, total, type)
    except Exception:
        traceback.print_exc()

_cecm.ecm_eccedc_init()
_cecm.unecm_eccedc_init()

def encode(input_path, output_path, progress=None):
    global py_callback
    cdef bytes py_in_path
    cdef bytes py_out_path
    cdef const char* c_in_path
    cdef const char* c_out_path
    cdef FILE* fin
    cdef FILE* fout
    cdef _cecm.progress_callback cb
    cdef int ret

    py_callback = progress

    py_in_path = os.fsencode(input_path)
    py_out_path = os.fsencode(output_path)
    c_in_path = py_in_path
    c_out_path = py_out_path

    fin = fopen(c_in_path, "rb")
    if fin == NULL:
        raise FileNotFoundError(f"Could not open input file: {input_path}")

    fout = fopen(c_out_path, "wb")
    if fout == NULL:
        fclose(fin)
        raise IOError(f"Could not open output file for writing: {output_path}")

    try:
        cb = &c_progress_callback if progress is not None else NULL

        print(f"Encoding {input_path} to {output_path}...")
        ret = _cecm.ecm_ecmify(fin, fout, cb)
        if ret != 0:
            print()
            raise RuntimeError("ECM encoding failed.")
        print()
        print("Encoding finished.")
    finally:
        fclose(fin)
        fclose(fout)
        py_callback = None

def decode(input_path, output_path, progress=None):
    global py_callback
    cdef bytes py_in_path, py_out_path
    cdef const char* c_in_path, *c_out_path
    cdef FILE* fin, *fout
    cdef _cecm.progress_callback cb
    cdef int ret

    py_callback = progress

    py_in_path = os.fsencode(input_path)
    py_out_path = os.fsencode(output_path)
    c_in_path = py_in_path
    c_out_path = py_out_path

    fin = fopen(c_in_path, "rb")
    if fin == NULL:
        raise FileNotFoundError(f"Could not open input file: {input_path}")

    fout = fopen(c_out_path, "wb")
    if fout == NULL:
        fclose(fin)
        raise IOError(f"Could not open output file for writing: {output_path}")

    try:
        cb = &c_progress_callback if progress is not None else NULL

        print(f"Decoding {input_path} to {output_path}...")
        ret = _cecm.unecm_unecmify(fin, fout, cb)
        if ret != 0:
            print()
            raise RuntimeError("ECM decoding failed. The file might be corrupt.")
        print()
        print("Decoding finished.")
    finally:
        fclose(fin)
        fclose(fout)
        py_callback = None