#ifndef SIO_DBG_MACRO_H
#define SIO_DBG_MACRO_H

/*
  2021.12 Jiayu notes:
    this code is copied from: https://github.com/sharkdp/dbg-macro/dbg.h
    commit 4db61805d90cb66d91bcc56c2703591a0127ed11
    It is a C++ version of Rust's dbg!() macro
  
  License: MIT

  Integration tips:
    1. DBG_MACRO_DISABLE flag: 
      disable the dbg(…) macro (i.e. to make it a no-op).
    2. DBG_MACRO_NO_WARNING flag:
      disable the "'dbg.h' header is included in your code base" warnings.
*/

//#define DBG_MACRO_DISABLE
#define DBG_MACRO_NO_WARNING

#include "sio/dbg.h"

#endif

