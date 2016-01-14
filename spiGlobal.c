/*
// ---------------------------------------------------------------
// File: spiGlobal.c
//
// Module: SPI global variables.
//
// Description:
//
// Operation:
//
// Version:
//
// History:
//	
// ---------------------------------------------------------------
*/


/*
// ---------------------------------------------------------------
// Header files.
// ---------------------------------------------------------------
*/

#include <spiLib.h>


/*
// ---------------------------------------------------------------
// Global variables.
// ---------------------------------------------------------------
*/


SPI_HDR SpiHdr;
SPI_STAT SpiStat;
SPI_CB SpiCB[SPI_MAX_CB];

int SpiMaxCB = SPI_MAX_CB;
int	spiPriority = 2;
int	spiOptions = 0;
int	spiStackSize = 8000;
int spiDebug = 0;
int spiMsgTimeout = WAIT_FOREVER;
int spiWdgTimeout = 5000;

char spiTxBuffer[SPI_BUFFER_SIZE];
char spiRxBuffer[SPI_BUFFER_SIZE];
