/******************************************************************************

   Copyright 2023-2024 MaxLinear, Inc.

   For licensing information, see the file 'LICENSE' in the root folder of
   this software module.

******************************************************************************/

#include "host_adapt.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/mii.h>
#include <linux/sockios.h>

#ifndef MII_ADDR_C45
#define MII_ADDR_C45		(1 << 30)
#endif
#ifndef MII_DEVADDR_C45_SHIFT
#define MII_DEVADDR_C45_SHIFT	16
#endif

#define SMDIO_ADDR	0x10
#define ETH_DEVNAME	"eth0"

#define MTK_PHYIAC_PHY_ACS_ST 				0x8000
#define MTK_PHYIAC_MDIO_PHY_ADDR_SHFT		5

#define RAETH_MII_READ							0x89F3
#define RAETH_MII_WRITE							0x89F4
#define RAETH_MII_READ_CL45					0x89FC
#define RAETH_MII_WRITE_CL45					0x89FD

struct ra_mii_ioctl_data {
	uint16_t phy_id;
	uint16_t reg_num;
	uint32_t val_in;
	uint32_t val_out;
};

static GSW_Device_t gsw_dev = {0};
static int hapi_fd = -1;	/* AF_INET socket for ioctl() calls. */
uint8_t lif_id = 0;

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

static int user_socket_init(void)
{
	hapi_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (hapi_fd < 0) {
		perror("Socket connection failed.");
		return -EINVAL;
	}

	return 0;
}

/*
static void user_socket_fin(void)
{
	if (hapi_fd >= 0) {
		close(hapi_fd);
		hapi_fd = -1;
	}
}
*/

static int mdiobus_read(void *mdiobus_data, uint8_t phyaddr, uint8_t mmd,
			uint16_t reg)
{
	uint16_t val=0;
	int ret;

	if (phyaddr > 31 || reg > GSW_MMD_REG_DATA_LAST)
		return -EINVAL;

	if (mmd == GSW_MMD_DEV)
	{
#ifdef ENABLE_CL22_EXTENSION
		ret = CL22_MDIO_WRITE(mdiobus_data, phyaddr, CL22_STD_MMDCTRL, (mmd & 0x3fff));
		ret |= CL22_MDIO_WRITE(mdiobus_data, phyaddr, CL22_STD_MMDDATA, reg);
		ret |= CL22_MDIO_WRITE(mdiobus_data, phyaddr, CL22_STD_MMDCTRL, ((mmd & 0x3fff) | (1 << 15)));
		if (!ret)
			ret = CL22_MDIO_READ(mdiobus_data, phyaddr, CL22_STD_MMDDATA, &val);
#else
		ret = CL45_MDIO_READ(mdiobus_data, phyaddr, mmd, reg, &val);
#endif
	}
	else if (mmd == GSW_MMD_SMDIO_DEV)
		ret = CL22_MDIO_READ(mdiobus_data, phyaddr, reg, &val);
	else
		ret = -EINVAL;

	return ret < 0 ? ret : (int)val;
}

static int mdiobus_write(void *mdiobus_data, uint8_t phyaddr, uint8_t mmd,
			 uint16_t reg, uint16_t val)
{
	int ret;

	if (phyaddr > 31 || reg > GSW_MMD_REG_DATA_LAST)
		return -EINVAL;

	if (mmd == GSW_MMD_DEV)
	{
#ifdef ENABLE_CL22_EXTENSION
		ret = CL22_MDIO_WRITE(mdiobus_data, phyaddr, CL22_STD_MMDCTRL, (mmd & 0x3fff));
		ret |= CL22_MDIO_WRITE(mdiobus_data, phyaddr, CL22_STD_MMDDATA, reg);
		ret |= CL22_MDIO_WRITE(mdiobus_data, phyaddr, CL22_STD_MMDCTRL, ((mmd & 0x3fff) | (1 << 14)));
		ret |= CL22_MDIO_WRITE(mdiobus_data, phyaddr, CL22_STD_MMDDATA, val);
#else
		ret = CL45_MDIO_WRITE(mdiobus_data, phyaddr, mmd, reg, val);
#endif
	}
	else if (mmd == GSW_MMD_SMDIO_DEV)
		ret = CL22_MDIO_WRITE(mdiobus_data, phyaddr, reg, val);
	else
		ret = -EINVAL;

	return ret;
}

/* TO be adapted  with target dependent implementation */
int gsw_adapt_init(void)
{
	gsw_dev.usleep = __usleep;

	gsw_dev.lock = __lock;
	gsw_dev.unlock = __unlock;
	gsw_dev.lock_data = &lock;

	gsw_dev.mdiobus_read = mdiobus_read;
	gsw_dev.mdiobus_write = mdiobus_write;
	gsw_dev.mdiobus_data = NULL;

	gsw_dev.phy_addr = SMDIO_ADDR;
	gsw_dev.smdio_phy_addr = SMDIO_ADDR;

	return 0;
}

int32_t api_gsw_get_links(char* lib)
{
	(void)lib;
	gsw_adapt_init();
	user_socket_init();
	return 0;
}

GSW_Device_t* gsw_get_struc(uint8_t lif_id,uint8_t phy_id)
{
	(void)lif_id;
	(void)phy_id;
	return &gsw_dev;
}

int gsw_read(const GSW_Device_t *dev, uint32_t regaddr)
{
	return dev->mdiobus_read(dev->mdiobus_data, dev->phy_addr, GSW_MMD_DEV,
				 regaddr);
}

int gsw_write(const GSW_Device_t *dev, uint32_t regaddr, uint16_t data)
{
	return dev->mdiobus_write(dev->mdiobus_data, dev->phy_addr, GSW_MMD_DEV,
				  regaddr, data);
}

/* Clause 22 MDIO read and write functions for driver-specific platforms  */
int CL22_MDIO_READ(void *bus, uint8_t phyaddr, uint16_t regnum, uint16_t *pval)
{
	struct ra_mii_ioctl_data mii;
	struct ifreq ifr;

	strncpy(ifr.ifr_name, ETH_DEVNAME, 5);
	ifr.ifr_data = &mii;

	mii.phy_id = phyaddr;
	mii.reg_num = regnum;

	if (-1 == ioctl(hapi_fd, RAETH_MII_READ, &ifr)) {
		perror("ioctl(RAETH_MII_READ) failed.");
		return -EIO;
	}
	*pval = mii.val_out;

	return 0;
}

int CL22_MDIO_WRITE(void *bus, uint8_t phyaddr, uint16_t regnum, uint16_t val)
{
	struct ra_mii_ioctl_data mii;
	struct ifreq ifr;

	strncpy(ifr.ifr_name, ETH_DEVNAME, 5);
	ifr.ifr_data = &mii;

	mii.phy_id = phyaddr;
	mii.reg_num = regnum;
	mii.val_in = val;

	if (-1 == ioctl(hapi_fd, RAETH_MII_WRITE, &ifr)) {
		perror("ioctl(RAETH_MII_WRITE) failed.");
		close(hapi_fd);
		return -EIO;
	}

	return 0;
}

/* Clause 45 MDIO read and write functions for driver-specific platforms  */
int CL45_MDIO_READ(void *bus, uint8_t phyaddr, uint8_t mmd, uint16_t regnum, uint16_t *pval)
{
	struct ra_mii_ioctl_data mii;
	uint16_t reg_value;
	struct ifreq ifr;

	strncpy(ifr.ifr_name, ETH_DEVNAME, 5);
	ifr.ifr_data = &mii;

	reg_value = MTK_PHYIAC_PHY_ACS_ST |
		(phyaddr << MTK_PHYIAC_MDIO_PHY_ADDR_SHFT) | mmd;

	mii.phy_id = reg_value;
	mii.reg_num = regnum;

	if (-1 == ioctl(hapi_fd, RAETH_MII_READ_CL45, &ifr)) {
		perror("ioctl(RAETH_MII_READ_CL45) failed.");
		return -EIO;
	}
	*pval = mii.val_out;

	return 0;
}

int CL45_MDIO_WRITE(void *bus, uint8_t phyaddr, uint8_t mmd, uint16_t regnum, uint16_t val)
{
	struct ra_mii_ioctl_data mii;
	uint16_t reg_value;
	struct ifreq ifr;

	strncpy(ifr.ifr_name, ETH_DEVNAME, 5);
	ifr.ifr_data = &mii;

	reg_value = MTK_PHYIAC_PHY_ACS_ST |
		(phyaddr << MTK_PHYIAC_MDIO_PHY_ADDR_SHFT) | mmd;

	mii.phy_id = reg_value;
	mii.reg_num = regnum;
	mii.val_in = val;

	if (-1 == ioctl(hapi_fd, RAETH_MII_WRITE_CL45, &ifr)) {
		perror("ioctl(RAETH_MII_WRITE_CL45) failed.");
		return -EIO;
	}

	return 0;
}
