/******************************************************************************

   Copyright 2023-2024 MaxLinear, Inc.

   For licensing information, see the file 'LICENSE' in the root folder of
   this software module.

******************************************************************************/

#include "host_adapt.h"

#include <lif_api.h>
#include <unistd.h>

uint8_t lif_id = 0;
static GSW_Device_t gsw_dev;

/* =========================================================================
 * GPIO pin scan result cache
 *
 * Each fapi-* command is a separate process, so the auto-scan result is
 * persisted to a file in the current working directory (package-local)
 * so that all commands share one cache file regardless of the working directory.
 *
 * Cache lifetime: persistent, cleared only by 'fapi-lif-scan'.
 * Format: "CLK=<n> DATA=<n>\n"
 * ========================================================================= */
#define LIF_CACHE_FILENAME "ethswbox_lif_cache"

const char *get_lif_cache_path(void)
{
	static char path[32] = {0};
	if (path[0] == '\0')
		snprintf(path, sizeof(path), "./%s", LIF_CACHE_FILENAME);
	return path;
}

static int read_lif_cache(uint8_t *clk, uint8_t *data)
{
	FILE *f = fopen(get_lif_cache_path(), "r");
	if (!f)
		return 0;
	int c = 0, d = 0;
	int n = fscanf(f, "CLK=%d DATA=%d", &c, &d);
	fclose(f);
	if (n == 2 && c > 1 && d > 1 && c != d && c < 32 && d < 32) {
		*clk  = (uint8_t)c;
		*data = (uint8_t)d;
		return 1;
	}
	return 0;
}

static void write_lif_cache(uint8_t clk, uint8_t data)
{
	const char *path = get_lif_cache_path();
	FILE *f = fopen(path, "w");
	if (!f) {
		printf("api_gsw_get_links: cannot write cache %s\n", path);
		return;
	}
	fprintf(f, "CLK=%d DATA=%d\n", (int)clk, (int)data);
	fclose(f);
}

/* Public wrappers used by fapi-lif-set-pins to update / clear the cache
   without running a full GPIO scan. */
void write_lif_cache_pins(uint8_t clk, uint8_t data)
{
	write_lif_cache(clk, data);
}

void clear_lif_cache(void)
{
	remove(get_lif_cache_path());
}

static void __usleep(unsigned long usec)
{
	/* TO be replaced with OS dependent implementation */
	usleep(usec);
}

static pthread_mutex_t lock;

static void __lock(void *lock_data)
{
	pthread_mutex_lock(lock_data);
}

static void __unlock(void *lock_data)
{
	pthread_mutex_unlock(lock_data);
}


typedef struct
{
    /** mdio clock pin with the following values
        - (0) pin not valid
        - (1) valid pin close
        - (pin) valid open pin as clock pin */
   uint8_t cpin;

    /** mdio data pin with the following values
        - (0) pin not valid
        - (1) valid pin closed
        - (pin) valid open pin as data pin */
   uint8_t dpin;

   /** Number of Phys Found */
   uint8_t nr_phys;

   /** Phy information in structure [Address, PHYID] */
   uint32_t phy_info[PHY_MAX_VAL][2];


   /** Phy information in structure [Address, PHYID] */
   // void* gsw_struct;

   GSW_Device_t gsw_struct[PHY_MAX_VAL];

} link_struc;
link_struc scanned_links[MAX_LINKS];

struct phy_if  {
   uint8_t lif_id;
	  
};
static struct phy_if phyif;

static int mdiobus_read(void *mdiobus_data, uint8_t phyaddr, uint8_t mmd,
			uint16_t reg)
{
	int ret;

	(void)mdiobus_data;

	if (phyaddr > 31 || reg > GSW_MMD_REG_DATA_LAST)
		return -EINVAL;

	if (mmd == GSW_MMD_DEV)
		ret = lif_mdio_c45_read(phyif.lif_id, phyaddr, mmd, reg);
	else if (mmd == GSW_MMD_SMDIO_DEV)
		ret = lif_mdio_c22_read(phyif.lif_id, phyaddr, reg);
	else
		return -EINVAL;

	return ret;
}

static int mdiobus_write(void *mdiobus_data, uint8_t phyaddr, uint8_t mmd,
			 uint16_t reg, uint16_t val)
{
	(void)mdiobus_data;

	if (phyaddr > 31 || reg > GSW_MMD_REG_DATA_LAST)
		return -EINVAL;

	if (mmd == GSW_MMD_DEV)
		return lif_mdio_c45_write(phyif.lif_id, phyaddr, mmd, reg, val);
	else if (mmd == GSW_MMD_SMDIO_DEV)
		return lif_mdio_c22_write(phyif.lif_id, phyaddr, reg, val);
	else
		return -EINVAL;
}

/* TO be adapted  with target dependent implementation */
GSW_Device_t* gsw_adapt_init(uint8_t lif_id, uint8_t phy_id)
{
	memset(&gsw_dev, 0, sizeof(gsw_dev));

	gsw_dev.usleep = __usleep;
	gsw_dev.lock = __lock;
	gsw_dev.unlock = __unlock;
	gsw_dev.lock_data = &lock;
	gsw_dev.mdiobus_read = mdiobus_read;
	gsw_dev.mdiobus_write = mdiobus_write;
	gsw_dev.mdiobus_data = NULL;

	gsw_dev.phy_addr = (uint8_t)gsw_get_phy_addr(lif_id, phy_id);
	gsw_dev.smdio_phy_addr = gsw_dev.phy_addr;

	return &gsw_dev;
}

/**
   Implements the MDIO library (PyRPIO only) Scan procedure.


   \param
    char*    lib,       GPIO Interface library name

   \return
    nr_lif number of links found.
*/
int32_t api_gsw_get_links(char* lib)
{
	int32_t nr_lif = 0;
	uint8_t lif_id, phy, max_phy;
	uint8_t cached_clk = 0, cached_data = 0;

	lif_mdio_init(lib);

	if (read_lif_cache(&cached_clk, &cached_data)) {
		/* Fast path: use cached pins — skip the full 272-pair GPIO scan */
		nr_lif = lif_scan_with_pins(lib, cached_clk, cached_data);
		if (nr_lif == 0) {
			/* Device not detected by fingerprint scan (non-standard SMDIO
			   device with no MDIO PHY-ID registers).  Open the bus directly
			   so raw lif_mdio_c22_read / smdio_read calls still work. */
			nr_lif = lif_open_forced(lib, cached_clk, cached_data);
		}
	} else {
		/* Slow path: full auto-scan, then cache result for next time */
		nr_lif = lif_scan(lib);
		if (nr_lif > 0) {
			write_lif_cache((uint8_t)lif_get_cpin(0),
					(uint8_t)lif_get_dpin(0));
			printf("api_gsw_get_links: cached CLK=%d DATA=%d to %s\n",
				lif_get_cpin(0), lif_get_dpin(0), get_lif_cache_path());
		}
	}

	if (nr_lif == 0) {
		printf("api_gsw_get_links: no MDIO link found."
			" Verify GPIO wiring and pin configuration.\n");
		return 0;
	}

	for (lif_id = 0; lif_id < nr_lif; lif_id++){
		max_phy = (uint8_t)lif_get_nr_phys(lif_id);
		scanned_links[lif_id].cpin = (uint8_t)lif_get_cpin(lif_id);
		scanned_links[lif_id].dpin = (uint8_t)lif_get_dpin(lif_id);
		scanned_links[lif_id].nr_phys = max_phy;


		for (phy = 0; phy < max_phy; phy++){
				scanned_links[lif_id].phy_info[phy][0]=lif_get_phy_addr(lif_id,phy);
				scanned_links[lif_id].phy_info[phy][1]=lif_get_phy_id(lif_id,phy);
				scanned_links[lif_id].gsw_struct[phy] = *(GSW_Device_t *)gsw_adapt_init(lif_id,phy);
		}
	}
	return nr_lif;
}

int32_t gsw_get_cpin (uint8_t lif_id)
{
	return  scanned_links[lif_id].cpin;
}

/**
   Return Lif Data pin.

   \param

   uint8_t lif_id,       Lif_Id index


   \return
      int32_t data pin
*/
int32_t gsw_get_dpin(uint8_t lif_id)
{
	return  scanned_links[lif_id].dpin;
}
int32_t gsw_update_lif_id(uint8_t lif_id)
{
	phyif.lif_id  = lif_id;  
	return 0;
}

/**
   Return Number of Phys per Lif.

   \param

   uint8_t lif_id,       Lif_Id index


   \return
      int32_t  Number of Phys
*/
int32_t gsw_get_nr_phys (uint8_t lif_id)
{
	return  scanned_links[lif_id].nr_phys;
}

/**
   Return PHY ADDR based on LIF and Phy Index.
   \param

   uint8_t lif_id,      Lif_Id index
   uint8_t phy,      Phy Index

   \return
      int32_t   PHY ADDR
*/

int32_t gsw_get_phy_addr(uint8_t lif_id, uint8_t phy)
{
	return  scanned_links[lif_id].phy_info[phy][0];
}

/**
   Return PHY ID based on LIF and Phy Index.
   \param

   uint8_t lif_id,       Lif_Id index
   uint8_t phy,      Phy Index

   \return
      int32_t   PHY ID
*/
int32_t gsw_get_phy_id(uint8_t lif_id, uint8_t phy)
{
	return  scanned_links[lif_id].phy_info[phy][1];
}

/**
   Return gsw structure pointer based on LIf.
   \param

   uint8_t lif_id,       Lif_Id index
   uint8_t phy_id,       phy_id index


   \return
      void*   gsw_211 struc
*/

GSW_Device_t* gsw_get_struc(uint8_t lif_id,uint8_t phy_id)
{
	gsw_update_lif_id(lif_id);
	return &scanned_links[lif_id].gsw_struct[phy_id];
}

int gsw_read(const GSW_Device_t *dev, uint32_t regaddr)
{
	if (!dev || !dev->mdiobus_read) {
		printf("gsw_read: device not initialised (NULL function pointer).\n"
			"Run lif_scan first and verify GPIO pin configuration.\n");
		return -EINVAL;
	}
	return dev->mdiobus_read(dev->mdiobus_data, dev->phy_addr, GSW_MMD_DEV,
				 regaddr);
}

int gsw_write(const GSW_Device_t *dev, uint32_t regaddr, uint16_t data)
{
	if (!dev || !dev->mdiobus_write) {
		printf("gsw_write: device not initialised (NULL function pointer).\n"
			"Run lif_scan first and verify GPIO pin configuration.\n");
		return -EINVAL;
	}
	return dev->mdiobus_write(dev->mdiobus_data, dev->phy_addr, GSW_MMD_DEV,
				  regaddr, data);
}
