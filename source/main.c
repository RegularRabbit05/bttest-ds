#include "bcm2070b0_nds_spi.h"
#include "utility.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
  // Init screens.
  videoSetMode(MODE_0_2D);
  videoSetModeSub(MODE_0_2D);
  vramSetBankA(VRAM_A_MAIN_BG);
  consoleInit(NULL, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);

  // Enable card access.
  enableSlot1();

  // Main loop.
  bool quit = false;
  while (true) {
    consoleClear();

    // Get region.
    BTRegion region = btRegion();
    while (region == BTRegion_Unknown) {
      printError("Unknown cartridge/region!");
      iprintf("Press any key to retry...\n");
      waitForKey();
      consoleClear();
      region = btRegion();
    }

    swiDelay(0x82EA * 200); // 200ms

    iprintf("Region: %s\n", regionAsString(region));
    iprintf("> A: Begin Testing To\n");
    iprintf("> B: Begin Testing From\n");
    iprintf("> Other: Quit\n");

    u32 opt = waitForKey();
    if (opt & KEY_A) {
      if (hciReset()) {
        hciClientSetName("BCM2070");

        BTDevices devs = hciScanDevices(0x04);
        u8 selected = 0;
        if (devs.count == 0) goto skip;
        iprintf("Devices found:\n");
        for (u8 d = 0; d < devs.count; d++) iprintf("%03d %s\n", d, devs.devices[d].name);
        do {
          iprintf("%d     \r", selected);
          opt = waitForKey();
          if (opt & KEY_DOWN && selected < devs.count-1) selected++;
          if (opt & KEY_UP && selected != 0) selected--;
          if (opt & KEY_B) goto skip;
        } while (!(opt & KEY_A));
        iprintf("\n");
        hciRemoteConnect(&devs.devices[selected], 0xCC18, 0);
      }
    } else if (opt & KEY_B) {
      if (hciReset()) {
        hciClientSetName("BCM2070");
        hciClientInquiryScanParameters(0x0800, 0x0012);
        hciClientPageScanParameters(0x0800, 0x0012);
        hciClientPairingMode(0x03);
      }
    } else {
      quit = true;
    }
    skip:

    if (quit)
      break;

    iprintf("Press any key to continue...\n");
    waitForKey();
  }

  return 0;
}