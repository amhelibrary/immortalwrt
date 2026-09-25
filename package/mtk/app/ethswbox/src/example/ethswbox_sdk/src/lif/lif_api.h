/******************************************************************************

    Copyright 2022 Maxlinear

    SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0-only)

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.
******************************************************************************/

#ifndef LIF_API_H
#define LIF_API_H

int32_t lif_mdio_init(char *lib);
int32_t lif_mdio_deinit(char *lib);
int32_t lif_mdio_open(char *lib, uint8_t clk_pin, uint8_t data_pin);
int32_t lif_mdio_close(char *lib, uint8_t clk_pin, uint8_t data_pin);
int32_t lif_mdio_c22_read(uint8_t lif_id, uint8_t pad, uint8_t dad);
int32_t lif_mdio_c22_write(uint8_t lif_id, uint8_t pad, uint8_t dad, uint16_t val);
int32_t lif_mdio_c45_read(uint8_t lif_id, uint8_t pad, uint8_t dad, uint16_t reg);
int32_t lif_mdio_c45_write(uint8_t lif_id, uint8_t pad, uint8_t dad, uint16_t reg, uint16_t val);
int32_t lif_scan(char *lib);
int32_t lif_full_scan(char *lib);  /* always does full 272-pair sweep, regardless of LIF_NO_SCAN */
int32_t lif_set_pins(uint8_t clk_pin, uint8_t data_pin);
int32_t lif_scan_with_pins(char *lib, uint8_t clk_pin, uint8_t data_pin);
int32_t lif_open_forced(char *lib, uint8_t clk_pin, uint8_t data_pin);
int32_t lif_get_cpin(uint8_t lif_id);
int32_t lif_get_dpin(uint8_t lif_id);
int32_t lif_get_nr_phys(uint8_t lif_id);
int32_t lif_get_phy_addr(uint8_t lif_id, uint8_t phy);
int32_t lif_get_phy_id(uint8_t lif_id, uint8_t phy);
int32_t lif_get_nr_lif(void);
/* Returns 1 if the device on lif_id is an MxL862xx switch, 0 otherwise.
   Two detection methods are tried; either passing is sufficient:
   (1) C22 register-address echo: reading reg N at the SMDIO PHY address
       returns N (e.g. reg2→2, reg3→3). No standard PHY behaves this way.
   (2) SMDIO indirect register probe: MxL862xx-specific registers 0xf383
       and 0x0384 return stable non-trivial values; standard PHYs return
       0x0000 or 0xffff for these addresses.
   Call only after a successful lif_scan() / lif_full_scan(). */
int32_t lif_is_mxl862xx(uint8_t lif_id);

/*
return codes
*/

#define LIF_API_RET_SUCCESS          0      /* No error               : Successful operation */
#define LIF_API_RET_LIB_ERROR       -1      /* Library error          : Invalid library operation: wrong library selection, library not or already initialized. */
#define LIF_API_RET_PINS_ERROR      -2      /* Pins validity error    : Pins not allowed or (clk , data) pins are identical */
#define LIF_API_RET_OPEN_ERROR      -3      /* Pins open error        : Invalid Pins open operation: open operation already done. */
#define LIF_API_RET_CLOSE_ERROR     -4      /* Pins Close error       : Invalid Pins close operation: close operation already done or open operation missing.	 */
#define LIF_API_RET_ACCESS_ERROR    -5      /* Pins RW Access error    : Pins are not available. */
#define LIF_API_RET_LCOMM_ERROR     -6      /* Link communication Error:Mdio communication error, cannot be determined, will never occurs */
#define LIF_API_RET_FAPI_PRM_ERROR  -7      /* Parameter Error         : FAPI Parameter error  */

/*
RPI Information
*/

#define PHY_MAX_VAL 32
/* COMB_MAX_VAL: total ordered (clk,data) pin permutations for the full scan.
   Derived from RASP_AVAILABLE_PINS so it stays correct if the pin list changes. */
#define COMB_MAX_VAL (RASP_AVAILABLE_PINS * (RASP_AVAILABLE_PINS - 1))

/* MAX_SCAN_LINKS: size of the global scanned_devices[] table and the upper
   bound for all lif_id bounds checks.  Always 5 regardless of LIF_NO_SCAN
   so that lif_full_scan() can discover up to 5 simultaneous MDIO buses even
   when LIF_NO_SCAN limits MAX_LINKS to 1 for normal operation. */
#define MAX_SCAN_LINKS 5

/* ==========================================================================
 * Scan Mode Selection: LIF_NO_SCAN
 * ==========================================================================
 *
 * Two operating modes are available, selected by defining or undefining
 * LIF_NO_SCAN below:
 *
 * -------------------------------------------------------------------------
 * MODE A — Fixed pins  (define LIF_NO_SCAN)
 * -------------------------------------------------------------------------
 *   How to enable:  remove the '#undef LIF_NO_SCAN' line below, or add
 *                   -DLIF_NO_SCAN to the compiler flags in CMakeLists.txt.
 *
 *   Behaviour:
 *     - lif_scan() uses exactly one (CLK, DATA) pin pair.
 *     - Compile-time defaults: LIF_CLK_PIN and LIF_DATA_PIN below.
 *     - Runtime override   : call lif_set_pins(clk, data) before lif_scan(),
 *                            or use the CLI command:
 *                              ./fapi-lif-set-pins clk=<gpio> data=<gpio>
 *                            Pass clk=0 data=0 to revert to compile defaults.
 *     - Use this mode when the wiring is known and startup time matters.
 *       MAX_LINKS is 1 (only one MDIO bus).
 *
 * -------------------------------------------------------------------------
 * MODE B — Auto-scan  (#undef LIF_NO_SCAN)  [DEFAULT]
 * -------------------------------------------------------------------------
 *   How to enable:  keep the '#undef LIF_NO_SCAN' line below (default), or
 *                   ensure -DLIF_NO_SCAN is NOT in the compiler flags.
 *
 *   Behaviour:
 *     - lif_scan() tries all ordered (CLK, DATA) permutations from the
 *       RASP_AVAILABLE_PINS list and probes each for MDIO devices.
 *     - lif_set_pins() / fapi-lif-set-pins have no effect in this mode.
 *     - Use this mode when the GPIO wiring is unknown or for initial bring-up.
 *       Slower startup (up to COMB_MAX_VAL * 32 MDIO reads).
 *       MAX_LINKS is 5 (up to 5 simultaneous MDIO buses).
 *
 * -------------------------------------------------------------------------
 * Quick-change guide
 * -------------------------------------------------------------------------
 *   1. Change to fixed-pin mode:
 *        - Remove or comment out '#undef LIF_NO_SCAN' below.
 *        - Optionally update LIF_CLK_PIN / LIF_DATA_PIN for your wiring.
 *        - Rebuild: . ethswbox-prepare.sh rpi4evk && . ethswbox-set-build.sh
 *
 *   2. Change default pins without rebuilding (fixed-pin mode only):
 *        ./fapi-lif-set-pins clk=<gpio> data=<gpio>
 *        Then run any fapi-GSW-* command as usual.
 *
 *   3. Switch back to auto-scan:
 *        - Restore '#undef LIF_NO_SCAN' below and rebuild.
 * ==========================================================================
 */
/* #define LIF_NO_SCAN */
/* Uncomment the line below to switch to auto-scan MODE B (tries all 272 GPIO
   pin combinations — useful for initial bring-up but adds ~5-50 ms per command). */
#undef LIF_NO_SCAN

#ifdef LIF_NO_SCAN
#define MAX_LINKS 1
#define RASP_AVAILABLE_PINS 2

/* Compile-time default GPIO pins for fixed-pin mode (MODE A).
 * Change these values to match your hardware wiring, then rebuild.
 * Alternatively, override at runtime (no rebuild needed) with:
 *   lif_set_pins(clk_pin, data_pin)  -- from C code
 *   ./fapi-lif-set-pins clk=X data=Y -- from CLI
 */
#define LIF_CLK_PIN  5   /* BCM GPIO used as MDIO clock */
#define LIF_DATA_PIN 6   /* BCM GPIO used as MDIO data  */

#else  /* auto-scan mode (MODE B) */
#define MAX_LINKS 5
/* RPi4 40-pin header GPIO pins usable for bit-bang MDIO:
 *   Group A  (4-6)   : GPIO 4, 5, 6
 *   Group B  (12-13) : GPIO 12 (PWM0), 13 (PWM1)
 *   Group C  (16-21) : GPIO 16, 17, 18, 19, 20, 21  <-- extended
 *   Group D  (22-27) : GPIO 22, 23, 24, 25, 26, 27
 *
 * Excluded (dedicated functions):
 *   GPIO 0-1  : ID EEPROM
 *   GPIO 2-3  : I2C1 SDA/SCL
 *   GPIO 7-11 : SPI0 (CE1,CE0,MISO,MOSI,CLK)
 *   GPIO 14-15: UART0 TXD/RXD
 */
#define RASP_AVAILABLE_PINS 17
#endif

#endif /* LIF_API_H */
