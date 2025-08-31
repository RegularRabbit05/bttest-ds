#include "utility.h"

#include <stdio.h>

// Helpers

static void printColor(char const *s, bool const fail) {
  if (fail)
    iprintf("\x1b[31;1m");
  else
    iprintf("\x1b[32;1m");

  iprintf("%s\x1b[39m\n", s);
}

// Utility

void printSuccess(char const *s) { printColor(s, false); }
void printError(char const *s) { printColor(s, true); }

u32 waitForKey(void) {
  u32 key = 0;
  while (1) {
    swiWaitForVBlank();
    scanKeys();
    key = keysDown();
    if (key)
      break;
  }

  return key;
}

char const *regionAsString(BTRegion const region) {
  switch (region) {
  case BTRegion_JPN:
    return "Japan";
  case BTRegion_ITA:
    return "Italy";
  case BTRegion_ENG:
    return "United Kingdom";
  case BTRegion_SPA:
    return "Spain";
  case BTRegion_GER:
    return "Germany";
  case BTRegion_FRA:
    return "France";
  default:
  }

  return "(UNKNOWN)";
}

void printBtData(BTData *in) {
  char buffer[128] = "";
  char tmp[8];
  for (int i = 0; i < in->requestSize; i++) {
    sprintf(tmp, "%x ", in->request[i]);
    strcat(buffer, tmp);
  }
  strcat(buffer, ": ");
  for (int i = 0; i < in->responseSize; i++) {
    sprintf(tmp, "%x ", in->response[i]);
    strcat(buffer, tmp);
  }
  strcat(buffer, "\n");
  iprintf(buffer);
}

// Bt

bool hciReset(void) {
  const u8 buffer[] = {0x01, 0x03, 0x0C, 0x00};
  u8 out[7] = {};
  BTData data;

  data.request = buffer;
  data.requestSize = sizeof(buffer);
  data.response = out;
  data.responseSize = sizeof(out);
  btTransfer(&data);
  printBtData(&data);
  return data.responseSize == 7;
}

void hciClientSetName(char *name) {
  u8 buffer[248 + 4] = {0x01, 0x13, 0x0C, 0xF8, 0};
  strcat((char*) buffer, name);
  u8 out[1024] = {};
  BTData data;

  data.request = buffer;
  data.requestSize = sizeof(buffer);
  data.response = out;
  data.responseSize = sizeof(out);
  btTransfer(&data);
}

void hciClientPairingMode(u8 mode) {
  const u8 buffer[] = {0x01, 0x1A, 0x0C , mode, 0x01};
  u8 out[1024] = {};
  BTData data;

  data.request = buffer;
  data.requestSize = sizeof(buffer);
  data.response = out;
  data.responseSize = sizeof(out);
  btTransfer(&data);
  printBtData(&data);
}

void hciClientInquiryScanParameters(u16 interval, u16 window) {
  const u8 buffer[] = {0x01, 0x1E, 0x0C, 0x04 , interval & 0xFF, interval >> 8, window & 0xFF, window >> 8};
  u8 out[1024] = {};
  BTData data;

  data.request = buffer;
  data.requestSize = sizeof(buffer);
  data.response = out;
  data.responseSize = sizeof(out);
  btTransfer(&data);
  printBtData(&data);
}

void hciClientPageScanParameters(u16 interval, u16 window) {
  const u8 buffer[] = {0x01, 0x1E, 0x0C, 0x04 , interval & 0xFF, interval >> 8, window & 0xFF, window >> 8};
  u8 out[1024] = {};
  BTData data;

  data.request = buffer;
  data.requestSize = sizeof(buffer);
  data.response = out;
  data.responseSize = sizeof(out);
  btTransfer(&data);
  printBtData(&data);
}

BTDevices hciScanDevices(u8 len) {
  const u8 buffer[] = {0x01, 0x01, 0x04, 0x05, 0x33, 0x8B, 0x9E, len, 0x00};
  const u8 completeCmd[] = {4, 1, 1, 0};
  u8 out[1024] = {};
  BTData data;

  data.request = buffer;
  data.requestSize = sizeof(buffer);
  data.response = out;
  data.responseSize = sizeof(out);
  btTransfer(&data);
  printBtData(&data);

  BTDevices queue;
  memset(&queue, 0, sizeof(queue));

  data.requestSize = 0;
  while (true) {
    swiDelay(0x82EA * 500);
    data.responseSize = sizeof(out);
    btRead(&data);
    if (data.responseSize == 0) continue;
    if (data.responseSize == 4 && memcmp(data.response, completeCmd, 4) == 0) break;

    u8 evtCode = data.response[1];
    if (evtCode == 0x2F || evtCode == 0x02) {
      u8 *evt = data.response + 3;
      u8 numResponses = evt[0];
      for (int i = 0; i < numResponses; i++) {
        if (queue.count >= sizeof(queue.devices) / sizeof(BTDevice)) break;

        BTDevice q;
        for (int j = 0; j < 6; j++) q.bdaddr[j] = evt[1 + i*13 + j];
        q.pageScanRepMode = evt[7 + i*13];
        q.clockOffset = evt[11 + i*13] | (evt[12 + i*13] << 8);
        q.name[0] = 0;
        queue.devices[queue.count] = q;
        queue.count++;
      }
    }
  }

  for (int i = 0; i < queue.count; i++) hciRemoteNameRequest(&queue.devices[i]);
  return queue;
}

void hciRemoteNameRequest(BTDevice *dev) {
  const u8 buffer[14] = {0x01, 0x19, 0x04, 0x0A, dev->bdaddr[0], dev->bdaddr[1], dev->bdaddr[2], dev->bdaddr[3], dev->bdaddr[4], dev->bdaddr[5], dev->pageScanRepMode, 0, (u8)(dev->clockOffset & 0xFF), (u8)((dev->clockOffset >> 8) & 0xFF)};
  u8 out[1024] = {};
  BTData data;

  data.request = buffer;
  data.requestSize = sizeof(buffer);
  data.response = out;
  data.responseSize = sizeof(out);
  btTransfer(&data);

  for (int i = 0; i < 10; i++) {
    swiDelay(0x82EA * 500);
    data.requestSize = 0;
    data.responseSize = sizeof(out);
    btRead(&data);
    if (data.responseSize <= 10) continue;
    if (data.response[1] != 0x07) continue;
    strncpy((char*) dev->name, (char*) data.response + 10, 31);
    dev->name[31] = 0;
    break;
  }
}

bool hciRemoteConnect(BTDevice *dev, u16 pktType, u8 allowSwitch) {
  bool success = false;
  const u8 buffer[] = {0x01, 0x05, 0x04, 0x0D, dev->bdaddr[0], dev->bdaddr[1], dev->bdaddr[2], dev->bdaddr[3], dev->bdaddr[4], dev->bdaddr[5], pktType & 0xFF, pktType >> 8, dev->pageScanRepMode, 0, (u8)(dev->clockOffset & 0xFF), (u8)((dev->clockOffset >> 8) & 0xFF), allowSwitch};
  u8 out[1024] = {};
  BTData data;

  data.request = buffer;
  data.requestSize = sizeof(buffer);
  data.response = out;
  data.responseSize = sizeof(out);
  btTransfer(&data);
  printBtData(&data);

  for (int i = 0; i < 40; i++) {
    swiDelay(0x82EA * 500);
    data.requestSize = 0;
    data.responseSize = sizeof(out);
    btRead(&data);
    if (data.responseSize == 0) continue;
    printBtData(&data);
  }

  return success;
}