#include <stdio.h>
#include "ffi.h"
int ffi_obtain_call_fn_and_closure_fn(ffi_cif *cif);
int ffi_build_cif_for_short_name(char *name, ffi_cif *cif);

int main(int argc, char **argv)
{
  if (argc != 2) {
    printf("usage: ffi_util [ --version | --help | shortname ]\n");
    return 0;
  }
  if (0 == strcmp(argv[1], "--version")) {
    printf(LIBFFI_VERSION " z/OS anaconda build %d\n", LIBFFI_BUILD_NUMBER);
    return 0;
  }
  if (0 == strcmp(argv[1], "--help")) {
    printf("ffi_util will build a ffi shared object, unless it is already present.\n"
	   "It uses the same environment variables that libffi uses:\n"
	   "FFI_LIB directory\n"
	   "    the pathname of a directory containing many ffi shared objects\n"
	   "    this variable is set by \"conda bin/activate\", which runs the scripts in $CONDA_PREFIX/etc/conda/activate.d\n"
	   "    this directory is $CONDA_PREFIX/lib/ffi\n"
	   "FFI_LOCAL_LIB directory\n"
	   "    the pathname of a directory to put new ffi shared objects\n"
	   "    If this variable is not specified, $HOME/.ffi is used\n"
	   "HOME directory\n"
	   "    new ffi shared objects are written into $FFI_LOCAL_LOB (if defined) otherwise $HOME/.ffi\n"
	   "\n"
	   "environment variables useful for debugging:\n"
	   "FFI_TRACE filename\n"
	   "    sends all ffi messages produced while building a ffi shared object to filename\n"
	   "    If this environment variable is not present, stderr is used\n"
	   "FFI_SHOW_PROGRESS YES or yes\n"
	   "    causes ffi to show a few additional messages\n"
	   "\n"
	   "environment variables useful for low level debugging:\n"
	   "FFI_LIST [ YES | yes ]\n"
	   "    causes the compiler to be given an argument to create a listing\n"
	   "FFI_DEBUG [ YES | yes ]\n"
	   "    causes /bin/amblist to be called\n"
	   "FFI_COMMAND\n"
	   "    default: /bin/xlc -Wl,dll -qdll -qexportall -q32 -qnocse -qfloat=ieee\n");	   
    return 0;
  }
  char *shortname = argv[1];
  ffi_cif cif_s, *cif = &cif_s;
  int result;
  if (0 != (result = ffi_build_cif_for_short_name(shortname, cif))) {
    fprintf(stderr, "ffi_build_cif_for_short_name failed with result %d\n", result);
    return result;
  }
  if (0 != (result = ffi_obtain_call_fn_and_closure_fn(cif))) {
    fprintf(stderr, "ffi_obtain_call_fn_and_closure_fn failed with result %d\n", result);
    return result;
  }
  return 0;    
}


