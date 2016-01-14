/*
// ---------------------------------------------------------------
// File: spiTempSensor.h
//
// Module: SPI temperature sensor device interface.
//
// Description:
//
// Version:
//
// History:
//	
// ---------------------------------------------------------------
*/


#ifndef	SPITEMPSENSOR_H
#define	SPITEMPSENSOR_H

#ifdef __cplusplus
extern "C" {
#endif


/*
// ---------------------------------------------------------------
// SPI temperature sensor control block definitions.
// ---------------------------------------------------------------
*/

#define SPICB_MODE_TEMPSENSOR		0x0f70


/*
// ---------------------------------------------------------------
// Defines.
// ---------------------------------------------------------------
*/


/*
// ---------------------------------------------------------------
// SPI temperature sensor specific definitions.
// ---------------------------------------------------------------
*/


/*
// ---------------------------------------------------------------
// SPI temperature sensor device limits.
// ---------------------------------------------------------------
*/


/*
// ---------------------------------------------------------------
// Type definitions.
// ---------------------------------------------------------------
*/


/*
// ---------------------------------------------------------------
// Function declarations.
// ---------------------------------------------------------------
*/

#if defined(__STDC__) || defined(__cplusplus)
extern void spiTempSensorInit(void);
extern void spiCsOnTempSensor(void);
extern void spiCsOffTempSensor(void);
extern int spiPostTempSensorRead(SPI_CB *cb);
extern int spiPreTempSensorRead(SPI_CB *cb);
extern int spiTempSensorRead(int *piCelsius);
#else
extern void spiTempSensorInit();
extern void spiCsOnTempSensor();
extern void spiCsOffTempSensor();
extern int spiPostTempSensorRead();
extern int spiPreTempSensorRead();
extern int spiTempSensorRead();
#endif	/* __STDC__ */


#ifdef __cplusplus
}
#endif

#endif	/* SPITEMPSENSOR_H */
