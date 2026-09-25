/******************************************************************************

    Copyright 2022 Maxlinear

    SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0-only)

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.
******************************************************************************/

#include <os_types.h>
#include <os_linux.h>
#include <stdlib.h>
#include <unistd.h>

#include "gsw_device.h"
#include "host_adapt.h"
#include "host_smdio_ssb.h"
#include "mdio_relay.h"
#include "gsw_cli_common.h"

#ifdef ETHSWBOX_RPI4EVK
#include "lif_api.h"
#endif

extern uint8_t lif_id;

/* read internal GPHY MDIO/MMD registers */
int fapi_int_gphy_read(int prmc, char *prmv[])
{
    GSW_Device_t *gsw_dev;
    int ret;
    struct mdio_relay_data param = {0};
    int rret;

    rret = scanParamArg(prmc, prmv, "phy", sizeof(param.phy), &param.phy);
    if (rret < 1)
    {
        printf("parameter not Found: phy\n");
        return OS_ERROR;
    }

    rret = scanParamArg(prmc, prmv, "mmd", sizeof(param.mmd), &param.mmd);
    if (rret < 1)
    {
        printf("parameter not Found: mmd\n");
        return OS_ERROR;
    }

    rret = scanParamArg(prmc, prmv, "reg", sizeof(param.reg), &param.reg);
    if (rret < 1)
    {
        printf("parameter not Found: reg\n");
        return OS_ERROR;
    }

    gsw_dev = gsw_get_struc(lif_id, 0);
    ret = int_gphy_read(gsw_dev, &param);

    if (ret < 0)
        printf("\t%40s:\t0x%x\n", "fapi_int_gphy_read failed with ret code", ret);
    else
        printf("fapi_int_gphy_read:\tphy=0x%x mmd=0x%x reg=0x%x ret=0x%x\n", param.phy, param.mmd, param.reg, param.data);
    return 0;
}

/* write internal GPHY MDIO/MMD registers */
int fapi_int_gphy_write(int prmc, char *prmv[])
{
    GSW_Device_t *gsw_dev;
    int ret;
    struct mdio_relay_data param = {0};
    int rret;

    rret = scanParamArg(prmc, prmv, "phy", sizeof(param.phy), &param.phy);
    if (rret < 1)
    {
        printf("parameter not Found: phy\n");
        return OS_ERROR;
    }

    rret = scanParamArg(prmc, prmv, "mmd", sizeof(param.mmd), &param.mmd);
    if (rret < 1)
    {
        printf("parameter not Found: mmd\n");
        return OS_ERROR;
    }

    rret = scanParamArg(prmc, prmv, "reg", sizeof(param.reg), &param.reg);
    if (rret < 1)
    {
        printf("parameter not Found: reg\n");
        return OS_ERROR;
    }

    rret = scanParamArg(prmc, prmv, "data", sizeof(param.data), &param.data);
    if (rret < 1)
    {
        printf("parameter not Found: data\n");
        return OS_ERROR;
    }

    gsw_dev = gsw_get_struc(lif_id, 0);
    ret = int_gphy_write(gsw_dev, &param);

    if (ret < 0)
        printf("\t%40s:\t0x%x\n", "fapi_int_gphy_write failed with ret code", ret);
    else
        printf("fapi_int_gphy_write:\tphy=0x%x mmd=0x%x reg=0x%x data=0x%x\n", param.reg, param.mmd, param.reg, param.data);
    return 0;
}

// /* modify internal GPHY MDIO/MMD registers */
int fapi_int_gphy_mod(int prmc, char *prmv[])
{
    GSW_Device_t *gsw_dev;
    int ret;
    struct mdio_relay_mod_data param = {0};
    int rret;

    rret = scanParamArg(prmc, prmv, "phy", sizeof(param.phy), &param.phy);
    if (rret < 1)
    {
        printf("parameter not Found: phy\n");
        return OS_ERROR;
    }

    rret = scanParamArg(prmc, prmv, "mmd", sizeof(param.mmd), &param.mmd);
    if (rret < 1)
    {
        printf("parameter not Found: mmd\n");
        return OS_ERROR;
    }

    rret = scanParamArg(prmc, prmv, "reg", sizeof(param.reg), &param.reg);
    if (rret < 1)
    {
        printf("parameter not Found: reg\n");
        return OS_ERROR;
    }

    rret = scanParamArg(prmc, prmv, "data", sizeof(param.data), &param.data);
    if (rret < 1)
    {
        printf("parameter not Found: data\n");
        return OS_ERROR;
    }

    rret = scanParamArg(prmc, prmv, "mask", sizeof(param.mask), &param.mask);
    if (rret < 1)
    {
        printf("parameter not Found: mask\n");
        return OS_ERROR;
    }
    gsw_dev = gsw_get_struc(lif_id, 0);
    ret = int_gphy_mod(gsw_dev, &param);

    if (ret < 0)
        printf("\t%40s:\t0x%x\n", "fapi_int_gphy_mod failed with ret code", ret);
    else
        printf("fapi_int_gphy_mod:\tphy=0x%x mmd=0x%x reg=0x%x data=0x%x mask=0x%x\n", param.reg, param.mmd, param.reg, param.data, param.mask);
    return 0;
}

/* read external GPHY MDIO/MMD registers via MDIO bus */
int fapi_ext_mdio_read(int prmc, char *prmv[])
{
    GSW_Device_t *gsw_dev;
    int ret;
    struct mdio_relay_data param = {0};
    int rret;

    rret = scanParamArg(prmc, prmv, "phy", sizeof(param.phy), &param.phy);
    if (rret < 1)
    {
        printf("parameter not Found: phy\n");
        return OS_ERROR;
    }

    rret = scanParamArg(prmc, prmv, "mmd", sizeof(param.mmd), &param.mmd);
    if (rret < 1)
    {
        printf("parameter not Found: mmd\n");
        return OS_ERROR;
    }

    rret = scanParamArg(prmc, prmv, "reg", sizeof(param.reg), &param.reg);
    if (rret < 1)
    {
        printf("parameter not Found: reg\n");
        return OS_ERROR;
    }

    gsw_dev = gsw_get_struc(lif_id, 0);
    ret = ext_mdio_read(gsw_dev, &param);

    if (ret < 0)
        printf("\t%40s:\t0x%x\n", "fapi_ext_mdio_read failed with ret code", ret);
    else
        printf("fapi_ext_mdio_read:\tphy=0x%x mmd=0x%x reg=0x%x ret=0x%x\n", param.reg, param.mmd, param.reg, param.data);
    return 0;
}
/* write external GPHY MDIO/MMD registers via MDIO bus */
int fapi_ext_mdio_write(int prmc, char *prmv[])
{
    GSW_Device_t *gsw_dev;
    int ret;
    struct mdio_relay_data param = {0};
    int rret;

    rret = scanParamArg(prmc, prmv, "phy", sizeof(param.phy), &param.phy);
    if (rret < 1)
    {
        printf("parameter not Found: phy\n");
        return OS_ERROR;
    }

    rret = scanParamArg(prmc, prmv, "mmd", sizeof(param.mmd), &param.mmd);
    if (rret < 1)
    {
        printf("parameter not Found: mmd\n");
        return OS_ERROR;
    }

    rret = scanParamArg(prmc, prmv, "reg", sizeof(param.reg), &param.reg);
    if (rret < 1)
    {
        printf("parameter not Found: reg\n");
        return OS_ERROR;
    }

    rret = scanParamArg(prmc, prmv, "data", sizeof(param.data), &param.data);
    if (rret < 1)
    {
        printf("parameter not Found: data\n");
        return OS_ERROR;
    }

    gsw_dev = gsw_get_struc(lif_id, 0);
    ret = ext_mdio_write(gsw_dev, &param);

    if (ret < 0)
        printf("\t%40s:\t0x%x\n", "fapi_ext_mdio_write failed with ret code", ret);
    else
        printf("fapi_ext_mdio_write:\tphy=0x%x mmd=0x%x reg=0x%x data=0x%x\n", param.reg, param.mmd, param.reg, param.data);
    return 0;
}
/* modify external GPHY MDIO/MMD registers via MDIO bus */
int fapi_ext_mdio_mod(int prmc, char *prmv[])
{
    GSW_Device_t *gsw_dev;
    int ret;
    struct mdio_relay_mod_data param = {0};
    int rret;

    rret = scanParamArg(prmc, prmv, "phy", sizeof(param.phy), &param.phy);
    if (rret < 1)
    {
        printf("parameter not Found: phy\n");
        return OS_ERROR;
    }

    rret = scanParamArg(prmc, prmv, "mmd", sizeof(param.mmd), &param.mmd);
    if (rret < 1)
    {
        printf("parameter not Found: mmd\n");
        return OS_ERROR;
    }

    rret = scanParamArg(prmc, prmv, "reg", sizeof(param.reg), &param.reg);
    if (rret < 1)
    {
        printf("parameter not Found: reg\n");
        return OS_ERROR;
    }

    rret = scanParamArg(prmc, prmv, "data", sizeof(param.data), &param.data);
    if (rret < 1)
    {
        printf("parameter not Found: data\n");
        return OS_ERROR;
    }

    rret = scanParamArg(prmc, prmv, "mask", sizeof(param.mask), &param.mask);
    if (rret < 1)
    {
        printf("parameter not Found: mask\n");
        return OS_ERROR;
    }
    gsw_dev = gsw_get_struc(lif_id, 0);
    ret = ext_mdio_mod(gsw_dev, &param);

    if (ret < 0)
        printf("\t%40s:\t0x%x\n", "fapi_ext_mdio_mod failed with ret code", ret);
    else
        printf("fapi_ext_mdio_mod:\tphy=0x%x mmd=0x%x reg=0x%x data=0x%x mask=0x%x\n", param.reg, param.mmd, param.reg, param.data, param.mask);
    return 0;
}

/* =========================================================================
 * Direct GPIO bit-bang MDIO access (bypasses the switch host API layer)
 * Uses lif_mdio_c22 / lif_mdio_c45 / smdio functions directly.
 * lif_id 0 is always used (first scanned link).
 * ========================================================================= */

/* fapi_c22_mdio_read: Clause-22 register read
   Parameters: phy=<addr>  reg=<offset>
   Output    : phy, reg, data */
int fapi_c22_mdio_read(int prmc, char *prmv[])
{
    GSW_Device_t *gsw_dev;
    uint16_t phy = 0;
    uint16_t reg = 0;
    int ret;

    if (scanParamArg(prmc, prmv, "phy", sizeof(phy), &phy) < 1 ||
        scanParamArg(prmc, prmv, "reg", sizeof(reg), &reg) < 1)
    {
        printf("Usage: fapi-c22-mdio-read phy=<addr> reg=<offset>\n");
        return OS_ERROR;
    }

    gsw_dev = gsw_get_struc(lif_id, 0);
    ret = gsw_dev->mdiobus_read(gsw_dev->mdiobus_data, (uint8_t)phy, GSW_MMD_SMDIO_DEV, (uint16_t)reg);
    if (ret < 0)
        printf("fapi-c22-mdio-read failed: ret=%d\n", ret);
    else
        printf("fapi-c22-mdio-read: phy=0x%02x reg=0x%04x data=0x%04x\n",
               (unsigned)phy, (unsigned)reg, (unsigned)ret);
    return 0;
}

/* fapi_c22_mdio_write: Clause-22 register write
   Parameters: phy=<addr>  reg=<offset>  data=<value> */
int fapi_c22_mdio_write(int prmc, char *prmv[])
{
    GSW_Device_t *gsw_dev;
    uint16_t phy  = 0;
    uint16_t reg  = 0;
    uint16_t data = 0;
    int ret;

    if (scanParamArg(prmc, prmv, "phy",  sizeof(phy),  &phy)  < 1 ||
        scanParamArg(prmc, prmv, "reg",  sizeof(reg),  &reg)  < 1 ||
        scanParamArg(prmc, prmv, "data", sizeof(data), &data) < 1)
    {
        printf("Usage: fapi-c22-mdio-write phy=<addr> reg=<offset> data=<value>\n");
        return OS_ERROR;
    }

    gsw_dev = gsw_get_struc(lif_id, 0);
    ret = gsw_dev->mdiobus_write(gsw_dev->mdiobus_data, (uint8_t)phy, GSW_MMD_SMDIO_DEV, (uint16_t)reg, data);
    if (ret < 0)
        printf("fapi-c22-mdio-write failed: ret=%d\n", ret);
    else
        printf("fapi-c22-mdio-write: phy=0x%02x reg=0x%04x data=0x%04x  OK\n",
               (unsigned)phy, (unsigned)reg, (unsigned)data);
    return 0;
}

/* fapi_c45_mdio_read: Clause-45 register read
   Parameters: phy=<addr>  mmd=<devad>  reg=<offset>
   Output    : phy, mmd, reg, data */
int fapi_c45_mdio_read(int prmc, char *prmv[])
{
    GSW_Device_t *gsw_dev;
    uint16_t phy = 0;
    uint16_t mmd = 0;
    uint16_t reg = 0;
    int ret;

    if (scanParamArg(prmc, prmv, "phy", sizeof(phy), &phy) < 1 ||
        scanParamArg(prmc, prmv, "mmd", sizeof(mmd), &mmd) < 1 ||
        scanParamArg(prmc, prmv, "reg", sizeof(reg), &reg) < 1)
    {
        printf("Usage: fapi-c45-mdio-read phy=<addr> mmd=<devad> reg=<offset>\n");
        return OS_ERROR;
    }

    gsw_dev = gsw_get_struc(lif_id, 0);
    ret = gsw_dev->mdiobus_read(gsw_dev->mdiobus_data, (uint8_t)phy, (uint8_t)mmd, (uint16_t)reg);
    if (ret < 0)
        printf("fapi-c45-mdio-read failed: ret=%d\n", ret);
    else
        printf("fapi-c45-mdio-read: phy=0x%02x mmd=0x%02x reg=0x%04x data=0x%04x\n",
               (unsigned)phy, (unsigned)mmd, (unsigned)reg, (unsigned)ret);
    return 0;
}

/* fapi_c45_mdio_write: Clause-45 register write
   Parameters: phy=<addr>  mmd=<devad>  reg=<offset>  data=<value> */
int fapi_c45_mdio_write(int prmc, char *prmv[])
{
    GSW_Device_t *gsw_dev;
    uint16_t phy  = 0;
    uint16_t mmd  = 0;
    uint16_t reg  = 0;
    uint16_t data = 0;
    int ret;

    if (scanParamArg(prmc, prmv, "phy",  sizeof(phy),  &phy)  < 1 ||
        scanParamArg(prmc, prmv, "mmd",  sizeof(mmd),  &mmd)  < 1 ||
        scanParamArg(prmc, prmv, "reg",  sizeof(reg),  &reg)  < 1 ||
        scanParamArg(prmc, prmv, "data", sizeof(data), &data) < 1)
    {
        printf("Usage: fapi-c45-mdio-write phy=<addr> mmd=<devad> reg=<offset> data=<value>\n");
        return OS_ERROR;
    }

    gsw_dev = gsw_get_struc(lif_id, 0);
    ret = gsw_dev->mdiobus_write(gsw_dev->mdiobus_data, (uint8_t)phy, (uint8_t)mmd, (uint16_t)reg, data);
    if (ret < 0)
        printf("fapi-c45-mdio-write failed: ret=%d\n", ret);
    else
        printf("fapi-c45-mdio-write: phy=0x%02x mmd=0x%02x reg=0x%04x data=0x%04x  OK\n",
               (unsigned)phy, (unsigned)mmd, (unsigned)reg, (unsigned)data);
    return 0;
}

/* fapi_smdio_read: SMDIO (Serial MDIO / SSB) register read
   Uses 2-phase C22 sequence: write address to reg 0x1F, read data from reg 0x0
   Parameters: reg=<offset>
   Output    : reg, data */
int fapi_smdio_read(int prmc, char *prmv[])
{
    uint16_t reg = 0;
    int ret;
    struct host_smdio_ssb_ops *mdio_ops;

    if (scanParamArg(prmc, prmv, "reg", sizeof(reg), &reg) < 1)
    {
        printf("Usage: fapi-smdio-read reg=<offset>\n");
        printf("  reg : 16-bit register offset inside the SMDIO device\n");
        return OS_ERROR;
    }

    mdio_ops = host_smdio_ssb_ops_init();

    printf("smdio addr = 0x%x\n", mdio_ops->pdev->smdio_phy_addr);
    ret = mdio_ops->smdio_read(mdio_ops->pdev, reg);
    if (ret < 0)
        printf("fapi-smdio-read failed: ret=%d\n", ret);
    else
        printf("fapi-smdio-read: reg=0x%04x data=0x%04x\n",
               (unsigned)reg, (unsigned)(uint16_t)ret);
    return 0;
}

/* fapi_smdio_write: SMDIO (Serial MDIO / SSB) register write
   Uses 2-phase C22 sequence: write address to reg 0x1F, write data to reg 0x0
   Parameters: reg=<offset>  data=<value> */
int fapi_smdio_write(int prmc, char *prmv[])
{
    uint16_t reg  = 0;
    uint16_t data = 0;
    int ret;
    struct host_smdio_ssb_ops *mdio_ops;

    if (scanParamArg(prmc, prmv, "reg",  sizeof(reg),  &reg)  < 1 ||
        scanParamArg(prmc, prmv, "data", sizeof(data), &data) < 1)
    {
        printf("Usage: fapi-smdio-write reg=<offset> data=<value>\n");
        printf("  reg : 16-bit register offset inside the SMDIO device\n");
        printf("  data: 16-bit value to write\n");
        return OS_ERROR;
    }

    mdio_ops = host_smdio_ssb_ops_init();

    printf("smdio addr = 0x%x\n", mdio_ops->pdev->smdio_phy_addr);
    ret = mdio_ops->smdio_write(mdio_ops->pdev, reg, data);
    if (ret < 0)
        printf("fapi-smdio-write failed: ret=%d\n", ret);
    else
        printf("fapi-smdio-write: reg=0x%04x data=0x%04x  OK\n",
               (unsigned)reg, (unsigned)data);
    return 0;
}

int fapi_rescue_enhwuart(int prmc, char *prmv[])
{
    (void)prmc;
    (void)prmv;
    int ret;
    struct host_smdio_ssb_ops *mdio_ops;

    mdio_ops = host_smdio_ssb_ops_init();

    printf("smdio addr = 0x%x\n", mdio_ops->pdev->smdio_phy_addr);
    ret = mdio_ops->smdio_write(mdio_ops->pdev, 0xf383, 0x2);
    printf("fapi-rescue-enhwuart: reg=0xf383 data=0x2 ret=%d\n", ret);
    ret |= mdio_ops->smdio_write(mdio_ops->pdev, 0xf384, 0xe);
    printf("fapi-rescue-enhwuart: reg=0xf384 data=0xe ret=%d\n", ret);
    ret |= mdio_ops->smdio_write(mdio_ops->pdev, 0xf382, 0xe095);
    printf("fapi-rescue-enhwuart: reg=0xf382 data=0xe095 ret=%d\n", ret);

    return 0;
}

#ifdef ETHSWBOX_RPI4EVK
/* fapi_lif_scan: force a new full GPIO auto-scan and refresh the pin cache.
 *
 * Deletes /tmp/ethswbox_lif_cache so that api_gsw_get_links() runs the slow
 * full-combination scan instead of using the cached (CLK, DATA) pair.  The
 * result is written back to the cache so subsequent commands are fast again.
 *
 * Use this command when:
 *   - Wiring has changed
 *   - Cache is stale (fapi commands fail to find the device)
 *   - After first boot before the cache file exists */
int fapi_lif_scan(int prmc, char *prmv[])
{
    (void)prmc;
    (void)prmv;
    int32_t nr;

    printf("fapi-lif-scan: clearing cache and running full GPIO pin scan...\n");
    remove(get_lif_cache_path());

    /* Always use lif_full_scan() so discovery works even in LIF_NO_SCAN
       (fixed-pin) mode where lif_scan() only tries the compile-time defaults) */
    lif_mdio_init("bcm2835");
    nr = lif_full_scan("bcm2835");
    if (nr == 0) {
        printf("fapi-lif-scan: no MDIO device found.\n"
               "  Check GPIO wiring and ensure the EVK is powered.\n");
        return -1;
    }

    /* Identify which found link is the MxL862xx switch.
     * The MxL862xx SMDIO bus echoes the PHY address value back on any C22
     * read: c22_read(phy=N, reg=*) == N.  Use this to pick the right link
     * when multiple MDIO buses are connected on different GPIO pairs.
     * Fall back to link 0 if the MxL862xx cannot be positively identified. */
    {
        int32_t mxl_lid = -1;
        int32_t lid;
        for (lid = 0; lid < nr; lid++) {
            if (lif_is_mxl862xx((uint8_t)lid)) {
                mxl_lid = lid;
                break;
            }
        }
        if (mxl_lid < 0) {
            printf("fapi-lif-scan: MxL862xx not identified, caching link 0.\n");
            mxl_lid = 0;
        }

        /* Write the identified MxL862xx GPIO pins to cache */
        {
            const char *path = get_lif_cache_path();
            FILE *f = fopen(path, "w");
            if (f) {
                fprintf(f, "CLK=%d DATA=%d\n",
                        lif_get_cpin((uint8_t)mxl_lid),
                        lif_get_dpin((uint8_t)mxl_lid));
                fclose(f);
            }
        }

        printf("fapi-lif-scan: found %d link(s)\n", (int)nr);
        {
            int32_t p;
            for (lid = 0; lid < nr; lid++) {
                int32_t np = lif_get_nr_phys((uint8_t)lid);
                printf("  [%d] CLK=%-2d DATA=%-2d",
                       lid,
                       lif_get_cpin((uint8_t)lid),
                       lif_get_dpin((uint8_t)lid));
                for (p = 0; p < np; p++)
                    printf("  addr=0x%02x",
                           lif_get_phy_addr((uint8_t)lid, (uint8_t)p));
                if (mxl_lid >= 0 && lid == mxl_lid)
                    printf("  [cached]");
                printf("\n");
            }
        }
    }
    return 0;
}

int fapi_lif_pin_set(int prmc, char *prmv[])
{
    uint16_t clk_pin  = 0;
    uint16_t data_pin = 0;
    int ret;

    if (scanParamArg(prmc, prmv, "clk",  sizeof(clk_pin), &clk_pin)  < 1 ||
        scanParamArg(prmc, prmv, "data", sizeof(data_pin), &data_pin) < 1)
    {
        printf("Usage: fapi-lif-set-pins clk=<gpio> data=<gpio>\n");
        printf("  clk  : BCM GPIO number for MDIO clock  (e.g. 5)\n");
        printf("  data : BCM GPIO number for MDIO data   (e.g. 6)\n");
        printf("  Pass clk=0 data=0 to clear override and revert to defaults.\n");
        printf("  Valid GPIO pins: 4,5,6,12,13,16,17,18,19,20,21,22,23,24,25,26,27\n");
        return OS_ERROR;
    }

    ret = lif_set_pins((uint8_t)clk_pin, (uint8_t)data_pin);
    if (ret == LIF_API_RET_SUCCESS)
    {
        if (clk_pin == 0 && data_pin == 0) {
            clear_lif_cache();
            printf("fapi-lif-set-pins: override cleared, cache removed.\n"
                    "  Next command will use default/scanned pins.\n");
        } else {
            write_lif_cache_pins((uint8_t)clk_pin, (uint8_t)data_pin);
            printf("fapi-lif-set-pins: CLK=%u DATA=%u written to cache.\n"
                    "  Subsequent MDIO commands will use these pins immediately.\n"
                    "  Run fapi-lif-scan to rediscover all connected devices.\n",
                    (unsigned)clk_pin, (unsigned)data_pin);
        }
    }
    else
        printf("fapi-lif-set-pins: failed (ret=%d) — check pin numbers.\n", ret);
    return 0;
}
#endif /* ETHSWBOX_RPI4EVK*/
