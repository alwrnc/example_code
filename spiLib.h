/*
// ---------------------------------------------------------------
// File: spiLib.h
//
// Module: SPI device driver header file
//
// Description: This file contains defines and data structures
//		for the SPI device driver.
//
// Version:
//
// History:
//	AL     Create
// ---------------------------------------------------------------
*/


#ifndef	SPILIB_H
#define	SPILIB_H

#include "VxWorks.h"
#include "semLib.h"
#include "ioLib.h"
#include "taskLib.h"
#include "msgQLib.h"
#include "iosLib.h"
#include "errno.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
// ---------------------------------------------------------------
// SPI device state definitions
// ---------------------------------------------------------------
*/

#define SPIDEV_STATE_INIT 	 	0
#define SPIDEV_STATE_BUSY 	 	1
#define SPIDEV_STATE_IDLE		2

/*
// ---------------------------------------------------------------
// SPI control block state definitions
// ---------------------------------------------------------------
*/

#define SPICB_STATE_FREE 	 	0	/* unallocated control block */
#define SPICB_STATE_ERROR		1	/* error processing command block */
#define SPICB_STATE_QUEUE   	2	/* ready to process control block */
#define SPICB_STATE_COMPLETE	3	/* completed comand in control block */
#define SPICB_STATE_REPEAT		4	/* repeat command in control block */
#define SPICB_STATE_RUN			5	/* command is running */
#define SPICB_STATE_DELAY		6	/* delay command */
#define SPICB_STATE_ABORT		7	/* cancelled command */

/*
// ---------------------------------------------------------------
// SPI read/write operation modes.
// ---------------------------------------------------------------
*/

#define SPI_SYNC				0	/* immediate */
#define SPI_ASYNC_ISR			1	/* asynchronous isr notification */
#define SPI_ASYNC_TASK			2	/* asynchronous task notification */

/*
// ---------------------------------------------------------------
// SPI interrupt events.
// ---------------------------------------------------------------
*/

#define SPI_EVENT_TXE    	0x10    /* spi event Tx error        */
#define SPI_EVENT_BSY    	0x04    /* spi event Busy condition  */
#define SPI_EVENT_RXB    	0x01    /* spi event Buffer received */

/*
// ---------------------------------------------------------------
// SPI miscellanous defintions.
// ---------------------------------------------------------------
*/

#define SPI_MAX_ARGS     		6
#define SPI_MAX_MSGS     		10
#define SPI_MAX_CB				10
#define SPI_BUFFER_SIZE			1024

#define SPIDEBUG(x)				{ if (spiDebug) logMsg x ; }

#define SPI_ARG_PARM0	Arg[0]
#define SPI_ARG_PARM1	Arg[1]
#define SPI_ARG_PARM2	Arg[2]
#define SPI_ARG_PARM3	Arg[3]
#define SPI_ARG_PARM4	Arg[4]
#define SPI_ARG_PARM5	Arg[5]

#define SPI_ARG_TEMP0	Arg[0]
#define SPI_ARG_TEMP1	Arg[1]
#define SPI_ARG_TEMP2	Arg[2]
#define SPI_ARG_TEMP3	Arg[3]
#define SPI_ARG_TEMP4	Arg[4]
#define SPI_ARG_TEMP5	Arg[5]


/*
// ---------------------------------------------------------------
// Type definitions.
// ---------------------------------------------------------------
*/

/* spi internal message structure */
typedef struct {
	FUNCPTR Function;
	int Arg[SPI_MAX_ARGS];
} SPI_MSG;

/* spi statistics block structure */
typedef struct {
	int spiMsgsLost;	/* message lost counter */
	int oldMsgsLost;
	int newMsgsLost;
} SPI_STAT;

/* spi command block structure */
typedef struct {
	int Mode;
	int TxSize;
	int RxSize;
	char *TxBuf;
	char *RxBuf;
	int Arg[SPI_MAX_ARGS];
	FUNCPTR CsOff;		/* chip select off - interrupt time */
	FUNCPTR CsOn;		/* chip select on - interrupt time */
	FUNCPTR PostOp;		/* postprocessing - interrupt time */
	FUNCPTR PreOp;		/* preprocessing - interrupt time */
} SPI_CMD;

/* spi control block structure */
struct SPI_CB {
	int Id;
	int State;
	int Index;
	int Return;
	int Error;
	int Priority;
	int Count;
	int SyncMode;
	FUNCPTR NotifyOp;	/* notification operation - isr/task time */
	SEM_ID sem;
	struct SPI_CB *Next;
	SPI_CMD *Cmd;
};
typedef struct SPI_CB SPI_CB;

/* spi device header structure */
typedef struct {
	MSG_Q_ID mq;		/* daemon message queue id */
	int tid;			/* daemon task id */
	int State;			/* device state mask */
	int IsStalled;		/* interrupt routine stalled */
	int StalledIndex;	/* which command block stalled */
	int LastTick;		/* tick count for timeout mechanism */
	int Interval;		/* tick interval for timeout mechansim */
	SPI_CB *CBHead;		/* head of control block link list */
	SPI_CB *CBTail;		/* tail of control block link list */
	SPI_CB *RunCB;		/* run queue */
	SPI_CB *DelayCB;	/* delay queue */
	SEM_ID mutex;		/* mutual exclusion semaphore */
} SPI_HDR;


/*
// ---------------------------------------------------------------
// Function declarations.
// ---------------------------------------------------------------
*/

#if defined(__STDC__) || defined(__cplusplus)
extern char spiRxBuffer[];
extern char spiTxBuffer[];
extern int spiDebug;
extern int SpiMaxCB;
extern int spiOptions;
extern int spiPriority;
extern int spiStackSize;
extern int spiMsgTimeout;
extern int spiWdgTimeout;
extern SPI_CB SpiCB[];
extern SPI_HDR SpiHdr;
extern SPI_STAT SpiStat;

extern int spiAllocate(void);
extern int spiCancel(int id);
extern int spiError(int id);
extern int spiFree(int id);
extern int spiInit(void);
extern int spiSched(int id, SPI_CMD *cmd, int ncmds, int mode, FUNCPTR op);
extern int spiSync(int id, int timeout);
extern void spiDaemon();
extern void spiIntr(SPI_HDR *h);
extern void spiStart(void);
#else
extern char spiRxBuffer[];
extern char spiTxBuffer[];
extern int spiDebug;
extern int SpiMaxCB;
extern int spiOptions;
extern int spiPriority;
extern int spiStackSize;
extern int spiMsgTimeout;
extern int spiWdgTimeout;
extern SPI_CB SpiCB[];
extern SPI_HDR SpiHdr;
extern SPI_STAT SpiStat;

extern int spiAllocate();
extern int spiCancel();
extern int spiError();
extern int spiFree();
extern int spiInit();
extern int spiSched();
extern int spiSync();
extern void spiDaemon();
extern void spiIntr();
extern void spiStart();
#endif	/* __STDC__ */


#ifdef __cplusplus
}
#endif

#endif	/* SPILIB_H */
