/*
setenv DEBUG -g
setenv DEBUG
/bin/xlc -Wl,dll -qdll $DEBUG -qexportall -q32 -qnocse -qfloat=ieee -qgonum -o ffi.so ffi.c
/bin/xlc -qdll $DEBUG -qexportall -q32 -qnocse -qfloat=ieee -qgonum -o ffi_test ffi_test.c ffi.x
setenv FFI_COMMAND "/bin/xlc -Wl,dll -Wc,list -qdll $DEBUG -qexportall -q32 -qnocse -qfloat=ieee -qgonum"
setenv LIBPATH .:/usr/lib
./ffi_test
 */

#include <stdio.h>
#include "ffi.h"
     
int ffi_call_test()
{
  ffi_cif cif;
  ffi_type *args[1];
  void *values[1];
  char *s;
  int rc;
     
  /* Initialize the argument info vectors */
  args[0] = &ffi_type_pointer;
  values[0] = &s;
     
  /* Initialize the cif */
  if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_uint, args) == FFI_OK) {
    s = "Hello World!";
    ffi_call(&cif, FFI_FN(puts), &rc, values);
    /* rc now holds the result of the call to puts */
     
    /* values holds a pointer to the function's arg, so to
       call puts() again all we need to do is change the
       value of s */
    s = "This is cool!";
    ffi_call(&cif, FFI_FN(puts), &rc, values);
  }
     
  return 0;
}

/* Acts like puts with the file given at time of enclosure. */
void fputs_binding(ffi_cif *cif, void *ret, void* args[], void *stream)
{
  *(unsigned int *)ret = fputs(*(char **)args[0], (FILE *)stream);
}
     
int ffi_closure_test()
{
  ffi_cif cif;
  ffi_type *args[1];
  ffi_closure *closure;
     
  int (*bound_fputs)(char *);
  int rc;
     
  /* Allocate closure and bound_fputs */
  closure = ffi_closure_alloc(sizeof(ffi_closure), (void **)&bound_fputs);
     
  if (closure) {
    /* Initialize the argument info vectors */
    args[0] = &ffi_type_pointer;
     
    /* Initialize the cif */
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_uint, args) == FFI_OK) {
      /* Initialize the closure, setting stream to stdout */
      if (ffi_prep_closure_loc(closure, &cif, fputs_binding, (void *)stdout, (void *)bound_fputs) == FFI_OK) {
	rc = bound_fputs("Hello World!\n");
	/* rc now holds the result of the call to fputs */
      }
    }
  }
     
  /* Deallocate both closure, and bound_fputs */
  ffi_closure_free(closure);
     
  return 0;
}

int main(int argc, char **argv)
{
  ffi_call_test();
  ffi_closure_test();
}
