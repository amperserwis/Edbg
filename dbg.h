/*
 * Copyright (c) 2013-2015, Alex Taradov <alex@taradov.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DBG_H_
#define _DBG_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/*- Definitions -------------------------------------------------------------*/
#define DBG_VID_ATMEL             0x03eb
#define DBG_PID_ATMEL_EDBG          0x2111
#define DBG_PID_ATMEL_MEDBG         0x2145
#define DBG_PID_ATMEL_ICE           0x2141
#define DBG_PID_ARDUINO_ZERO        0x2157

/*- Types -------------------------------------------------------------------*/
typedef struct
{
  uint16_t vendor_id;
  const uint16_t* products;
} dap_vendor_t;

typedef struct
{
  char     *path;
  char     *serial;
  wchar_t  *wserial;
  char     *manufacturer;
  char     *product;
  uint16_t  VID;
  uint16_t  PID;
} debugger_t;

/*- Prototypes --------------------------------------------------------------*/
int dbg_enumerate(debugger_t *debuggers, int size);
void dbg_open(debugger_t *debugger);
void dbg_close(void);
int dbg_dap_cmd(uint8_t *data, int size, int rsize);

int dbg_validate_dap(uint16_t vendorid, uint16_t productid);

#endif // _DBG_H_

