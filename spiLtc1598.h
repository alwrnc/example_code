/*
// ---------------------------------------------------------------
// File: spiLtc1598.h
//
// Module: SPI LTC1598 8-Channel, 12-Bit ADC device interface.
//
// Description:
//
// Version:
//
// History:
//	
// ---------------------------------------------------------------
*/


#ifndef	SPILTC1598_H
#define	SPILTC1598_H

#ifdef __cplusplus
extern "C" {
#endif


/*
// ---------------------------------------------------------------
// SPI control block definitions.
// ---------------------------------------------------------------
*/

#if 0
#define SPICB_MODE_LTC1598		0x0f7f
#else
#define SPICB_MODE_LTC1598		0x0f77
#endif


/*
// ---------------------------------------------------------------
// Defines.
// ---------------------------------------------------------------
*/


/*
// ---------------------------------------------------------------
// SPI specific definitions.
// ---------------------------------------------------------------
*/


/*
// ---------------------------------------------------------------
// SPI device limits.
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
extern void spiLtc1598Init(void);
extern void spiCsOnLtc1598(SPI_CB *cb);
extern void spiCsOffLtc1598(SPI_CB *cb);
extern int spiPostLtc1598ChannelSelect(SPI_CB *cb);
extern int spiPreLtc1598ChannelSelect(SPI_CB *cb);
extern int spiPostLtc1598Read(SPI_CB *cb);
extern int spiPreLtc1598Read(SPI_CB *cb);
extern int spiLtc1598Read(int ChipSelect, int Channel, int *Data);
#else
extern void spiLtc1598Init();
extern void spiCsOnLtc1598();
extern void spiCsOffLtc1598();
extern int spiPostLtc1598ChannelSelect();
extern int spiPreLtc1598ChannelSelect();
extern int spiPostLtc1598Read();
extern int spiPreLtc1598Read();
extern int spiLtc1598Read();
#endif	/* __STDC__ */


#ifdef __cplusplus
}
#endif

#endif	/* SPILTC1598_H */
