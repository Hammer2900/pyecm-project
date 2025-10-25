# distutils: language = c

from libc.stdio cimport FILE

cdef extern from "c_src/ecm_tools.h":
    ctypedef void (*progress_callback)(unsigned int current, unsigned int total, int type)

    void ecm_eccedc_init()
    int ecm_ecmify(FILE *infile, FILE *outfile, progress_callback callback)

    void unecm_eccedc_init()
    int unecm_unecmify(FILE *infile, FILE *outfile, progress_callback callback)