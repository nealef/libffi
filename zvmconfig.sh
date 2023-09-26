#!/rocket/conda/bin/bash
 
######################### VARIABLES ##########################################
PREFIX="/usr/local/"
 
 
OFLAGS="-O"
QFLAGS="-qlanglvl=extended:extc89:extc99 -qxplink -qdll -qenum=int -qexportall \
	        -qfloat=ieee -qlongname -q32 -qseverity=e=CCN3296 -qascii"
DFLAGS="-D_ALL_SOURCE -D_EXT -D_UNIX03_SOURCE -D_XOPEN_SOURCE_EXTENDED=1 -D__VM__ \
	        -D_ISOC99_SOURCE -DEBCDIC -D_OE_SOCKETS -D_OPEN_MSGQ_EXT -D_OPEN_SYS \
		        -D_OPEN_THREADS -D_POSIX_SOURCE -D_UNIX03_SOURCE \
			        -D_UNIX03_WITHDRAWN -D_XOPEN_SOURCE=600 \
		-DNCURSES_CONST=const -I/usr/local/include"
 
CFLAGS="${OFLAGS} ${QFLAGS} ${DFLAGS}"
CC="xlc"
CXX="xlc"
CPP="xlc -E"
QXXFLAGS="-qlanglvl=extended -qxplink -qdll -qenum=int -qexportall -qascii \
	          -qfloat=ieee -qlongname"
CXXFLAGS="-+ ${OFLAGS} ${QXXFLAGS} ${DFLAGS}"
 
QLFLAGS="-qxplink -qdll"
LDFLAGS="${OFLAGS} ${QLFLAGS}"
CPPFLAGS="${DFLAGS}"
 
########################## OVERRIDES #########################################
export ac_cv_func_accept4='no'
export ac_cv_func_clock_gettime='no'
export ac_cv_func_copysign='no'
export ac_cv_func_fstatat='no'
export ac_cv_func_faccessat='no'
export ac_cv_func_fchdir='no'
export ac_cv_func_fchmodat='no'
export ac_cv_func_fchownat='no'
export ac_cv_func_fseek64='no'
export ac_cv_func_fseeko='no'
export ac_cv_func_fstatat='no'
export ac_cv_func_ftell64='no'
export ac_cv_func_ftello='no'
export ac_cv_func_futimes='no'
export ac_cv_func_futimesat='no'
export ac_cv_func_getc_unlocked='no'
export ac_cv_func_getgrgid_r='no'
export ac_cv_func_getgrnam_r='no'
export ac_cv_func_gethostbyname_r='no'
export ac_cv_func_gethostbyname_r_6_arg='no'
export ac_cv_func_getitimer='no'
export ac_cv_func_getpriority='no'
export ac_cv_func_getpwnam_r='no'
export ac_cv_func_getpwuid_r='no'
export ac_cv_func_getsid='no'
export ac_cv_func_initgroups='no'
export ac_cv_func_lchown='no'
export ac_cv_func_log2='no'
export ac_cv_func_linkat='no'
export ac_cv_func_mkdirat='no'
export ac_cv_func_mkfifoat='no'
export ac_cv_func_mknodat='no'
export ac_cv_func_mmap='no'
export ac_cv_func_unlinkat='no'
export ac_cv_func_wait3='no'
export ac_cv_func_wait4='no'
export ac_cv_func_dup3='no'
export ac_cv_func_pipe2='no'
export ac_cv_func_poll='no'
export ac_cv_func_posix_fallocate='no'
export ac_cv_func_pselect='no'
export ac_cv_func_readlinkat='no'
export ac_cv_func_renameat='no'
export ac_cv_func_setbuf='no'
export ac_cv_func_setbuffer='no'
export ac_cv_func_setgroups='no'
export ac_cv_func_sethostname='no'
export ac_cv_func_setitimer='no'
export ac_cv_func_setpgrp='no'
export ac_cv_func_setpriority='no'
export ac_cv_func_setregid='no'
export ac_cv_func_setresgid='no'
export ac_cv_func_setresuid='no'
export ac_cv_func_setreuid='no'
export ac_cv_func_setvbuf='no'
export ac_cv_func_sigaltstack='no'
export ac_cv_func_sigafillset='no'
export ac_cv_func_siginterrupt='no'
export ac_cv_func_sigrelse='no'
export ac_cv_func_sigtimedwait='no'
export ac_cv_func_sigwaitinfo='no'
export ac_cv_func_strftime='no'
export ac_cv_func_symlinkat='no'
export ac_cv_func_sync='no'
export cf_cv_working_poll='no'
export ac_cv_header_poll='no'
export ac_cv_header_curses='no'
export ac_cv_prog_GREP="grep -W filecodeset=ISO8859-1,pgmcodeset=IBM-1047"
export CONFIG_SHELL='/rocket/conda/bin/bash'

######################### MAIN ###############################################
export ax_cv_c_float_words_bigendian=yes 
./configure --prefix=${PREFIX} CC="${CC}" CFLAGS="${CFLAGS}" CXX="${CXX}" \
	            CXXFLAGS="${CXXFLAGS}" LDFLAGS="${LDFLAGS}" CPP="${CPP}" \
		                CPPFLAGS="${CPPFLAGS}" \
                        --enable-shared
 
chtag -R -h -t -cISO8859-1 .

exit 0
##############################################################################
