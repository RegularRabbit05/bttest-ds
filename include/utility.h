#ifndef _UTILITY_H
#define _UTILITY_H

#include "bcm2070b0_nds_spi.h"

typedef struct {
    u8 bdaddr[6];
    u8 pageScanRepMode;
    u16 clockOffset;
    char name[32];
} BTDevice;

typedef struct {
    BTDevice devices[32];
    u8 count;
} BTDevices;

void printSuccess(char const *s);
void printError(char const *s);
u32 waitForKey(void);
char const *regionAsString(BTRegion const region);

bool hciReset(void);
void hciClientSetName(char *name);
void hciClientPairingMode(u8 mode);
void hciClientInquiryScanParameters(u16 interval, u16 window);
void hciClientPageScanParameters(u16 interval, u16 window);
BTDevices hciScanDevices(u8 len);
void hciRemoteNameRequest(BTDevice *dev);
bool hciRemoteConnect(BTDevice *dev, u16 pktType, u8 allowSwitch);

#endif /* _UTILITY_H */