/*
  Modified by Matthew Heironimus to support HID Report Descriptors to be in 
  standard RAM in addition to program memory (PROGMEM).

  Copyright (c) 2015, Arduino LLC
  Original code (pre-library): Copyright (c) 2011, Peter Barrett

  Permission to use, copy, modify, and/or distribute this software for
  any purpose with or without fee is hereby granted, provided that the
  above copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
  WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
  BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
  OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
  SOFTWARE.
 */

#ifndef DYNAMIC_HID_h
#define DYNAMIC_HID_h

#include <stdint.h>
#include <Arduino.h>

#ifdef _VARIANT_ARDUINO_DUE_X_
  // The following values are the same as AVR's USBAPI.h
  // Reproduced here because SAM doesn't have these in
  // its own USBAPI.H
  #define USB_EP_SIZE EP0_SIZE
  #define TRANSFER_PGM 0x80

  #include "USB/PluggableUSB.h"
#else
  #include "PluggableUSB.h"
#endif

#include "PIDReportHandler.h"

#if defined(USBCON)

#define _USING_DYNAMIC_HID

// DYNAMIC_HID 'Driver'
// ------------
#define DYNAMIC_HID_GET_REPORT        0x01
#define DYNAMIC_HID_GET_IDLE          0x02
#define DYNAMIC_HID_GET_PROTOCOL      0x03
#define DYNAMIC_HID_SET_REPORT        0x09
#define DYNAMIC_HID_SET_IDLE          0x0A
#define DYNAMIC_HID_SET_PROTOCOL      0x0B

#define DYNAMIC_HID_DESCRIPTOR_TYPE         0x21
#define DYNAMIC_HID_REPORT_DESCRIPTOR_TYPE      0x22
#define DYNAMIC_HID_PHYSICAL_DESCRIPTOR_TYPE    0x23

// HID subclass HID1.11 Page 8 4.2 Subclass
#define DYNAMIC_HID_SUBCLASS_NONE 0
#define DYNAMIC_HID_SUBCLASS_BOOT_INTERFACE 1

// HID Keyboard/Mouse bios compatible protocols HID1.11 Page 9 4.3 Protocols
#define DYNAMIC_HID_PROTOCOL_NONE 0
#define DYNAMIC_HID_PROTOCOL_KEYBOARD 1
#define DYNAMIC_HID_PROTOCOL_MOUSE 2

// Normal or bios protocol (Keyboard/Mouse) HID1.11 Page 54 7.2.5 Get_Protocol Request
// "protocol" variable is used for this purpose.
#define DYNAMIC_HID_BOOT_PROTOCOL	0
#define DYNAMIC_HID_REPORT_PROTOCOL	1

// HID Request Type HID1.11 Page 51 7.2.1 Get_Report Request
#define DYNAMIC_HID_REPORT_TYPE_INPUT   1
#define DYNAMIC_HID_REPORT_TYPE_OUTPUT  2
#define DYNAMIC_HID_REPORT_TYPE_FEATURE 3

#define PID_ENPOINT_COUNT 2

#define PID_ENDPOINT_IN	 (pluggedEndpoint)
#define PID_ENDPOINT_OUT (pluggedEndpoint+1)

typedef struct
{
  uint8_t len;      // 9
  uint8_t dtype;    // 0x21
  uint8_t addr;
  uint8_t versionL; // 0x101
  uint8_t versionH; // 0x101
  uint8_t country;
  uint8_t desctype; // 0x22 report
  uint8_t descLenL;
  uint8_t descLenH;
} DYNAMIC_HIDDescDescriptor;

typedef struct 
{
  InterfaceDescriptor hid;
  DYNAMIC_HIDDescDescriptor   desc;
  EndpointDescriptor  in;
  EndpointDescriptor  out;
} DYNAMIC_HIDDescriptor;

class DynamicHIDSubDescriptor {
public:
  DynamicHIDSubDescriptor *next = NULL;
  DynamicHIDSubDescriptor(const void *d, const uint16_t l, const void* pid_d, const uint16_t pid_l, const bool ipm = true) : data(d), length(l),pid_data(pid_d), pid_length(pid_l), inProgMem(ipm) { }

  const void* data;
  const uint16_t length;
  const void* pid_data;
  const uint16_t pid_length;
  const bool inProgMem;
};

class DynamicHID_ : public PluggableUSBModule
{
public:
  DynamicHID_(void);
  int begin(void);
  bool usb_Available();
  int SendReport(uint8_t id, const void* data, int len);
  int RecvData(byte* data);
  void RecvfromUsb();
  void AppendDescriptor(DynamicHIDSubDescriptor* node);
  PIDReportHandler pidReportHandler;

protected:
  // Implementation of the PluggableUSBModule
  int getInterface(uint8_t* interfaceCount);
  int getDescriptor(USBSetup& setup);
  bool GetReport(USBSetup& setup);
  bool SetReport(USBSetup& setup);
  bool setup(USBSetup& setup);
  uint8_t getShortName(char* name);

private:
  #ifdef _VARIANT_ARDUINO_DUE_X_
  uint32_t epType[2];
  #else
  uint8_t epType[2];
  #endif

  uint8_t out_ffbdata[USB_EP_SIZE];
  DynamicHIDSubDescriptor* rootNode;
  uint16_t descriptorSize;

  uint8_t protocol;
  uint8_t idle;
};

// Replacement for global singleton.
// This function prevents static-initialization-order-fiasco
// https://isocpp.org/wiki/faq/ctors#static-init-order-on-first-use
DynamicHID_& DynamicHID();

#define D_HIDREPORT(length) { 9, 0x21, 0x11, 0x01, 0, 1, 0x22, lowByte(length), highByte(length) }

#endif // USBCON

#endif // DYNAMIC_HID_h
