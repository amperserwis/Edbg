// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2013-2022, Alex Taradov <alex@taradov.com>. All rights reserved.

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include <setupapi.h>
// NOTE: Different versions of MinGW seem to place those files in differnet
//       locations. If you see build errors here, then use includes with
//       'ddk/' prefix. The no-prefix version seems to be the best moving
//       forward, so consider updating your version of MinGW. MinGW-W64
//       appears to have most recent vestions of the tools, and that's
//       what I'm currently using.
#if 0
#include <ddk/hidsdi.h>
#include <ddk/hidpi.h>
#else
#include <hidsdi.h>
#include <hidpi.h>
#endif
#include "dbg.h"
#include "edbg.h"

/*- Definitions -------------------------------------------------------------*/
#define MAX_STRING_SIZE   256

/*- Variables ---------------------------------------------------------------*/
static HANDLE debugger_handle = INVALID_HANDLE_VALUE;
static uint8_t hid_buffer[DBG_MAX_EP_SIZE + 1];
static int report_size = 0;

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
int dbg_enumerate(debugger_t *debuggers, int size)
{
  GUID hid_guid;
  HDEVINFO hid_dev_info;
  HIDD_ATTRIBUTES hid_attr;
  SP_DEVICE_INTERFACE_DATA dev_info_data;
  PSP_DEVICE_INTERFACE_DETAIL_DATA detail_data;
  DWORD detail_size;
  HANDLE handle;
  int rsize = 0;

  HidD_GetHidGuid(&hid_guid);

  hid_dev_info = SetupDiGetClassDevs(&hid_guid, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);

  dev_info_data.cbSize = sizeof(dev_info_data);

  for (int i = 0; rsize < size; i++)
  {
    if (FALSE == SetupDiEnumDeviceInterfaces(hid_dev_info, 0, &hid_guid, i, &dev_info_data))
      break;

    SetupDiGetDeviceInterfaceDetail(hid_dev_info, &dev_info_data, NULL, 0,
        &detail_size, NULL);

    detail_data = (PSP_DEVICE_INTERFACE_DETAIL_DATA)buf_alloc(detail_size);
    detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    SetupDiGetDeviceInterfaceDetail(hid_dev_info, &dev_info_data, detail_data,
        detail_size, NULL, NULL);

    handle = CreateFile(detail_data->DevicePath, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (INVALID_HANDLE_VALUE != handle)
    {
      wchar_t wstr[MAX_STRING_SIZE];
      char str[MAX_STRING_SIZE];

      wstr[0] = 0;
      str[0] = 0;

      hid_attr.Size = sizeof(hid_attr);
      HidD_GetAttributes(handle, &hid_attr);

      debuggers[rsize].path = strdup(detail_data->DevicePath);

      HidD_GetSerialNumberString(handle, (PVOID)wstr, MAX_STRING_SIZE);
      wcstombs(str, wstr, MAX_STRING_SIZE);
      debuggers[rsize].serial = strdup(str);

      HidD_GetManufacturerString(handle, (PVOID)wstr, MAX_STRING_SIZE);
      wcstombs(str, wstr, MAX_STRING_SIZE);
      debuggers[rsize].manufacturer = strdup(str);

      HidD_GetProductString(handle, (PVOID)wstr, MAX_STRING_SIZE);
      wcstombs(str, wstr, MAX_STRING_SIZE);
      debuggers[rsize].product = strdup(str);

      debuggers[rsize].vid = hid_attr.VendorID;
      debuggers[rsize].pid = hid_attr.ProductID;

      debuggers[rsize].versions = DBG_CMSIS_DAP_V1;

      if (strstr(debuggers[rsize].product, "CMSIS-DAP"))
        rsize++;

      CloseHandle(handle);
    }

    buf_free(detail_data);
  }

  SetupDiDestroyDeviceInfoList(hid_dev_info);

  return rsize;
}

//-----------------------------------------------------------------------------
void dbg_open(debugger_t *debugger, int version)
{
  HIDP_CAPS caps;
  PHIDP_PREPARSED_DATA prep;
  int input, output;

  debugger_handle = CreateFile(debugger->path, GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

  if (INVALID_HANDLE_VALUE == debugger_handle)
    error_exit("unable to open device %s: 0x%08x",  debugger->path, GetLastError());

  HidD_GetPreparsedData(debugger_handle, &prep);
  HidP_GetCaps(prep, &caps);

  input = caps.InputReportByteLength - 1;
  output = caps.OutputReportByteLength - 1;

  if (input != output)
    error_exit("input and output report sizes do not match");

  if (64 != input && 512 != input && 1024 != input)
    error_exit("detected report size (%d) is not 64, 512 or 1024", input);

  report_size = input;

  (void)version;
}

//-----------------------------------------------------------------------------
void dbg_close(void)
{
  if (INVALID_HANDLE_VALUE != debugger_handle)
    CloseHandle(debugger_handle);
}

//-----------------------------------------------------------------------------
int dbg_get_packet_size(void)
{
  return report_size;
}

//-----------------------------------------------------------------------------
int dbg_dap_cmd(uint8_t *data, int resp_size, int req_size)
{
  uint8_t cmd = data[0];
  DWORD res;

  memset(hid_buffer, 0xff, report_size + 1);

  hid_buffer[0] = 0x00; // Report ID
  memcpy(&hid_buffer[1], data, req_size);

  if (FALSE == WriteFile(debugger_handle, (LPCVOID)hid_buffer, report_size + 1, &res, NULL))
    error_exit("debugger write()");

  if (FALSE == ReadFile(debugger_handle, (LPVOID)hid_buffer, report_size + 1, &res, NULL))
    error_exit("debugger read()");

  if (hid_buffer[1] != cmd)
    error_exit("invalid response received: request = 0x%02x, response = 0x%02x", cmd, hid_buffer[1]);

  res -= 2;
  memcpy(data, &hid_buffer[2], (resp_size < (int)res) ? resp_size : (int)res);

  return res;
}

