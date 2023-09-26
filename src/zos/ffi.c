/*
setenv DEBUG -g
setenv DEBUG
/bin/xlc -qdll $DEBUG -qexportall -q32 -qnocse -qfloat=ieee -qgonum -c -o libffi.so ffi.c
ar -r libffi.a ffi.o
/bin/xlc -Wl,dll -qdll $DEBUG -qexportall -q32 -qnocse -qfloat=ieee -qgonum -o libffi.so libffi.o
/bin/xlc -qdll $DEBUG -qexportall -q32 -qnocse -qfloat=ieee -qgonum -o ffi_test ffi_test.c libffi.x
setenv FFI_COMMAND "/bin/xlc -Wl,dll -Wc,list -qdll $DEBUG -qexportall -q32 -qnocse -qfloat=ieee -qgonum"
setenv LIBPATH .:/usr/lib
./ffi_test
 */

#define _POSIX_SOURCE
#define _OPEN_THREADS
#define _UNIX03_SOURCE
#define _XOPEN_SOURCE_EXTENDED 1
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <string.h>

#include "ffi.h"
#include "ffi_opts.h"

#define FFI_TYPEDEF(name, type, id, maybe_const)\
struct struct_align_##name {			\
  char c;					\
  type x;					\
};						\
maybe_const ffi_type ffi_type_##name = {	\
  sizeof(type),					\
  offsetof(struct struct_align_##name, x),	\
  id, NULL					\
}

#define FFI_COMPLEX_TYPEDEF(name, type, maybe_const)	\
static ffi_type *ffi_elements_complex_##name [2] = {	\
	(ffi_type *)(&ffi_type_##name), NULL		\
};							\
struct struct_align_complex_##name {			\
  char c;						\
  _Complex type x;					\
};							\
maybe_const ffi_type ffi_type_complex_##name = {	\
  sizeof(_Complex type),				\
  offsetof(struct struct_align_complex_##name, x),	\
  FFI_TYPE_COMPLEX,					\
  (ffi_type **)ffi_elements_complex_##name		\
}

/* Size and alignment are fake here. They must not be 0. */
ffi_type ffi_type_void = {
  1, 1, FFI_TYPE_VOID, NULL
};

typedef unsigned char  UINT8;
typedef signed char    SINT8;
typedef unsigned short UINT16;
typedef signed short   SINT16;
typedef unsigned int   UINT32;
typedef signed int     SINT32;
typedef unsigned long long UINT64;
typedef signed long long   SINT64;

#define CONST

FFI_TYPEDEF(uint8, UINT8, FFI_TYPE_UINT8, CONST);
FFI_TYPEDEF(sint8, SINT8, FFI_TYPE_SINT8, CONST);
FFI_TYPEDEF(uint16, UINT16, FFI_TYPE_UINT16, CONST);
FFI_TYPEDEF(sint16, SINT16, FFI_TYPE_SINT16, CONST);
FFI_TYPEDEF(uint32, UINT32, FFI_TYPE_UINT32, CONST);
FFI_TYPEDEF(sint32, SINT32, FFI_TYPE_SINT32, CONST);
FFI_TYPEDEF(uint64, UINT64, FFI_TYPE_UINT64, CONST);
FFI_TYPEDEF(sint64, SINT64, FFI_TYPE_SINT64, CONST);

FFI_TYPEDEF(pointer, void*, FFI_TYPE_POINTER, CONST);

FFI_TYPEDEF(int, int, FFI_TYPE_INT, CONST);
FFI_TYPEDEF(float, float, FFI_TYPE_FLOAT, CONST);
FFI_TYPEDEF(double, double, FFI_TYPE_DOUBLE, CONST);
FFI_TYPEDEF(longdouble, long double, FFI_TYPE_LONGDOUBLE, CONST);

FFI_COMPLEX_TYPEDEF(float, float, CONST);
FFI_COMPLEX_TYPEDEF(double, double, CONST);
FFI_COMPLEX_TYPEDEF(longdouble, long double, CONST);



const char *ffi_type_name_array[] = {"void", "int", "float", "double", "long double",
				     "unsigned char", "signed char",
				     "unsigned short int", "short int",
				     "unsigned int", "int",
				     "unsigned long long", "long long int",
				     "struct", "void *", "complex"};

const char ffi_type_letter_array[] = {'v', 'i', 'f', 'd', 'l',
				      'C', 'c',
				      'H', 'h',
				      'I', 'i',
				      'X', 'x',
				      '.', 'P', '.'};
const char ffi_type_letter_for_structure[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
					      'a', 'b', 'e', 'g'};

static int ffi_show_progress = 0;
static FILE *ffi_output = NULL;

static int ffi_printf(char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  if (ffi_output == NULL) {
    char *ffi_trace = getenv("FFI_TRACE");
    ffi_output = fopen(ffi_trace, "w");
  }
  if (ffi_output == NULL)
    ffi_output = stderr;
  int result = vfprintf(ffi_output, fmt, args);
  fflush(ffi_output);
  va_end(args);
  return result;
}
              
typedef struct _struct_info {
  int phase;
  int index;
  ffi_type *str;
  struct _struct_info *next;
} struct_info;

static struct_info *new_struct_info(int index, ffi_type *str, struct_info **sip)
{
  if (str == NULL) return NULL;
  struct_info *new_si = (struct_info *)calloc(1, sizeof(struct_info));
  new_si->str = str;
  new_si->index = index;
  *sip = new_si;
  return new_si;
}

static struct_info *info_for_struct_type(ffi_type *str, struct_info **sip)
{
  if (str == NULL) return NULL;
  int index=0;
  struct_info *last_si = NULL;
  for (struct_info *si = *sip; si; last_si = si, si = si->next, index++) {
    if (si->str == str)
      return si;
  }
  return new_struct_info(index, str, last_si ? &last_si->next : sip);
}

static void collect_struct_types(ffi_type *type, struct_info **sip)
{
  if (type->type != FFI_TYPE_STRUCT) return;
  struct_info *si = info_for_struct_type(type, sip);
  if (si->phase == 0) {
    si->phase = 1;
    ffi_type *etype;
    for (int e = 0; NULL != (etype = type->elements[e]); e++) {
      collect_struct_types(etype, sip);
    }
  }
}
                                    
static void write_type_name(FILE *out, ffi_type *type, struct_info **sip, int ffi_rtype_var)
{
  if (ffi_rtype_var && 
      (type->type == FFI_TYPE_INT ||
       type->type == FFI_TYPE_SINT8 ||
       type->type == FFI_TYPE_SINT16 ||
       type->type == FFI_TYPE_SINT32 ||
       type->type == FFI_TYPE_SINT64)) {
    fprintf(out, "long long"); /* aka ffi_sarg */
  } else if (ffi_rtype_var && 
	     (type->type == FFI_TYPE_UINT8 ||
	      type->type == FFI_TYPE_UINT16 ||
	      type->type == FFI_TYPE_UINT32 ||
	      type->type == FFI_TYPE_UINT64)) {
    fprintf(out, "unsigned long long"); /* aka ffi_arg */
  } else if (type->type == FFI_TYPE_COMPLEX) {
    fprintf(out, "%s",
	    (type == &ffi_type_complex_float) ? "complex float" :
	    (type == &ffi_type_complex_double) ? "complex double" :
	    (type == &ffi_type_complex_longdouble) ? "complex long double" :
	    "complex");
  } else if (type->type == FFI_TYPE_STRUCT) {
    struct_info *si = info_for_struct_type(type, sip);
    fprintf(out, "struct _struct_%d", si->index);
  } else {
    fprintf(out, "%s", ffi_type_name_array[type->type]);
  }
}

static void write_type_definition(FILE *out, ffi_type *type, struct_info **sip)
{
  if (type->type == FFI_TYPE_STRUCT) {
    struct_info *si = info_for_struct_type(type, sip);
    if (si->phase == 1) {
      si->phase = 2;
      ffi_type *etype;
      for (int e = 0; NULL != (etype = type->elements[e]); e++) {
	write_type_definition(out, etype, sip);
      }
      fprintf(out, "struct _struct_%d {\n", si->index);
      for (int e = 0; NULL != (etype = type->elements[e]); e++) {
	fprintf(out, "  ");
	write_type_name(out, etype, sip, 0);
	fprintf(out, " field_%d;\n", e);
      }
      fprintf(out, "};\n");
    }
  }
}

static char type_letter_for_type(ffi_type *type, struct_info **sip)
{
  if (type->type == FFI_TYPE_COMPLEX) {
    return
      (char)((type == &ffi_type_complex_float) ? 'F' :
	     (type == &ffi_type_complex_double) ? 'D' :
	     (type == &ffi_type_complex_longdouble) ? 'L' :
	     1);
  } else if (type->type == FFI_TYPE_STRUCT) {
    struct_info *si = info_for_struct_type(type, sip);
    if (si->index >= sizeof(ffi_type_letter_for_structure))
      return (char)2;
    return ffi_type_letter_for_structure[si->index];
  } else {
    if (type->type < 0 || type->type >= sizeof(ffi_type_letter_array))
      return (char)3;
    return ffi_type_letter_array[type->type];
  }
}

static ffi_type *structure_type_from_index(int index, struct_info **sip)
{
  for (struct_info *si = *sip; si; si = si->next, index--) {
    if (index == 0)
      return si->str;
  }
  return NULL;
}

static ffi_type *type_for_type_letter(char type_letter, struct_info **sip)
{
  switch (type_letter) {
  case 'v': return &ffi_type_void;
  /* case 'i': return &ffi_type_int; */
  case 'f': return &ffi_type_float;
  case 'd': return &ffi_type_double;
  case 'l': return &ffi_type_longdouble;
  case 'C': return &ffi_type_uint8;
  case 'c': return &ffi_type_sint8;
  case 'H': return &ffi_type_uint16;
  case 'h': return &ffi_type_sint16;
  case 'I': return &ffi_type_uint32;
  case 'i': return &ffi_type_sint32;
  case 'X': return &ffi_type_uint64;
  case 'x': return &ffi_type_sint64;
  case 'P': return &ffi_type_pointer;
  case 'F': return &ffi_type_complex_float;
  case 'D': return &ffi_type_complex_double;
  case 'L': return &ffi_type_complex_longdouble;
  case '0': return structure_type_from_index(0, sip);
  case '1': return structure_type_from_index(1, sip);
  case '2': return structure_type_from_index(2, sip);
  case '3': return structure_type_from_index(3, sip);
  case '4': return structure_type_from_index(4, sip);
  case '5': return structure_type_from_index(5, sip);
  case '6': return structure_type_from_index(6, sip);
  case '7': return structure_type_from_index(7, sip);
  case '8': return structure_type_from_index(8, sip);
  case '9': return structure_type_from_index(9, sip);
  case 'a': return structure_type_from_index(10, sip);
  case 'b': return structure_type_from_index(11, sip);
  case 'e': return structure_type_from_index(12, sip);
  case 'g': return structure_type_from_index(13, sip);
  default: return NULL;
  }
}

static int add_short_name_for_type(char **name_ptr, int *name_size_ptr, ffi_type *type, struct_info **sip, char *msg)
{
  if (*name_size_ptr < 1) {
    ffi_printf("ffi types are too large at %s\n", msg);
    return 1;
  }
  char letter = type_letter_for_type(type, sip);
  if (letter==1) {
    ffi_printf("Bad ffi complex type at %s\n", msg);
    return 1;
  }
  if (letter==2) {
    ffi_printf("Too many ffi structure types at %s\n", msg);
    return 1;
  }
  if (letter==3) {
    ffi_printf("Bad ffi type at %s\n", msg);
    return 1;
  }
  *((*name_ptr)++) = letter;
  (*name_size_ptr)--;
  return 0;
}

static int add_letter_to_short_name(char **name_ptr, int *name_size_ptr, char letter, char *msg)
{
  if (*name_size_ptr < 1) {
    ffi_printf("ffi types are too large at %s\n", msg);
    return 1;
  }
  *((*name_ptr)++) = letter;
  (*name_size_ptr)--;
  return 0;
}

static int short_name_for_fn(char *name, int name_size, ffi_cif *cif, struct_info **sip)
{
  char msg[32];
  snprintf(msg, sizeof(msg), "rtype");
  if (add_short_name_for_type(&name, &name_size, cif->rtype, sip, msg)) return 1;
  if (add_letter_to_short_name(&name, &name_size, '_', msg)) return 1;
  for (int a = 0; a < cif->nfixedargs; a++) {
    snprintf(msg, sizeof(msg), "arg%d", a);
    ffi_type *atype = cif->arg_types[a];
    if (add_short_name_for_type(&name, &name_size, atype, sip, msg)) return 1;
  }
  if (cif->is_varargs) {
    if (add_letter_to_short_name(&name, &name_size, 'V', msg)) return 1;
    for (int a = cif->nfixedargs; a < cif->nargs; a++) {
      snprintf(msg, sizeof(msg), "arg%d", a);
      ffi_type *atype = cif->arg_types[a];
      if (add_short_name_for_type(&name, &name_size, atype, sip, msg)) return 1;
    }
  }
  for (struct_info *si = *sip; si; si = si->next) {
    snprintf(msg, sizeof(msg), "struct%d", si->index);
    if (add_letter_to_short_name(&name, &name_size, '_', msg)) return 1;
    ffi_type *etype;
    for (int e = 0; NULL != (etype = si->str->elements[e]); e++) {
      snprintf(msg, sizeof(msg), "struct%d element%d", si->index, e);
      if (add_short_name_for_type(&name, &name_size, etype, sip, msg)) return 1;
    }
  }
  if (add_letter_to_short_name(&name, &name_size, 0, "add final null char")) return 1;
  return 0;
}

int ffi_build_cif_for_short_name(char *name, ffi_cif *cif)
{
  struct_info *first_si = NULL;
  struct_info **sip = &first_si;
  memset(cif, 0, sizeof(*cif));
  cif->abi = FFI_DEFAULT_ABI;
  char *u1 = strchr(name, '_');
  if (u1 == NULL) {
    ffi_printf("shortname must contain at least one '_'\n");
    return 1;
  }
  char *args = u1 + 1;
  char *var = strchr(args, 'V');
  char *start = args;
  char *end = strchr(args, '_');
  if (end == NULL) end = strchr(args, (char)0);
  cif->nargs = (int)(end - start);
  if (var) {
    cif->nargs -= 1;
    cif->nfixedargs = (int)(var - start);
    cif->is_varargs = 1;
  } else {
    cif->nfixedargs = cif->nargs;
    cif->is_varargs = 0;
  }
  if (cif->nargs > 0 && NULL == (cif->arg_types = (ffi_type **)malloc(sizeof(ffi_type *) * cif->nargs))) {
    ffi_printf("Out of memory\n");
    return 2;
  }
  
  char *un = args;
  int struct_count = 0;
  while (1) {
    un = strchr(un+1, '_');
    if (un == NULL) break;
    ffi_type *str = (ffi_type *)malloc(sizeof(ffi_type));
    if (str == NULL) {
      ffi_printf("Out of memory\n");
      return 2;
    }
    str->type = FFI_TYPE_STRUCT;
    if (0) printf("new struct type %llX type=%d\n", str, str->type);
    if (NULL == info_for_struct_type(str, sip)) {
      ffi_printf("Out of memory\n");
      return 2;
    }
    struct_count++;
  }
  if (NULL == (cif->rtype = type_for_type_letter(name[0], sip))) {
    ffi_printf("Invalid shortname type letter '%c'\n", name[0]);
    return 3;
  }
  if (0) printf("rtype %llX %d\n", cif->rtype, cif->rtype->type);
  int voffset = 0;
  for (int i=0; i< cif->nargs; i++) {
    if (args[i] == 'V') voffset = 1; 
    if (NULL == (cif->arg_types[i] = type_for_type_letter(args[i+voffset], sip))) {
      ffi_printf("Invalid shortname type letter '%c'\n", args[i+voffset]);
      return 3;
    }
    if (0) printf("arg type[%d] %llX %d\n", i, cif->arg_types[i], cif->arg_types[i]->type);
  }
  un = args;
  struct_count = 0;
  struct_info *info = *sip;
  while (1) {
    un = strchr(un+1, '_');
    if (un == NULL) break;
    start = un + 1;
    end = strchr(start, '_');
    if (end == NULL) end = strchr(start, (char)0);
    int n = (int)(end - start);
    if (0) printf("str %d type %llX %d, element_count %d\n", struct_count, info->str, info->str->type, n);
    if (NULL == (info->str->elements = (ffi_type **)malloc(sizeof(ffi_type *) * (n + 1)))) {
      ffi_printf("Out of memory\n");
      return 2;
    }
    for (int i=0; i<n; i++) {
      if (NULL == (info->str->elements[i] = type_for_type_letter(start[i], sip))) {
	ffi_printf("Invalid shortname type letter '%c'\n", start[i]);
	return 3;
      }
      if (0) printf("element type[%d] %llX %d\n", i, info->str->elements[i], info->str->elements[i]->type);
    }
    info->str->elements[n] = NULL;
    struct_count++;
    info = info->next;
  }
  return 0;
}

static void write_ffi_call_fn_and_closure_fn(FILE *out, ffi_cif *cif, struct_info **sip)
{
  if (cif->is_varargs) {
    fprintf(out, "#include <stdarg.h>\n\n");
  }
  for (struct_info *si = *sip; si; si = si->next) {
    fprintf(out, "struct _struct_%d;\n", si->index);
  }
  for (struct_info *si = *sip; si; si = si->next) {
    write_type_definition(out, si->str, sip);
  }
  fprintf(out, "\ntypedef ");
  write_type_name(out, cif->rtype, sip, 0);
  fprintf(out, "(fn_type)(");
  if (cif->nargs == 0) {
    fprintf(out, "void");
  } else {
    for (int a = 0; a < cif->nfixedargs; a++) {
      if (a > 0) fprintf(out, ", ");
      ffi_type *atype = cif->arg_types[a];
      write_type_name(out, atype, sip, 0);
    }
    if (cif->is_varargs) {
      if (cif->nargs > 0) fprintf(out, ", ");
      fprintf(out, "...");
    }
  }
  fprintf(out, ");\n\n");
  fprintf(out, "void _ffi_call (void *cif, void *fn, void *rvalue, void **avalues) {\n");
  fprintf(out, "  ");
  if (cif->rtype->type != FFI_TYPE_VOID) {
    fprintf(out, "  *((");
    write_type_name(out, cif->rtype, sip, 1);
    fprintf(out, " *)rvalue) = ");
  }
  fprintf(out, "((fn_type *)fn)(");
  for (int a = 0; a < cif->nargs; a++) {
    if (a > 0) fprintf(out, ", ");
    ffi_type *atype = cif->arg_types[a];
    fprintf(out, "*(");
    write_type_name(out, atype, sip, 0);
    fprintf(out, " *)avalues[%d]", a);
  }
  fprintf(out, ");\n");
  fprintf(out, "}\n\n");

  fprintf(out, "static struct _closure {\n");
  fprintf(out, "  void *cif;\n");
  fprintf(out, "  void (*fun) (void *cif, void *ret, void **args, void *user_data);\n");
  fprintf(out, "  void *user_data;\n");
  fprintf(out, "} _closure;\n\n");
  write_type_name(out, cif->rtype, sip, 0);
  fprintf(out, " _ffi_closure (");
  if (cif->nargs == 0) {
    fprintf(out, "void");
  } else {
    for (int a = 0; a < cif->nfixedargs; a++) {
      if (a > 0) fprintf(out, ", ");
      ffi_type *atype = cif->arg_types[a];
      write_type_name(out, atype, sip, 0);
      fprintf(out, " arg%d", a);
    }
    if (cif->is_varargs) {
      if (cif->nargs > 0) fprintf(out, ", ");
      fprintf(out, "...");
    }
  }
  fprintf(out, ") {\n");
  if (cif->nargs == 0) {
    fprintf(out, "  void **args = (void **)0;\n");
  } else {
    fprintf(out, "  void *args[] = {");
    for (int a = 0; a < cif->nargs; a++) {
      if (a > 0) fprintf(out, ",");
      if (a < cif->nfixedargs) {
	fprintf(out, "(void *)&arg%d", a);
      } else {
	fprintf(out, "(void *)0", a);
      }
    }
    fprintf(out, "};\n");
    if (cif->nargs > cif->nfixedargs) {
      fprintf(out, "  va_list ap;\n");
      fprintf(out, "  int a;\n");
      fprintf(out, "  va_start(ap, arg%d);\n", cif->nfixedargs-1);
      for (int a = cif->nfixedargs; a < cif->nargs; a++) {
	fprintf(out, "  ");
	ffi_type *atype = cif->arg_types[a];
	write_type_name(out, atype, sip, 0);
	fprintf(out, " arg%d = va_arg(ap, ", a);
	write_type_name(out, atype, sip, 0);
	fprintf(out, ");\n");
	fprintf(out, "  args[%d] = (void *)&arg%d;\n", a, a);
      }
      fprintf(out, "  va_end(ap);\n");
    }
  }
  if (cif->rtype->type != FFI_TYPE_VOID) {
    fprintf(out, "  ");
    write_type_name(out, cif->rtype, sip, 1);
    fprintf(out, " result;\n");
  }
  fprintf(out, "  _closure.fun (_closure.cif, (void *)%s, args, _closure.user_data);\n",
	  (cif->rtype->type != FFI_TYPE_VOID) ? "&result" : "0");
  if (cif->rtype->type != FFI_TYPE_VOID) {
    fprintf(out, "  return result;\n");
  }
  fprintf(out, "}\n\n");
}

static void show_data(void *ptr, int n)
{
  void **ptra = (void **)ptr;
  ffi_printf("%016llX: ", ptr);
  for (int i=0; i<n; i++) {
    ffi_printf("%016llX ", ptra[i]);
  }
  ffi_printf("\n");
}

typedef struct _registered_shared_object {
  char *short_name;
  void *dll_handle;
  void *call_fn;
  void *closure_fn;
  struct _registered_shared_object *next;
} RSO;

static RSO *rso_list = NULL;

static int find_registered_shared_object(ffi_cif *cif, char *short_name)
{
  int found = 0;
  for (RSO *rso=rso_list; rso; rso=rso->next) {
    if (0==strcmp(rso->short_name, short_name)) {
      if (cif) {
	cif->call_fn = rso->call_fn;
	cif->closure_fn = rso->closure_fn;
      }
      found = 1;
      break;
    }
  }
  return found;
}

static void register_shared_object(ffi_cif *cif, void *dll_handle, char *short_name)
{
  RSO *new_rso = (RSO *)calloc(1, sizeof(RSO));
  new_rso->short_name = strdup(short_name);
  new_rso->dll_handle = dll_handle;
  new_rso->call_fn = cif->call_fn;
  new_rso->closure_fn = cif->closure_fn;
  RSO *old_rso_list = rso_list;
  do {new_rso->next = old_rso_list;}
  while (__cs(&old_rso_list, &rso_list, &new_rso));
  if (ffi_show_progress)
    ffi_printf("loaded shared object for FFI %s\n", short_name);
}

static int load_shared_object(ffi_cif *cif, char *short_name, char *directory, int allow_message)
{
  char so_filename[256];
  snprintf(so_filename, sizeof(so_filename), "%s/%s.so", directory, short_name);

  void *dll_handle = dlopen(so_filename, RTLD_NOW+RTLD_LOCAL);
  if (dll_handle == NULL) {
    if (allow_message) ffi_printf("Failed to load %s: %s\n", so_filename, dlerror());
    return 1;
  }
  cif->call_fn = dlsym(dll_handle, "_ffi_call");
  if (cif->call_fn == NULL) {
    ffi_printf("Failed to find %s in %s: %s\n", "_ffi_call", so_filename, dlerror());
    dlclose(dll_handle);
    return 2;
  }
  cif->closure_fn = dlsym(dll_handle, "_ffi_closure");
  if (cif->closure_fn == NULL) {
    if (allow_message) ffi_printf("Failed to find %s in %s: %s\n", "_ffi_closure", so_filename, dlerror());
    cif->call_fn = NULL;
    dlclose(dll_handle);
    return 3;
  }
  register_shared_object(cif, dll_handle, short_name);
  return 0;
}

static int build_and_load_shared_object(ffi_cif *cif, struct_info **sip, char *short_name, char *base_directory)
{
  if (ffi_show_progress)
    ffi_printf("building shared object for FFI %s\n", short_name);
  if (mkdir(base_directory, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) && errno != EEXIST) {
    ffi_printf("failed to create the directory %s, errno=%d (%s)\n", base_directory, errno, strerror(errno));
    return 1;
  }
  char base_filename[256];
  snprintf(base_filename, sizeof(base_filename), "%s/%s", base_directory, short_name);
  char c_filename[256];
  snprintf(c_filename, sizeof(c_filename), "%s.c", base_filename);
  FILE *out = fopen(c_filename, "wt");
  if (out == NULL) {
    ffi_printf("failed to open %s for writing, errno=%d (%s)\n", c_filename, errno, strerror(errno));
    return 2;
  }
  write_ffi_call_fn_and_closure_fn(out, cif, sip);
  if (ferror(out)) {
    ffi_printf("error while writing %s\n", c_filename);
    return 3;
  }
  fclose(out);
  char so_filename[256];
  snprintf(so_filename, sizeof(so_filename), "%s.so", base_filename);
  char list_filename[256];
  snprintf(list_filename, sizeof(list_filename), "%s.list", base_filename);
  char *ffi_listing = getenv("FFI_LIST");
  int make_listing = ffi_listing && (0==strcmp(ffi_listing, "YES") || 0==strcmp(ffi_listing, "yes"));
  char *ffi_debug = getenv("FFI_DEBUG");
  int debug = ffi_debug && (0==strcmp(ffi_debug, "YES") || 0==strcmp(ffi_debug, "yes"));
  char *ffi_command = getenv("FFI_COMMAND");
  if (ffi_command == NULL)
    ffi_command = DEFAULT_FFI_COMMAND;
#if __CHARSET_LIB
#define tag "ISO8859-1"
#else
#define tag "IBM-1047"
#endif
  char command[1024];
  snprintf(command, sizeof(command), "cd %s; /bin/chtag -t -c %s %s; %s%s%s -o %s %s > %s 2>&1", /* avoid -g; it will break closures */
	   base_directory, tag, c_filename, ffi_command,
	   debug ? "-qgonum " : "", make_listing ? "-Wc,list " : "",
	   so_filename, c_filename,
	   list_filename);
  if (debug) {
    int len = strlen(command);
    snprintf(command+len, sizeof(command)-len, 
	     "; echo ' LISTOBJ' | amblist %s.o > %s.o.amblist; echo ' LISTLOAD' | amblist %s.so > %s.so.amblist", 
	     base_filename, base_filename, base_filename, base_filename);
  }
  if (ffi_show_progress || debug) {
    ffi_printf("about to run: %s\n", command);
  }
  int rc = system(command);
  if (rc != 0 || ffi_show_progress || debug) {
    ffi_printf("compile command returned %d\n", rc);
    /* might as well try to load it */
  }
  if (load_shared_object(cif, short_name, base_directory, 1))
    return 4;
  if (!debug) {
    remove(list_filename);
  }
  return 0;
}

int ffi_obtain_call_fn_and_closure_fn(ffi_cif *cif)
{
  char *ffi_sp = getenv("FFI_SHOW_PROGRESS");
  ffi_show_progress = (ffi_sp && (0==strcmp(ffi_sp, "YES") || 0==strcmp(ffi_sp, "yes")));

  struct_info *first_si = NULL;
  struct_info **sip = &first_si;
  collect_struct_types(cif->rtype, &first_si);
  for (int i=0; i<cif->nargs; i++) {
    collect_struct_types(cif->arg_types[i], &first_si);
  }
  char short_name[256];
  int result;
  if (0 != (result = short_name_for_fn(short_name, sizeof(short_name), cif, sip))) {
    return result;
  }
  if (find_registered_shared_object(cif, short_name)) {
    if (ffi_show_progress)
      ffi_printf("the shared object for FFI %s is already loaded\n", short_name);
    return 0;
  }

  char *ffi_lib = getenv("FFI_LIB");
  if (ffi_lib && 0 == load_shared_object(cif, short_name, ffi_lib, 0)) {
    if (ffi_show_progress)
      ffi_printf("the shared object for FFI %s was successfully loaded from %s\n", short_name, ffi_lib);
    return 0;
  }

  char *ffi_local_lib = getenv("FFI_LOCAL_LIB");
  char base_directory[256];
  if (ffi_local_lib) {
    strncpy(base_directory, ffi_local_lib, sizeof(base_directory));
  } else {
    snprintf(base_directory, sizeof(base_directory), "%s/.ffi", getenv("HOME"));
  }
  if (0 == load_shared_object(cif, short_name, base_directory, 0)) {
    if (ffi_show_progress)
      ffi_printf("the shared object for FFI %s was successfully loaded from %s\n", short_name, base_directory);
    return 0;
  }
    
  if (0 != (result = build_and_load_shared_object(cif, sip, short_name, base_directory)))
    return result;
  if (ffi_show_progress)
    ffi_printf("the shared object for FFI %s was successfully built and loaded from %s\n", short_name, base_directory);
  return 0;
}

/* search path of directories */
/* each directory has an index */
/* all the source files are kept */
/* all the so files are kept */

ffi_status ffi_prep_cif (ffi_cif *cif, ffi_abi abi, unsigned int nargs, ffi_type *rtype, ffi_type **arg_types)
{
  memset(cif, 0, sizeof(*cif));
  cif->abi = abi;
  cif->nargs = nargs;
  cif->nfixedargs = nargs;
  cif->is_varargs = 0;
  cif->rtype = rtype;
  cif->arg_types = arg_types;
  ffi_obtain_call_fn_and_closure_fn(cif);
  return FFI_OK;
}

ffi_status ffi_prep_cif_var(ffi_cif *cif,
			    ffi_abi abi,
			    unsigned int nfixedargs,
			    unsigned int ntotalargs,
			    ffi_type *rtype,
			    ffi_type **arg_types)
{
  memset(cif, 0, sizeof(*cif));
  cif->abi = abi;
  cif->nargs = ntotalargs;
  cif->nfixedargs = nfixedargs;
  cif->is_varargs = 1;
  cif->rtype = rtype;
  cif->arg_types = arg_types;
  ffi_obtain_call_fn_and_closure_fn(cif);
  return FFI_OK;
}

typedef void ffi_call_fn(ffi_cif *cif,
			 void_fn_type *fn,
			 void *rvalue,
			 void **avalue);

void ffi_call(ffi_cif *cif,
	      void_fn_type *fn,
	      void *rvalue,
	      void **avalue)
{
  if (cif->call_fn == NULL) {
    ffi_printf("No ffi call function could be found or created\n");
  }
  ((ffi_call_fn *)cif->call_fn)(cif, fn, rvalue, avalue);
}

/* Allocate a chunk of memory holding size bytes. 
   This returns a pointer to the writable address, and sets *code to the corresponding executable address.
   size should be sufficient to hold a ffi_closure object. */
FFI_API void *ffi_closure_alloc (size_t size, void **code)
{
  size = sizeof(ffi_closure);
  ffi_closure *result = (ffi_closure *)malloc(size);
  *code = (void *)&result->fn_env;
  return result;
}


/* Free memory allocated using ffi_closure_alloc. The argument is the writable address that was returned.
   Once you have allocated the memory for a closure, you must construct a ffi_cif describing the function call. 
   Finally you can prepare the closure function:*/
FFI_API void ffi_closure_free (void *writable)
{
  free(writable);
}

ffi_status
ffi_prep_closure (ffi_closure* closure,
		  ffi_cif* cif,
		  void (*fun)(ffi_cif*,void*,void**,void*),
		  void *user_data)
{
  return ffi_prep_closure_loc (closure, cif, fun, user_data, closure);
}

/* Prepare a closure function.
closure   is the address of a ffi_closure object; this is the writable address returned by ffi_closure_alloc.
cif       is the ffi_cif describing the function parameters.
user_data is an arbitrary datum that is passed, uninterpreted, to your closure function.
codeloc   is the executable address returned by ffi_closure_alloc.
fun       is the function which will be called when the closure is invoked. It is called with the arguments:

cif       The ffi_cif passed to ffi_prep_closure_loc. 
ret       A pointer to the memory used for the function's return value. fun must fill this, unless the function is declared as returning void. 
args      A vector of pointers to memory holding the arguments to the function. 
user_data The same user_data that was passed to ffi_prep_closure_loc.
*/

FFI_API ffi_status ffi_prep_closure_loc (ffi_closure *closure, ffi_cif *cif, void (*fun)(ffi_cif*,void*,void**,void*), 
                                         void *user_data, void *codeloc)
{
  if (cif->closure_fn == NULL) {
    ffi_printf("No ffi call function could be found or created\n");
  }
  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;
  closure->fn_env = closure;
  closure->fn_code = ((void **)cif->closure_fn)[1];
  return FFI_OK;
}

