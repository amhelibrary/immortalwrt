/******************************************************************************

    Copyright 2022 Maxlinear

    SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0-only)

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.
******************************************************************************/

/**
   \file lif_api.c
   This file implements the Ethernet MDIO Link Interface (LIF) API.

   This API abstract the underlying Target Ethernet MDIO connection by selecting with the
   Link Interface Id (lid) the corresponding MDIO library.

   Note: The LIF abstraction is not implemented. The lif api is tailored for the C mdio library
   located in PyRPIO package. The lif will be reworked once the missing block (conf,..) will be available
   in the future.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bcm2835.h"
#include "mdio.h"
#include "lif_api.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                            Types definitions                               */
/* ========================================================================== */

/** define the state of the mdio pins in the global pin table */
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

} Y_S_lif_pins;

/*  Global Variable handling Scan function
        - scanned_devices: Scanned Devices
        - PHY_MAX_VAL: Maximum PHY Value
        - COMB_MAX_VAL: Maximum possible combination of pins
        - MAX_LINKS: Maximum number of devices connected to RPI4

*/

typedef struct
{
    /** mdio clock pin with the following values
        - (0) pin not valid
        - (1) valid pin close
        - (pin) valid open pin as clock pin */
    char *lib;

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

} device_struc;

/* ========================================================================== */
/*                           Local Function prototypes                        */
/* ========================================================================== */
static void tableinit_lif_pins(void);
static int32_t update_init_lif_lib(char *lib);
static int32_t update_deinit_lif_lib(char *lib);
static int32_t check_valid_lif_lib(char *lib);
static int32_t check_valid_lif_pins(uint8_t clk_pin, uint8_t data_pin);
static int32_t update_open_lif_pins(uint8_t clk_pin, uint8_t data_pin);
static int32_t update_close_lif_pins(uint8_t clk_pin, uint8_t data_pin);
static int32_t check_rw_lif_pins(uint8_t clk_pin, uint8_t data_pin);

/* ========================================================================== */
/*                             Global variables                                */
/* ========================================================================== */

/* ========================================================================== */
/*                             Local variables                                */
/* ========================================================================== */

/*  Global Variable for handling the initialisation and shutdown of the
    underlying mdio library. gLifMdioInit
        - MAGIC_KEY_LIF_NOT_INITIALZED: not initialized
        - MAGIC_KEY_LIF_INITIALZED: initialized
*/

#define MAGIC_KEY_LIF_NOT_INITIALZED 0xffffffff
#define MAGIC_KEY_LIF_INITIALZED 0x12345678
static int32_t gLifMdioInit;
static int32_t scanned_nr_lif;
static device_struc scanned_devices[MAX_SCAN_LINKS];

/* Runtime pin override for LIF_NO_SCAN mode.
   0 means "not set" — lif_scan() falls back to LIF_CLK_PIN / LIF_DATA_PIN.
   Set via lif_set_pins() before calling lif_scan() or lif_scan_with_pins(). */
#ifdef LIF_NO_SCAN
static uint8_t gOverrideClkPin  = 0;
static uint8_t gOverrideDataPin = 0;
#endif /* LIF_NO_SCAN */

/*  Global Table for handling or checking the validaty of the clock and data
    pins used for the mdio connection between the  RPI4 and the EVK. The valid
    pins of the 40-pin RPI4 are the GPIO pins that will be used as unique
    (clock, data) pins couple for the physical connection and driven thru the
    lif API. The valid GPIO pins are 4, 5, 6, 12, 13, 22, 23, 24, 25, 26 and 27
    meaning up to 5 mdio physical connections can be made. Each connection is
    made of a unique clock, data pins.

    Only the entries marked as "LIF_VALID_PIN" are accepted (valid) pins corresponding
    to the pins 4, 5, 6, 12, 13, 22, 23, 24, 25, 26 and 27.
    For a valid entry, the cpin and dpin fields of the Y_S_lif_pins structure provide
    the status of both pins from the initialisation (LIF_VALID_PIN)  to open (pin number)
    and close (LIF_VALID_PIN).

    For a Valid entry, the cpin field matches the entry number when the cpin is open,
    dpin will contain the data pin numver associated to the clock pin.

    Example with clock pin = 5, data pin = 6:
    initialization:
    gLifTablePins[5].cpin = LIF_VALID_PIN, gLifTablePins[5].dpin = LIF_VALID_PIN
    gLifTablePins[6].cpin = LIF_VALID_PIN, gLifTablePins[6].dpin = LIF_VALID_PIN

    open:
    clock and data pins can only be open when both pin are free (LIF_VALID_PIN),
    this is checkedwith the value in gLifTablePins[5].cpin for the clock pin
    and gLifTablePins[6].dpin for the data pin.
    When open, the table is updated as follow (see update_open_lif_pins function)
    providing the unique pins couple:
    gLifTablePins[5].cpin = 5, gLifTablePins[5].dpin = 6
    gLifTablePins[6].cpin = 5, gLifTablePins[6].dpin = 6

    close:
    Only open clock and data pins couple can be closed for that
    gLifTablePins[5].cpin = 5 for clock pin and gLifTablePins[6].dpin = 6 for data pin
    are checked.
    When closed, the table is updated as follow (see update_close_lif_pins function)
    to the init values and the pins are free for a new assignment:
    gLifTablePins[5].cpin = LIF_VALID_PIN, gLifTablePins[5].dpin = LIF_VALID_PIN
    gLifTablePins[6].cpin = LIF_VALID_PIN, gLifTablePins[6].dpin = LIF_VALID_PIN
*/
#define LIF_INVALID_PIN 0
#define LIF_VALID_PIN 1

#define LIF_TABLE_PINS_ENTRIES_MAX 32
Y_S_lif_pins gLifTablePins[LIF_TABLE_PINS_ENTRIES_MAX] =
    {
/* for documenting the initial pins mapping as the table
   is initialized in "tableinit_lif_pins ()" function */
#if 0
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 0  : not valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 1  : not valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 2  : not valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 3  : not valid */
    {LIF_VALID_PIN,     LIF_VALID_PIN},     /* 4  : valid */
    {LIF_VALID_PIN,     LIF_VALID_PIN},     /* 5  : valid */
    {LIF_VALID_PIN,     LIF_VALID_PIN},     /* 6  : valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 7  : not valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 8  : not valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 9  : not valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 10 : not valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 11 : not valid */
    {LIF_VALID_PIN,     LIF_VALID_PIN},     /* 12 : valid */
    {LIF_VALID_PIN,     LIF_VALID_PIN},     /* 13 : valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 14 : not valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 15 : not valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 16 : not valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 17 : not valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 18 : not valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 19 : not valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 20 : not valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 21 : not valid */
    {LIF_VALID_PIN,     LIF_VALID_PIN},     /* 22 : valid */
    {LIF_VALID_PIN,     LIF_VALID_PIN},     /* 23 : valid */
    {LIF_VALID_PIN,     LIF_VALID_PIN},     /* 24 : valid */
    {LIF_VALID_PIN,     LIF_VALID_PIN},     /* 25 : valid */
    {LIF_VALID_PIN,     LIF_VALID_PIN},     /* 26 : valid */
    {LIF_VALID_PIN,     LIF_VALID_PIN},     /* 27 : valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 28 : not valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 29 : not valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN},   /* 30 : not valid */
    {LIF_INVALID_PIN,   LIF_INVALID_PIN}    /* 31 : not valid */
#endif
};

#ifdef LIF_NO_SCAN
#define LIF_CLK_PIN_ENTRY 0
#define LIF_DATA_PIN_ENTRY 1

static int32_t GPIO_PINS[RASP_AVAILABLE_PINS] = {LIF_CLK_PIN, LIF_DATA_PIN};
#else
/* All RPi4 GPIO pins safe for bit-bang MDIO (see lif_api.h for exclusion rationale).
   Group A: 4-6, Group B: 12-13, Group C: 16-21 (extended), Group D: 22-27 */
static int32_t GPIO_PINS[RASP_AVAILABLE_PINS] = {4, 5, 6, 12, 13, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27};
#endif

/* ========================================================================== */
/*                   Local Function implementation                            */
/* ========================================================================== */

/**
    This function initializes the global pins table.

   \param
    none
   \return
    none
*/
static void tableinit_lif_pins(void)
{
    uint8_t i;

    /* all cleared to invalid pin */
    for (i = 0; i < LIF_TABLE_PINS_ENTRIES_MAX; i++)
    {
        gLifTablePins[i].cpin = gLifTablePins[i].dpin = LIF_INVALID_PIN;
    }

    /* set the valid pin to valid for the allowed ranges */

    /* pins 4 to 6 */
    for (i = 4; i < 7; i++)
    {
        gLifTablePins[i].cpin = LIF_VALID_PIN;
        gLifTablePins[i].dpin = gLifTablePins[i].cpin;
    }

    /* pins 12 and 13 */
    for (i = 12; i < 14; i++)
    {
        gLifTablePins[i].cpin = LIF_VALID_PIN;
        gLifTablePins[i].dpin = gLifTablePins[i].cpin;
    }

    /* pins 16 to 21 (extended: safe general-purpose GPIOs added in this revision) */
    for (i = 16; i < 22; i++)
    {
        gLifTablePins[i].cpin = LIF_VALID_PIN;
        gLifTablePins[i].dpin = gLifTablePins[i].cpin;
    }

    /* pins 22 to 27 */
    for (i = 22; i < 28; i++)
    {
        gLifTablePins[i].cpin = LIF_VALID_PIN;
        gLifTablePins[i].dpin = gLifTablePins[i].cpin;
    }
}

/**
    This function checks the validity of the library and updates
    the global pins table accordingly at the library initialisation.

   \param
    none
   \return
    LIF_API_RET_SUCCESS      No error
    LIF_API_RET_LIB_ERROR    Library error

*/
static int32_t update_init_lif_lib(char *lib)
{

    /* Ensure we select the correct mdio library for RPI4 */
    if ((lib == NULL) || (strcmp(lib, "bcm2835") != 0) || gLifMdioInit == MAGIC_KEY_LIF_INITIALZED)
    {
        return LIF_API_RET_LIB_ERROR;
    }

    /* library initialized, update the magic key */
    (void)tableinit_lif_pins();
    gLifMdioInit = MAGIC_KEY_LIF_INITIALZED;

    return LIF_API_RET_SUCCESS;
}

/**
    This function checks the validity of the library and updates
    the global pins table accordingly at the library deinitialisation.

   \param
    none
   \return
    LIF_API_RET_SUCCESS      No error
    LIF_API_RET_LIB_ERROR    Library error
*/
static int32_t update_deinit_lif_lib(char *lib)
{

    /* Ensure we select the correct mdio library for RPI4 */
    if ((lib == NULL) || (strcmp(lib, "bcm2835") != 0) || gLifMdioInit != MAGIC_KEY_LIF_INITIALZED)
    {
        return LIF_API_RET_LIB_ERROR;
    }

    /* library deinitialzed, update the magic key */
    gLifMdioInit = MAGIC_KEY_LIF_NOT_INITIALZED;

    return LIF_API_RET_SUCCESS;
}

/**
    This function checks the validity of the library for open, close, read
    and write operations.

   \param
    none
   \return
    LIF_API_RET_SUCCESS      No error
    LIF_API_RET_LIB_ERROR    Library error
*/
static int32_t check_valid_lif_lib(char *lib)
{

    /* Ensure we select the correct mdio library for RPI4 */
    if ((lib == NULL) || (strcmp(lib, "bcm2835") != 0) || gLifMdioInit != MAGIC_KEY_LIF_INITIALZED)
    {
        return LIF_API_RET_LIB_ERROR;
    }

    return LIF_API_RET_SUCCESS;
}

/**
    This function checks the validity of the used GPIO pins on the 40-pin Header
    on the RPI4 board for the clock and data signals of the mdio.

    The valid pin numbers are: 4,5,6,12,13,22,23,24,25,26 and 27.


   \param
    uint8_t clk_pin,   GPIO mdio clock pin number
    uint8_t data_pin,  GPIO mdio data pin number

   \return
    LIF_API_RET_SUCCESS      No error
    LIF_API_RET_PINS_ERROR   Pins validity error
*/
static int32_t check_valid_lif_pins(uint8_t clk_pin, uint8_t data_pin)
{
    /* check the validity of the pins */
    if ((clk_pin == data_pin) ||
        (clk_pin > (LIF_TABLE_PINS_ENTRIES_MAX - 1)) ||
        (data_pin > (LIF_TABLE_PINS_ENTRIES_MAX - 1)))
    {
        return LIF_API_RET_PINS_ERROR;
    }

    if ((gLifTablePins[clk_pin].cpin == LIF_INVALID_PIN) ||
        (gLifTablePins[data_pin].cpin == LIF_INVALID_PIN))
    {
        return LIF_API_RET_PINS_ERROR;
    }

    return LIF_API_RET_SUCCESS;
}

/**
    This function checks the validity of the pins and updates
    the global pins table accordingly for the open opearation.

   \param
    uint16_t clk_pin,   GPIO mdio clock pin number
    uint16_t data_pin,  GPIO mdio data pin number
   \return
    LIF_API_RET_SUCCESS      No error
    LIF_API_RET_PINS_ERROR   Pins validity error
    LIF_API_RET_OPEN_ERROR   Pins open error
*/
static int32_t update_open_lif_pins(uint8_t clk_pin, uint8_t data_pin)
{
    /* check pins validity */
    if (check_valid_lif_pins(clk_pin, data_pin) != LIF_API_RET_SUCCESS)
    {
        return LIF_API_RET_PINS_ERROR;
    }

    /* check if the clock and data pin are free:
       clock and data pins can only be open when both pin are free (LIF_VALID_PIN),
       this is checked with the value in gLifTablePins[clk_pin].cpin for the clock pin
       and gLifTablePins[dat_pin].dpin for the data pin.
       (refer to the gLifTablePins description) */
    if ((gLifTablePins[clk_pin].cpin > LIF_VALID_PIN) ||
        (gLifTablePins[data_pin].dpin > LIF_VALID_PIN))
    {

        return LIF_API_RET_OPEN_ERROR;
    }

    /* update pin table
       (refer to the gLifTablePins description)*/
    gLifTablePins[clk_pin].cpin = gLifTablePins[data_pin].cpin = clk_pin;
    gLifTablePins[clk_pin].dpin = gLifTablePins[data_pin].dpin = data_pin;

    return LIF_API_RET_SUCCESS;
}

/**
    This function checks the validity of the pins and updates
    the global pins table accordingly for the close opearation.

   \param
    uint16_t clk_pin,   GPIO mdio clock pin number
    uint16_t data_pin,  GPIO mdio data pin number
   \return
    LIF_API_RET_SUCCESS      No error
    LIF_API_RET_PINS_ERROR   Pins validity error
    LIF_API_RET_CLOSE_ERROR  Pins close error
*/
static int32_t update_close_lif_pins(uint8_t clk_pin, uint8_t data_pin)
{

    /* check pins validity */
    if (check_valid_lif_pins(clk_pin, data_pin) != LIF_API_RET_SUCCESS)
    {
        return LIF_API_RET_PINS_ERROR;
    }

    /* check that both pins are open:
        clk_pin matches cpin in clk_pin entry in global table pin.
        data_pin matches dpin in data_pin entry in global table pin.
        (refer to the gLifTablePins description). */
    if ((gLifTablePins[clk_pin].cpin != clk_pin) ||
        (gLifTablePins[data_pin].dpin != data_pin))
    {
        return LIF_API_RET_CLOSE_ERROR;
    }

    /* update pin table with both pins set to close (value LIF_VALID_PIN) */
    gLifTablePins[clk_pin].cpin = LIF_VALID_PIN;
    gLifTablePins[clk_pin].dpin = gLifTablePins[clk_pin].cpin;

    gLifTablePins[data_pin].cpin = LIF_VALID_PIN;
    gLifTablePins[data_pin].dpin = gLifTablePins[data_pin].cpin;

    return LIF_API_RET_SUCCESS;
}

/**
    This function checks the validity of the pins according
    to the global pin table for read and write operations.

   \param
    uint16_t clk_pin,   GPIO mdio clock pin number
    uint16_t data_pin,  GPIO mdio data pin number
   \return
    LIF_API_RET_SUCCESS      No error
    LIF_API_RET_PINS_ERROR   Pins validity error
    LIF_API_RET_ACCESS_ERROR  Pins RW Access error
*/
static int32_t check_rw_lif_pins(uint8_t clk_pin, uint8_t data_pin)
{

    /* check pins validity */
    if (check_valid_lif_pins(clk_pin, data_pin) != LIF_API_RET_SUCCESS)
    {
        return LIF_API_RET_PINS_ERROR;
    }

    /* check that both pins are open:
        clk_pin matches cpin in clk_pin entry in global table pin.
        data_pin matches dpin in data_pin entry in global table pin.
     */
    if ((gLifTablePins[clk_pin].cpin != clk_pin) ||
        (gLifTablePins[data_pin].dpin != data_pin))
    {
        return LIF_API_RET_ACCESS_ERROR;
    }
    return LIF_API_RET_SUCCESS;
}

/* ========================================================================== */
/*                   Function implementation                                  */
/* ========================================================================== */
/**
    Implements the MDIO library (PyRPIO only) Initialization.


   \param
    char*    lib,       GPIO Interface library name

   \return
    LIF_API_RET_SUCCESS      No error
    LIF_API_RET_LIB_ERROR    Library error
*/
int32_t lif_mdio_init(char *lib)
{
    int32_t ret;

    /* Ensure we select the correct mdio library for RPI4
       and update global vaiable */
    ret = update_init_lif_lib(lib);
    if (ret != LIF_API_RET_SUCCESS)
    {
        return ret;
    }

    /* initialize the mdio library:
       return code: 1=success, 0=error
    */
    if (bcm2835_init(1) == 1)
    {
        return LIF_API_RET_SUCCESS;
    }
    else
    {
        /* we do not evaluate the return code */
        (void)lif_mdio_deinit(lib);
        return LIF_API_RET_LIB_ERROR;
    }
}

/**
    Implements the MDIO library (PyRPIO only) Deinitialization.


   \param
    char*    lib,       GPIO Interface library name

   \return
    LIF_API_RET_SUCCESS      No error
    LIF_API_RET_LIB_ERROR    Library error
*/
int32_t lif_mdio_deinit(char *lib)
{
    int32_t ret;

    /* Ensure we select the correct mdio library for RPI4
       and update global vaiable */
    ret = update_deinit_lif_lib(lib);
    if (ret != LIF_API_RET_SUCCESS)
    {
        return ret;
    }

    /* shutdown the mdio library:
       return code: always 1
       we do not evaluate the return code
    */
    (void)bcm2835_close();

    return LIF_API_RET_SUCCESS;
}

/**
    Implements the MDIO library (PyRPIO only) Open.
    Initialize the MDIO clock and data pins for the MDIO connection.

   \param
    char*    lib,       GPIO Interface library name
    uint16_t clk_pin,   GPIO mdio clock pin number
    uint16_t data_pin,  GPIO mdio data pin number

   \return
    LIF_API_RET_SUCCESS      No error
    LIF_API_RET_LIB_ERROR    Library error
    LIF_API_RET_PINS_ERROR   Pins validity error
    LIF_API_RET_OPEN_ERROR   Pins open error
*/
int32_t lif_mdio_open(char *lib, uint8_t clk_pin, uint8_t data_pin)
{
    int32_t ret;

    /* check library validity */
    ret = check_valid_lif_lib(lib);
    if (ret != LIF_API_RET_SUCCESS)
    {
        return ret;
    }

    /* check and update global pins table */
    ret = update_open_lif_pins(clk_pin, data_pin);
    if (ret != LIF_API_RET_SUCCESS)
    {
        return ret;
    }

    /* Open the mdio connection
       return code: always 0.
       we do not evaluate the return code
    */
    (void)mdio_open(clk_pin, data_pin);

    return LIF_API_RET_SUCCESS;
}

/**
    Implements the MDIO library (PyRPIO only) Close.
    Deinitialize the MDIO clock and data pins of the MDIO connection.

   \param
    char*    lib,       GPIO Interface library name
    uint16_t clk_pin,   GPIO mdio clock pin number
    uint16_t data_pin,  GPIO mdio data pin number

   \return
    LIF_API_RET_SUCCESS      No error
    LIF_API_RET_LIB_ERROR    Library error
    LIF_API_RET_PINS_ERROR   Pins validity error
    LIF_API_RET_CLOSE_ERROR  Pins close error
*/

int32_t lif_mdio_close(char *lib, uint8_t clk_pin, uint8_t data_pin)
{
    int32_t ret;

    /* check library validity */
    ret = check_valid_lif_lib(lib);
    if (ret != LIF_API_RET_SUCCESS)
    {
        return ret;
    }

    /* check and update global pins table */
    ret = update_close_lif_pins(clk_pin, data_pin);
    if (ret != LIF_API_RET_SUCCESS)
    {
        return ret;
    }

    /* close the mdio connection
       return code: always 0.
       we do not evaluate the return code
    */
    (void)mdio_close(clk_pin, data_pin);

    return LIF_API_RET_SUCCESS;
}

/**
    Implements the MDIO library (PyRPIO only) Read Register Clause 22.

   \param
    uint8_t lif_id,     Link Index number
    uint16_t pad,       PHY address
    uint16_t dad,       Register Address

   \return
    >= 0                      No error: Register value
    LIF_API_RET_PINS_ERROR    Pins validity error
    LIF_API_RET_ACCESS_ERROR  Pins RW Access error
*/
int32_t lif_mdio_c22_read(uint8_t lif_id, uint8_t pad, uint8_t dad)
{
    uint16_t rret; /* return type from the underlying mdio library */
    int32_t ret;

    /* guard against out-of-bounds lif_id before array access */
    if (lif_id >= MAX_SCAN_LINKS)
        return LIF_API_RET_FAPI_PRM_ERROR;

    /* check library validity */
    ret = check_valid_lif_lib(scanned_devices[lif_id].lib);
    if (ret != LIF_API_RET_SUCCESS)
    {
        return ret;
    }

    /* check pins validity */
    ret = check_rw_lif_pins(scanned_devices[lif_id].cpin, scanned_devices[lif_id].dpin);
    if (ret != LIF_API_RET_SUCCESS)
    {
        return ret;
    }

    /* call the underlying mdio library returning
       the uint16_t register value */

    rret = mdio_c22_read(scanned_devices[lif_id].cpin, scanned_devices[lif_id].dpin, pad, dad);

    /* convert from 16-bit to 32-bit unsigned */
    return ((int32_t)rret & 0x0000FFFF);
}

/**
    Implements the MDIO library (PyRPIO only) Write Register Clause 22.

   \param
    uint8_t lif_id,     Link Index number
    uint16_t pad,       PHY address
    uint16_t dad,       Register Address
    uint16_t val,       Register value to write
   \return
    LIF_API_RET_SUCCESS      No error
    LIF_API_RET_PINS_ERROR   Pins validity error
    LIF_API_RET_ACCESS_ERROR Pins RW Access error
*/
int32_t lif_mdio_c22_write(uint8_t lif_id, uint8_t pad, uint8_t dad, uint16_t val)
{
    int32_t ret;

    /* guard against out-of-bounds lif_id before array access */
    if (lif_id >= MAX_SCAN_LINKS)
        return LIF_API_RET_FAPI_PRM_ERROR;

    /* check library validity */
    ret = check_valid_lif_lib(scanned_devices[lif_id].lib);
    if (ret != LIF_API_RET_SUCCESS)
    {
        return ret;
    }

    /* check pins validity */
    ret = check_rw_lif_pins(scanned_devices[lif_id].cpin, scanned_devices[lif_id].dpin);
    if (ret != LIF_API_RET_SUCCESS)
    {
        return ret;
    }

    /* call the underlying mdio library returning
       always 0. We do not evaluate the return code */
    (void)mdio_c22_write(scanned_devices[lif_id].cpin, scanned_devices[lif_id].dpin, pad, dad, val);

    return LIF_API_RET_SUCCESS;
}

/**
    Implements the MDIO library (PyRPIO only) Read Register Clause 45.

   \param
    uint8_t lif_id,     Link Index number
    uint16_t pad,       PHY address
    uint16_t dad,       DEVICE address
    uint16_t reg,       Register Address

   \return
    >= 0                      No error: Register value
    LIF_API_RET_PINS_ERROR    Pins validity error
    LIF_API_RET_ACCESS_ERROR  Pins RW Access error
*/
int32_t lif_mdio_c45_read(uint8_t lif_id, uint8_t pad, uint8_t dad, uint16_t reg)
{
    uint16_t rret; /* return type from the underlying mdio library */
    int32_t ret;

    /* guard against out-of-bounds lif_id before array access */
    if (lif_id >= MAX_SCAN_LINKS)
        return LIF_API_RET_FAPI_PRM_ERROR;

    /* check library validity */
    ret = check_valid_lif_lib(scanned_devices[lif_id].lib);
    if (ret != LIF_API_RET_SUCCESS)
    {
        return ret;
    }

    /* check pins validity */
    ret = check_rw_lif_pins(scanned_devices[lif_id].cpin, scanned_devices[lif_id].dpin);
    if (ret != LIF_API_RET_SUCCESS)
    {
        return ret;
    }

    /* call the underlying mdio library returning
       the uint16_t register value */
    rret = mdio_c45_read(scanned_devices[lif_id].cpin, scanned_devices[lif_id].dpin, pad, dad, reg);

    /*  convert from 16-bit to 32-bit unsigned */
    return ((int32_t)rret & 0x0000FFFF);
}

/**
    Implements the MDIO library (PyRPIO only) Write Register Clause 45.

   \param
    uint8_t lif_id,     Link Index number
    uint16_t pad,       PHY address
    uint16_t dad,       DEVICE Address
    uint16_t val,       Register value to write
   \return
    LIF_API_RET_SUCCESS      No error
    LIF_API_RET_PINS_ERROR   Pins validity error
    LIF_API_RET_ACCESS_ERROR Pins RW Access error
*/
int32_t lif_mdio_c45_write(uint8_t lif_id, uint8_t pad, uint8_t dad, uint16_t reg, uint16_t val)
{
    int32_t ret;

    /* guard against out-of-bounds lif_id before array access */
    if (lif_id >= MAX_SCAN_LINKS)
        return LIF_API_RET_FAPI_PRM_ERROR;

    /* check library validity */
    ret = check_valid_lif_lib(scanned_devices[lif_id].lib);
    if (ret != LIF_API_RET_SUCCESS)
    {
        return ret;
    }

    /* check pins validity */
    ret = check_rw_lif_pins(scanned_devices[lif_id].cpin, scanned_devices[lif_id].dpin);
    if (ret != LIF_API_RET_SUCCESS)
    {
        return ret;
    }

    /* call the underlying mdio library returning
       always 0. We do not evaluate the return code */
    (void)mdio_c45_write(scanned_devices[lif_id].cpin, scanned_devices[lif_id].dpin, pad, dad, reg, val);

    return LIF_API_RET_SUCCESS;
}

/* =========================================================================
 * Stable register read helper.
 *
 * Reads a C22 register STABLE_READ_COUNT times and returns its value only
 * when every read returns the same value.  If any read differs, 0xFFFF is
 * returned — the same sentinel used for "bus idle / no device present".
 *
 * Rationale: a real MDIO device (including the MxL862xx GPHY on the SMDIO
 * bus) always returns a stable, repeatable value for its identifier
 * registers.  A floating GPIO line or a wrong pin combination produces
 * noise and returns inconsistent values across back-to-back reads.
 * Requiring stability across STABLE_READ_COUNT reads eliminates these
 * false positives without adding noticeable latency on a real device.
 * ========================================================================= */
#define STABLE_READ_COUNT 3

/* Maximum number of set bits allowed in an SMDIO probe result.
 * A real SMDIO device returns a register value with few bits set.
 * Floating GPIO lines drift to HIGH after the clock phase ends, causing
 * the MDIO data bus to return patterns with many bits set (e.g. 0x3fff or
 * 0x7fff).  Rejecting values whose popcount exceeds this threshold
 * eliminates floating-pin false positives without affecting real devices. */
#define SMDIO_PROBE_MAX_POPCOUNT 10

static int s_popcount16(uint16_t v)
{
    v = v - ((v >> 1) & 0x5555u);
    v = (v & 0x3333u) + ((v >> 2) & 0x3333u);
    v = (v + (v >> 4)) & 0x0f0fu;
    return (int)((v * 0x0101u) >> 8);
}

static int32_t read_c22_stable(uint8_t lif_id, uint8_t phy, uint8_t reg)
{
    int32_t val = lif_mdio_c22_read(lif_id, phy, reg);
    int i;
    /* A negative return code (LIB_ERROR=-1, PINS_ERROR=-2, etc.) is a
       consistent error — all three reads agree on -1.  Without this guard
       the -1 is treated as a stable value and stored as (uint16_t)-1 =
       0xFFFF, which is NOT equal to the int32 sentinel 0xffff (65535) used
       in the filter below, creating spurious false-positive devices. */
    if (val < 0)
        return 0xffff;
    for (i = 1; i < STABLE_READ_COUNT; i++) {
        int32_t v = lif_mdio_c22_read(lif_id, phy, reg);
        if (v < 0 || v != val)
            return 0xffff;  /* inconsistent reads or error — not a real device */
    }
    return val;
}

/* =========================================================================
 * SMDIO indirect read with stability check.
 *
 * The MxL862xx SMDIO bus uses a 2-step C22 indirect access protocol:
 *   1. Write the 16-bit target register address to C22 reg 0x1F.
 *   2. Read the data from C22 reg 0x00.
 * This helper repeats the full 2-step cycle STABLE_READ_COUNT times and
 * returns the stable value, or 0xffff if any result differs.
 * ========================================================================= */
static int32_t smdio_read_stable(uint8_t lif_id, uint8_t phy, uint16_t target_reg)
{
    int32_t val, v;
    int i;

    if (lif_mdio_c22_write(lif_id, phy, 0x1F, target_reg) != 0)
        return 0xffff;
    val = lif_mdio_c22_read(lif_id, phy, 0x00);
    if (val < 0)
        return 0xffff;

    for (i = 1; i < STABLE_READ_COUNT; i++) {
        lif_mdio_c22_write(lif_id, phy, 0x1F, target_reg);
        v = lif_mdio_c22_read(lif_id, phy, 0x00);
        if (v < 0 || v != val)
            return 0xffff;  /* inconsistent or error — not a real device */
    }
    return val;
}

/* =========================================================================
 * MxL862xx device identification.
 *
 * Two independent methods are tried for each PHY found on the link.
 * Either passing is sufficient to confirm an MxL862xx.
 *
 * Method 1 — C22 register-address echo:
 *   The MxL862xx SMDIO interface echoes the C22 register address as the
 *   read data (reading reg 2 returns 2, reading reg 3 returns 3, etc.).
 *   No standard IEEE 802.3 PHY exhibits this behaviour.
 *
 * Method 2 — SMDIO indirect register probe:
 *   MxL862xx-specific registers 0xf383 and 0x0384, accessed via the SMDIO
 *   indirect 2-step protocol (write addr to reg 0x1F, read data from
 *   reg 0x00), return stable non-trivial values on MxL862xx.  Standard
 *   PHYs and floating lines return 0x0000 or 0xffff for these addresses.
 *
 * The stable-read helper is used for both methods to reject noise from
 * floating GPIO lines or wrong pin combinations.
 * ========================================================================= */
/* SMDIO registers with known stable non-trivial values on MxL862xx */
#define MXL_SMDIO_REG_A 0xf383
#define MXL_SMDIO_REG_B 0x0384
/* Single probe register: returns a stable non-trivial value on all MaxLinear
   MDIO devices (MxL862xx and GPY-type alike). Used during scan discovery.
   lif_is_mxl862xx() uses the dual MXL_SMDIO_REG_A/B check to distinguish
   device families after initial discovery. */
#define SMDIO_PROBE_REG 0xf384

int32_t lif_is_mxl862xx(uint8_t lif_id)
{
    uint8_t nr_phys, p;

    if (lif_id >= MAX_SCAN_LINKS || scanned_devices[lif_id].nr_phys == 0)
        return 0;

    nr_phys = scanned_devices[lif_id].nr_phys;
    for (p = 0; p < nr_phys; p++) {
        uint8_t phy_addr = (uint8_t)scanned_devices[lif_id].phy_info[p][0];

        /* C22 register-address echo: the MxL862xx SMDIO interface echoes the
           C22 register address back as the read data for any register.
           Check four consecutive registers (2, 3, 4, 5) for robustness —
           no real IEEE 802.3 PHY has an OUI that produces this pattern. */
        if (read_c22_stable(lif_id, phy_addr, 2) == 2 &&
            read_c22_stable(lif_id, phy_addr, 3) == 3 &&
            read_c22_stable(lif_id, phy_addr, 4) == 4 &&
            read_c22_stable(lif_id, phy_addr, 5) == 5)
            return 1;
    }
    return 0;
}

/******************************************
    Implements MDIO links scans functions.
*******************************************/

#ifdef LIF_NO_SCAN

/**
    Store a runtime CLK/DATA pin override for LIF_NO_SCAN mode.
    Call before lif_scan() to use pins other than LIF_CLK_PIN/LIF_DATA_PIN.
    Pass 0,0 to clear the override and revert to compile-time defaults.

   \param clk_pin   BCM GPIO number to use as MDIO clock
   \param data_pin  BCM GPIO number to use as MDIO data
   \return LIF_API_RET_SUCCESS or LIF_API_RET_PINS_ERROR
*/
int32_t lif_set_pins(uint8_t clk_pin, uint8_t data_pin)
{
    if (clk_pin == 0 && data_pin == 0)
    {
        /* clear override — revert to compile-time defaults */
        gOverrideClkPin  = 0;
        gOverrideDataPin = 0;
        return LIF_API_RET_SUCCESS;
    }

    if (clk_pin == data_pin ||
        clk_pin  >= LIF_TABLE_PINS_ENTRIES_MAX ||
        data_pin >= LIF_TABLE_PINS_ENTRIES_MAX)
    {
        fprintf(stderr, "lif_set_pins: invalid pin numbers CLK=%u DATA=%u\n",
                clk_pin, data_pin);
        return LIF_API_RET_PINS_ERROR;
    }

    gOverrideClkPin  = clk_pin;
    gOverrideDataPin = data_pin;
    return LIF_API_RET_SUCCESS;
}

/**
    Convenience wrapper: set pins and immediately scan.
    Equivalent to lif_set_pins(clk_pin, data_pin) + lif_scan(lib).

   \param lib       Library name (must be "bcm2835")
   \param clk_pin   BCM GPIO number to use as MDIO clock
   \param data_pin  BCM GPIO number to use as MDIO data
   \return number of links found, or 0 on error
*/
int32_t lif_scan_with_pins(char *lib, uint8_t clk_pin, uint8_t data_pin)
{
    int32_t ret = lif_set_pins(clk_pin, data_pin);
    if (ret != LIF_API_RET_SUCCESS)
        return 0;
    return lif_scan(lib);
}

int32_t lif_scan(char *lib)
{
    int32_t ret;
    int32_t nr_dev;
    uint8_t phy;
    uint8_t phy_index;
    uint8_t use_clk;
    uint8_t use_data;

    /* Fix #3+#10: clear all scan state so re-scan starts fresh */
    memset(scanned_devices, 0, sizeof(scanned_devices));
    scanned_nr_lif = 0;

    /* Select pins: runtime override takes priority, else compile-time defaults */
    if (gOverrideClkPin != 0 || gOverrideDataPin != 0)
    {
        use_clk  = gOverrideClkPin;
        use_data = gOverrideDataPin;
    }
    else
    {
        use_clk  = GPIO_PINS[LIF_CLK_PIN_ENTRY];
        use_data = GPIO_PINS[LIF_DATA_PIN_ENTRY];
        fprintf(stderr, "lif_scan: using default pins CLK=%u DATA=%u\n",
                use_clk, use_data);
    }

    nr_dev = 0;
    scanned_devices[nr_dev].lib  = lib;
    scanned_devices[nr_dev].cpin = use_clk;
    scanned_devices[nr_dev].dpin = use_data;

    ret = lif_mdio_open(lib, scanned_devices[nr_dev].cpin, scanned_devices[nr_dev].dpin);
    if (ret != LIF_API_RET_SUCCESS)
    {
        scanned_nr_lif = 0;
        return scanned_nr_lif;
    }

    phy_index = 0;
    for (phy = 0; phy < PHY_MAX_VAL; phy++)
    {
        /* Read both PHY Identifier registers (reg 2 and reg 3) with
           stability check: read each register STABLE_READ_COUNT times and
           require all reads to match.  A real MDIO device always returns a
           stable repeatable value; a floating GPIO or wrong pin combination
           produces inconsistent noise (read_c22_stable returns 0xFFFF).
           0xFFFF,0xFFFF = no device / bus idle — skip.
           0x0000,0x0000 = floating GPIO line (pull-down) — skip. */
        int32_t id2 = read_c22_stable(nr_dev, phy, 2);
        int32_t id3 = read_c22_stable(nr_dev, phy, 3);

        /* Both reg2 and reg3 must be non-zero and non-0xffff.
           0x0000 = floating pull-down or no response.
           0xffff = bus idle / no device. */
        if (id2 == 0x0000 || id2 == 0xffff ||
            id3 == 0x0000 || id3 == 0xffff)
            continue;

        scanned_devices[nr_dev].nr_phys++;
        scanned_devices[nr_dev].phy_info[phy_index][0] = phy;
        scanned_devices[nr_dev].phy_info[phy_index][1] = id3;
        phy_index++;
    }

    if (phy_index == 0)
    {
        /* No PHY found on these pins — close and report 0 links so callers
           do not dereference an uninitialised GSW_Device_t. */
        lif_mdio_close(lib, scanned_devices[nr_dev].cpin, scanned_devices[nr_dev].dpin);
        fprintf(stderr, "lif_scan: no MDIO device found on CLK=%u DATA=%u.\n"
                        "         Check GPIO pin wiring and LIF_CLK_PIN/LIF_DATA_PIN settings.\n",
                scanned_devices[nr_dev].cpin, scanned_devices[nr_dev].dpin);
        scanned_nr_lif = 0;
        return scanned_nr_lif;
    }

    nr_dev++;
    scanned_nr_lif = nr_dev;

    return scanned_nr_lif;
}

#else  /* LIF_NO_SCAN */

/* In full-scan mode lif_set_pins() is a no-op: the scan tries all pin
   combinations automatically.  lif_scan_with_pins() ignores the pin
   arguments and delegates to lif_scan(). */
int32_t lif_set_pins(uint8_t clk_pin, uint8_t data_pin)
{
    (void)clk_pin;
    (void)data_pin;
    fprintf(stderr, "lif_set_pins: ignored in full-scan mode"
            " (LIF_NO_SCAN not defined) \u2014 all GPIO combinations are probed.\n");
    return LIF_API_RET_SUCCESS;
}

int32_t lif_scan_with_pins(char *lib, uint8_t clk_pin, uint8_t data_pin)
{
    int32_t ret;
    uint8_t phy, phy_index;

    /* If no specific pins given, fall back to the full combination scan */
    if (clk_pin == 0 || data_pin == 0 || clk_pin == data_pin)
        return lif_scan(lib);

    /* Targeted single-pair scan — same logic as LIF_NO_SCAN lif_scan() but
       executed with explicitly supplied pins.  Much faster than iterating all
       272 pin combinations when the correct pair is already known. */
    fprintf(stderr, "lif_scan_with_pins: targeted scan CLK=%u DATA=%u\n",
            clk_pin, data_pin);

    memset(scanned_devices, 0, sizeof(scanned_devices));
    scanned_nr_lif = 0;

    ret = lif_mdio_open(lib, clk_pin, data_pin);
    if (ret != LIF_API_RET_SUCCESS) {
        fprintf(stderr, "lif_scan_with_pins: mdio_open failed (ret=%d) —"
                " cache may be stale, run fapi-lif-scan to rescan.\n", ret);
        return 0;
    }

    scanned_devices[0].lib  = lib;
    scanned_devices[0].cpin = clk_pin;
    scanned_devices[0].dpin = data_pin;

    phy_index = 0;
    for (phy = 0; phy < PHY_MAX_VAL; phy++)
    {
        int32_t id2 = read_c22_stable(0, phy, 2);
        int32_t id3 = read_c22_stable(0, phy, 3);

        /* Both reg2 and reg3 must be non-zero and non-0xffff.
           0x0000 = floating pull-down or no response.
           0xffff = bus idle / no device. */
        if (id2 == 0x0000 || id2 == 0xffff ||
            id3 == 0x0000 || id3 == 0xffff)
            continue;

        scanned_devices[0].nr_phys++;
        scanned_devices[0].phy_info[phy_index][0] = phy;
        scanned_devices[0].phy_info[phy_index][1] = id3;
        phy_index++;
    }

    /* Method 2: SMDIO indirect probe.
       Two MxL862xx-specific registers are probed: 0xf383 and 0xf384.
       A device is accepted if either register returns a stable non-trivial
       value — not 0x0000 (no response), not 0xffff (bus idle), and with
       popcount <= SMDIO_PROBE_MAX_POPCOUNT (rejects floating GPIO noise).
       Only the four known valid SMDIO device addresses are tried
       (0x1f first as it is the most common).
       smdio_read_stable() repeats the 2-step read three times; all reads
       must agree before the result is accepted. */
    if (phy_index == 0) {
        static const uint8_t smdio_addrs[] = { 0x1f, 0x08, 0x10, 0x1c };
        static const uint16_t probe_regs[] = { MXL_SMDIO_REG_A, MXL_SMDIO_REG_B };
        int si;
        for (si = 0; si < (int)(sizeof(smdio_addrs)/sizeof(smdio_addrs[0])) && phy_index == 0; si++) {
            uint8_t sphy = smdio_addrs[si];
            int ri;
            for (ri = 0; ri < (int)(sizeof(probe_regs)/sizeof(probe_regs[0])); ri++) {
                int32_t vp = smdio_read_stable(0, sphy, probe_regs[ri]);
                if (vp != 0x0000 && vp != 0xffff &&
                    s_popcount16((uint16_t)vp) <= SMDIO_PROBE_MAX_POPCOUNT) {
                    scanned_devices[0].nr_phys = 1;
                    scanned_devices[0].phy_info[0][0] = sphy;
                    scanned_devices[0].phy_info[0][1] = (uint16_t)vp;
                    phy_index = 1;
                    break;  /* either register passing is sufficient */
                }
            }
        }
    }

    if (phy_index == 0) {
        lif_mdio_close(lib, clk_pin, data_pin);
        fprintf(stderr, "lif_scan_with_pins: no device found on CLK=%u DATA=%u"
                " — cache may be stale, run fapi-lif-scan to force a new scan.\n",
                clk_pin, data_pin);
        return 0;
    }

    scanned_nr_lif = 1;
    return 1;
}

int32_t lif_scan(char *lib)
{
    int i, j;
    int32_t nr_dev = 0;
    /* Fix #2: generate all ordered (clk,data) pairs — both orderings per unique
       pin pair are tried because we do not know ahead of time which GPIO the
       user wired as CLK vs DATA.  COMB_MAX_VAL is now derived from
       RASP_AVAILABLE_PINS so the array size is always correct (#1). */
    uint8_t pins[COMB_MAX_VAL][2];
    int count = 0;

    /* Fix #3+#10: clear all scan state so a re-scan starts fresh */
    memset(scanned_devices, 0, sizeof(scanned_devices));
    scanned_nr_lif = 0;

    /* Build ordered pair table: (i,j) and (j,i) for every unique pair */
    for (i = 0; i < RASP_AVAILABLE_PINS; i++)
    {
        for (j = i + 1; j < RASP_AVAILABLE_PINS; j++)
        {
            pins[count][0] = GPIO_PINS[i];
            pins[count][1] = GPIO_PINS[j];
            count++;
            pins[count][0] = GPIO_PINS[j];
            pins[count][1] = GPIO_PINS[i];
            count++;
        }
    }

    for (i = 0; i < count; i++)
    {
        int32_t ret;

        /* Fix #8: MAX_LINKS reached — close the current candidate pin pair
           (if it happens to be open) before returning cleanly */
        if (nr_dev >= MAX_LINKS)
        {
            scanned_nr_lif = nr_dev;
            return scanned_nr_lif;
        }

        ret = lif_mdio_open(lib, pins[i][0], pins[i][1]);
        if (ret == LIF_API_RET_SUCCESS)
        {
            uint8_t phy;
            uint8_t phy_index = 0;

            scanned_devices[nr_dev].lib = lib;
            scanned_devices[nr_dev].cpin = pins[i][0];
            scanned_devices[nr_dev].dpin = pins[i][1];

            for (phy = 0; phy < PHY_MAX_VAL; phy++)
            {
                /* Read both PHY Identifier registers (reg 2 and reg 3)
                   with stability check (STABLE_READ_COUNT reads must agree).
                   0xFFFF,0xFFFF = no device / bus idle — skip.
                   0x0000,0x0000 = floating GPIO line (pull-down) — skip. */
                int32_t id2 = read_c22_stable(nr_dev, phy, 2);
                int32_t id3 = read_c22_stable(nr_dev, phy, 3);

                /* Both reg2 and reg3 must be non-zero and non-0xffff.
                   0x0000 = floating pull-down or no response.
                   0xffff = bus idle / no device. */
                if (id2 == 0x0000 || id2 == 0xffff ||
                    id3 == 0x0000 || id3 == 0xffff)
                    continue;

                scanned_devices[nr_dev].nr_phys++;
                scanned_devices[nr_dev].phy_info[phy_index][0] = phy;
                scanned_devices[nr_dev].phy_info[phy_index][1] = id3;
                phy_index++;
            }

            /* Method 2: SMDIO indirect probe.
               Two MxL862xx-specific registers are probed: 0xf383 and 0xf384.
               A device is accepted if either register returns a stable
               non-trivial value — not 0x0000 (no response), not 0xffff
               (bus idle), and with popcount <= SMDIO_PROBE_MAX_POPCOUNT
               (rejects floating GPIO noise).
               Only the four known valid SMDIO device addresses are probed
               (0x1f first as it is the most common).
               smdio_read_stable() repeats the 2-step read three times; all
               reads must agree before the result is accepted. */
            if (phy_index == 0) {
                static const uint8_t smdio_addrs[] = { 0x1f, 0x08, 0x10, 0x1c };
                static const uint16_t probe_regs[] = { MXL_SMDIO_REG_A, MXL_SMDIO_REG_B };
                int si;
                for (si = 0; si < (int)(sizeof(smdio_addrs)/sizeof(smdio_addrs[0])) && phy_index == 0; si++) {
                    uint8_t sphy = smdio_addrs[si];
                    int ri;
                    for (ri = 0; ri < (int)(sizeof(probe_regs)/sizeof(probe_regs[0])); ri++) {
                        int32_t vp = smdio_read_stable(nr_dev, sphy, probe_regs[ri]);
                        if (vp != 0x0000 && vp != 0xffff &&
                            s_popcount16((uint16_t)vp) <= SMDIO_PROBE_MAX_POPCOUNT) {
                            scanned_devices[nr_dev].nr_phys = 1;
                            scanned_devices[nr_dev].phy_info[0][0] = sphy;
                            scanned_devices[nr_dev].phy_info[0][1] = (uint16_t)vp;
                            phy_index = 1;
                            break;  /* either register passing is sufficient */
                        }
                    }
                }
            }

            if (phy_index > 0)
            {
                nr_dev++;
            }
            else
            {
                /* Fix #8: no device found — close the pin pair so these pins
                   remain available for the next combination */
                lif_mdio_close(lib, pins[i][0], pins[i][1]);
            }
        }
    }

    scanned_nr_lif = nr_dev;
    return scanned_nr_lif;
}
#endif /* LIF_NO_SCAN */

/**
    Full GPIO pin-combination scan — always compiled regardless of LIF_NO_SCAN.
    Tries every ordered (CLK, DATA) pair from the RASP_AVAILABLE_PINS list.
    Used by fapi-lif-scan to discover the actual GPIO wiring on any setup.

    Unlike lif_scan() (which in LIF_NO_SCAN mode only tests the compile-time
    default pins), this function always sweeps all 272 combinations and is
    therefore the right tool for initial bring-up and pin discovery.

   \param lib   Library name (must be "bcm2835")
   \return      Number of links found
*/
int32_t lif_full_scan(char *lib)
{
    /* All RPi4 GPIO pins safe for bit-bang MDIO */
    static const int32_t ALL_PINS[] = {
        4, 5, 6, 12, 13, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27
    };
    const int N = (int)(sizeof(ALL_PINS) / sizeof(ALL_PINS[0]));
    const int COMB = N * (N - 1);
    int i, j;
    int32_t nr_dev = 0;
    /* VLA would require C99 — use a fixed-size array (17*16 = 272) */
    uint8_t pins[272][2];
    int count = 0;

    (void)COMB;

    memset(scanned_devices, 0, sizeof(scanned_devices));
    scanned_nr_lif = 0;

    for (i = 0; i < N; i++)
        for (j = i + 1; j < N; j++) {
            pins[count][0] = (uint8_t)ALL_PINS[i];
            pins[count][1] = (uint8_t)ALL_PINS[j];
            count++;
            pins[count][0] = (uint8_t)ALL_PINS[j];
            pins[count][1] = (uint8_t)ALL_PINS[i];
            count++;
        }

    for (i = 0; i < count; i++) {
        int32_t ret;

        if (nr_dev >= MAX_SCAN_LINKS) {
            scanned_nr_lif = nr_dev;
            return scanned_nr_lif;
        }

        ret = lif_mdio_open(lib, pins[i][0], pins[i][1]);
        if (ret == LIF_API_RET_SUCCESS) {
            uint8_t phy;
            uint8_t phy_index = 0;

            scanned_devices[nr_dev].lib  = lib;
            scanned_devices[nr_dev].cpin = pins[i][0];
            scanned_devices[nr_dev].dpin = pins[i][1];

            for (phy = 0; phy < PHY_MAX_VAL; phy++) {
                int32_t id2 = read_c22_stable(nr_dev, phy, 2);
                int32_t id3 = read_c22_stable(nr_dev, phy, 3);

                /* Both reg2 and reg3 must be non-zero and non-0xffff.
                   0x0000 = floating pull-down or no response.
                   0xffff = bus idle / no device. */
                if (id2 == 0x0000 || id2 == 0xffff ||
                    id3 == 0x0000 || id3 == 0xffff)
                    continue;

                scanned_devices[nr_dev].nr_phys++;
                scanned_devices[nr_dev].phy_info[phy_index][0] = phy;
                scanned_devices[nr_dev].phy_info[phy_index][1] = id3;
                phy_index++;
            }

            /* Method 2: SMDIO indirect probe.
               Two MxL862xx-specific registers are probed: 0xf383 and 0xf384.
               A device is accepted if either register returns a stable
               non-trivial value — not 0x0000 (no response), not 0xffff
               (bus idle), and with popcount <= SMDIO_PROBE_MAX_POPCOUNT
               (rejects floating GPIO noise).
               Only the four known valid SMDIO device addresses are probed
               (0x1f first as it is the most common).
               smdio_read_stable() repeats the 2-step read three times; all
               reads must agree before the result is accepted. */
            if (phy_index == 0) {
                static const uint8_t smdio_addrs[] = { 0x1f, 0x08, 0x10, 0x1c };
                static const uint16_t probe_regs[] = { MXL_SMDIO_REG_A, MXL_SMDIO_REG_B };
                int si;
                for (si = 0; si < (int)(sizeof(smdio_addrs)/sizeof(smdio_addrs[0])) && phy_index == 0; si++) {
                    uint8_t sphy = smdio_addrs[si];
                    int ri;
                    for (ri = 0; ri < (int)(sizeof(probe_regs)/sizeof(probe_regs[0])); ri++) {
                        int32_t vp = smdio_read_stable(nr_dev, sphy, probe_regs[ri]);
                        if (vp != 0x0000 && vp != 0xffff &&
                            s_popcount16((uint16_t)vp) <= SMDIO_PROBE_MAX_POPCOUNT) {
                            scanned_devices[nr_dev].nr_phys = 1;
                            scanned_devices[nr_dev].phy_info[0][0] = sphy;
                            scanned_devices[nr_dev].phy_info[0][1] = (uint16_t)vp;
                            phy_index = 1;
                            break;  /* either register passing is sufficient */
                        }
                    }
                }
            }

            if (phy_index > 0)
                nr_dev++;
            else
                lif_mdio_close(lib, pins[i][0], pins[i][1]);
        }
    }

    scanned_nr_lif = nr_dev;
    return scanned_nr_lif;
}

/**
    Open the MDIO bus on the specified CLK/DATA pins without any device
    fingerprint scan.  Use this when the connected device does not expose
    standard MDIO PHY-ID registers (reg2/reg3) or MaxLinear SMDIO probe
    registers, so lif_scan_with_pins() cannot detect it automatically.

    After this call lif_id 0 is valid for raw lif_mdio_c22_read/write
    and smdio_read/write operations.  lif_get_nr_phys(0) returns 0
    (no PHY discovered), so GSW API calls that require a PHY entry will
    not work until a proper scan finds the device.

   \param lib       Library name (must be "bcm2835")
   \param clk_pin   GPIO clock pin
   \param data_pin  GPIO data pin
   \return          1 on success, 0 on failure
*/
int32_t lif_open_forced(char *lib, uint8_t clk_pin, uint8_t data_pin)
{
    int32_t ret;

    memset(scanned_devices, 0, sizeof(scanned_devices));
    scanned_nr_lif = 0;

    ret = lif_mdio_open(lib, clk_pin, data_pin);
    if (ret != LIF_API_RET_SUCCESS) {
        fprintf(stderr, "lif_open_forced: mdio_open failed (ret=%d) for"
                " CLK=%u DATA=%u.\n", ret, clk_pin, data_pin);
        return 0;
    }

    scanned_devices[0].lib  = lib;
    scanned_devices[0].cpin = clk_pin;
    scanned_devices[0].dpin = data_pin;
    scanned_devices[0].nr_phys = 0; /* no PHY scan — raw MDIO access only */

    scanned_nr_lif = 1;
    fprintf(stderr, "lif_open_forced: CLK=%u DATA=%u opened"
            " (raw MDIO access, no device scan).\n", clk_pin, data_pin);
    return 1;
}

int32_t lif_get_cpin(uint8_t lif_id)
{
    if (lif_id >= MAX_SCAN_LINKS)
        return LIF_API_RET_FAPI_PRM_ERROR;
    return scanned_devices[lif_id].cpin;
}

int32_t lif_get_dpin(uint8_t lif_id)
{
    if (lif_id >= MAX_SCAN_LINKS)
        return LIF_API_RET_FAPI_PRM_ERROR;
    return scanned_devices[lif_id].dpin;
}

int32_t lif_get_nr_phys(uint8_t lif_id)
{
    if (lif_id >= MAX_SCAN_LINKS)
        return LIF_API_RET_FAPI_PRM_ERROR;
    return scanned_devices[lif_id].nr_phys;
}

int32_t lif_get_phy_addr(uint8_t lif_id, uint8_t phy)
{
    if (lif_id >= MAX_SCAN_LINKS || phy >= PHY_MAX_VAL)
        return LIF_API_RET_FAPI_PRM_ERROR;
    return scanned_devices[lif_id].phy_info[phy][0];
}

int32_t lif_get_phy_id(uint8_t lif_id, uint8_t phy)
{
    if (lif_id >= MAX_SCAN_LINKS || phy >= PHY_MAX_VAL)
        return LIF_API_RET_FAPI_PRM_ERROR;
    return scanned_devices[lif_id].phy_info[phy][1];
}

int32_t lif_get_nr_lif(void)
{
    return scanned_nr_lif;
}
