/*
// ---------------------------------------------------------------
// File: spiLib.c
//
// Module: SPI library.
//
// Description:	This module provides support for basic SPI
//		(serial	peripheral interface) communication on the
//		M68360 SPI.
//
// Operation: Before the driver can be used, it must be
//		initialized	by calling spiInit().  This routine should
//		be called exactly once, before any spi calls are used.
//		Normally, it is called from usrRoot() in usrConfig.c.
//
//		The spiInit() routine performs the following actions:
//			- initializes library data structures.
//			- creates a message queue.
//			- spawns a SPI daemon, which handles jobs in the
//				message queue.
//			- connects the SPI interrupt handler.
//			- enables the SPI interrupt.
//
//		The SPI interrupt handler performs the following actions:
//			- ...
//
// ToDo:
//
//
// Version:
//
// History:
//	AL     Create
// ---------------------------------------------------------------
*/


/*
// ---------------------------------------------------------------
// Header files.
// ---------------------------------------------------------------
*/

#include "vxWorks.h"
#include "taskLib.h"
#include "msgQLib.h"
#include "iv.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "semLib.h"
#include "ioLib.h"
#include "iosLib.h"
#include "intLib.h"
#include "logLib.h"
#include "spiLib.h"
#include "config.h"
#include "m68360.h"


/*
// ---------------------------------------------------------------
// Miscellanous definitions.
// ---------------------------------------------------------------
*/

#define CB_CLEAR(cb)	{ \
	(cb)->Index = 0; \
	(cb)->Return = 0; \
	(cb)->Error = 0; \
	(cb)->Priority = 0; \
	(cb)->Count = 0; \
	(cb)->SyncMode = 0; \
	(cb)->Next = 0; \
	(cb)->Cmd = 0; \
}

#define CB_ENQUEUE(h, cb)	{ \
	if ((h)->CBTail) { \
		(h)->CBTail->Next = (cb); \
		(h)->CBTail = (cb); \
	} else { \
		(h)->CBHead = (h)->CBTail = (cb); \
	} \
	(cb)->Next = 0; \
}

#define CB_SCHED(h)	{ \
	if (((h)->RunCB = (h)->CBHead) != 0L) { \
		(h)->CBHead = (h)->CBHead->Next; \
		(h)->RunCB->Next = 0; \
		if ((h)->RunCB == (h)->CBTail) \
			(h)->CBTail = (h)->CBHead; \
	} \
}


/*
// ---------------------------------------------------------------
// External declarations.
// ---------------------------------------------------------------
*/


/*
// ---------------------------------------------------------------
// Local variables.
// ---------------------------------------------------------------
*/


/*
// ---------------------------------------------------------------
// Function: spiInit
//
// Purpose: Initialize the SPI driver.
//
// Description: This routine creates two character mode VxWorks
//		drivers, it needs to be called before any other functions
//		in this file.
//
//		WARNING: Before this routine is called, the chip select
//		on all devices attached to the SPI bus must be disabled.
//
// Architecture:
//
// Relationship:
//
// Returns:
//
// Exception:
//
// Concurrency:
//
// ---------------------------------------------------------------
*/
int
spiInit(void)
{
	int i;
	int lvl;

	/*
	// -----------------------------------------------------------
	// initialize SPI control blocks
	// -----------------------------------------------------------
	*/

	for (i = 0; i < SpiMaxCB; ++i) {
		SpiCB[i].Id = i;
		SpiCB[i].State = SPICB_STATE_FREE;
		SpiCB[i].Index = 0;
		SpiCB[i].Return = 0;
		SpiCB[i].Error = 0;
		SpiCB[i].Priority = 0;
		SpiCB[i].Count = 0;
		SpiCB[i].Next = 0L;
		SpiCB[i].Cmd = 0;
		SpiCB[i].sem = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
		if (SpiCB[i].sem == NULL)
			return ERROR;
	}

	/*
	// -----------------------------------------------------------
	// initialize SPI device headers.
	// -----------------------------------------------------------
	*/

	SpiHdr.State = SPIDEV_STATE_IDLE;
	SpiHdr.RunCB = 0L;
	SpiHdr.CBHead = 0L;
	SpiHdr.CBTail = 0L;

	SpiHdr.mq = msgQCreate(SPI_MAX_MSGS, sizeof(SPI_MSG), MSG_Q_FIFO);
	if (SpiHdr.mq == NULL)
		return ERROR;

#if 0
	SpiHdr.tid = taskSpawn("spiDaemon", spiPriority, spiOptions, spiStackSize,
		(FUNCPTR) spiDaemon, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	if (SpiHdr.tid == ERROR)
		return ERROR;
#endif

	SpiHdr.mutex =
		semMCreate(SEM_Q_PRIORITY|SEM_DELETE_SAFE|SEM_INVERSION_SAFE);
	if (SpiHdr.mutex == NULL)
		return ERROR;

	/*
	// -----------------------------------------------------------
	// initialize SPI device registers
	// -----------------------------------------------------------
	*/

	lvl = intLock();

	/*
	// -----------------------------------------------------------
	// setup port b parallel i/o for spi
	// -----------------------------------------------------------
	*/

	*M360_CPM_PBPAR(M_ADRS) |= 0x0000000E;
			/* bits 1,2,3 must be 1 */
	*M360_CPM_PBODR(M_ADRS) &= 0xFFFFFFF1;
			/* bits 1,2,3 must be 0 */
	*M360_CPM_PBDIR(M_ADRS) |= 0x0000000E;
			/* bits 1,2,3 must be 1 */

	/*
	// -----------------------------------------------------------
	// set up general SPI parameters
	// -----------------------------------------------------------
	*/

	*M360_SPI_M_RXBASE(M_ADRS) = 0x600;
			/* receive bd at 0x10000600 */
	*M360_SPI_M_TXBASE(M_ADRS) = 0x608;
			/* transmit bd at 0x10000608 */
	*M360_SPI_M_RFCR(M_ADRS) = 0x18;
			/* FC = normal, Big-endian transfer */
	*M360_SPI_M_TFCR(M_ADRS) = 0x18;
			/* FC = normal; Big-endian transfer */
	*M360_CPM_CR(M_ADRS) = 0x0051;
			/* execute the INIT RXTX */

	/*
	// -----------------------------------------------------------
	// initialize SPI interrupt registers
	// -----------------------------------------------------------
	*/

	*M360_CPM_SPIE(M_ADRS) = 0xFF;
	*M360_CPM_SPIM(M_ADRS) = SPI_EVENT_TXE|SPI_EVENT_BSY|SPI_EVENT_RXB;
	*M360_CPM_CIMR(M_ADRS) |= CPIC_CIXR_SPI;

	/*
	// -----------------------------------------------------------
	// attach interrupt service routine
	// -----------------------------------------------------------
	*/

	intConnect(INUM_TO_IVEC(INT_VEC_SPI(M_ADRS)),
		(VOIDFUNCPTR) spiIntr, (int) &SpiHdr);

	/*
	// -----------------------------------------------------------
	// SPI hardware initialization is complete.
	// -----------------------------------------------------------
	*/

	intUnlock(lvl);

	return OK;
}


/*
// ---------------------------------------------------------------
// Function: spiDefer
//
// Purpose: request a task-level function call from interrupt level
//
// Description: This routine allows interrupt level code to request
//		a function call to be made by spidaemon at task-level.
//
// Architecture:
//
// Relationship:
//
// Returns:
//
// Exception:
//
// Concurrency:
//
// ---------------------------------------------------------------
*/
int
spiDefer(FUNCPTR Function, int Arg1, int Arg2)
{
	SPI_MSG m;

	m.Function = Function;
	m.Arg[0] = Arg1;
	m.Arg[1] = Arg2;

	if (msgQSend(SpiHdr.mq, (char *) &m, sizeof (m),
		INT_CONTEXT() ? NO_WAIT : WAIT_FOREVER, MSG_PRI_NORMAL) != OK) {

		++SpiStat.spiMsgsLost;
		return ERROR;
	}

	return OK;
}


/*
// ---------------------------------------------------------------
// Function: spiDaemon
//
// Purpose: handle task-level SPI events
//
// Description: This routine is spawned as a task by spiInit()
//		to perform functions that cannot be performed at interrupt
//		or trap level.  It has a priority of 0.  Do not suspend,
//		delete, or change the priority of this task.
//
//		NOTE: This routine must be spawned before the interrupt
//		routine is enabled.
//
// Architecture:
//
// Relationship:
//
// Returns:
//
// Exception:
//
// Concurrency:
//
// ---------------------------------------------------------------
*/
void
spiDaemon(void)
{
	int iv;
	SPI_MSG m;
	extern int tickGet();

	SpiHdr.LastTick = tickGet();
	SpiHdr.Interval = spiWdgTimeout;

	SpiStat.oldMsgsLost = 0;

	for (;;) {

		/*
		// -------------------------------------------------------
		// wait for a message to arrive or timeout.
		// -------------------------------------------------------
		*/

		if (msgQReceive(SpiHdr.mq,
			(char *) &m, sizeof (m), spiMsgTimeout) != sizeof (m)) {

			SPIDEBUG(("spiDaemon: timed out, status = %#x\n",
				errno, 0, 0, 0, 0, 0));

			printf("DAEMON---TIMED OUT\n");

		} else {

			printf("DAEMON---RECEIVED MESSAGE\n");

			(*m.Function)(m.Arg[0], m.Arg[1]);

			/*
			// ---------------------------------------------------
			// check to see if interrupt lost any more calls.
			// ---------------------------------------------------
			*/

			SpiStat.newMsgsLost = SpiStat.spiMsgsLost;

			if (SpiStat.newMsgsLost != SpiStat.oldMsgsLost) {

				SPIDEBUG(("spiDaemon: lost %d msgs from interrupt.\n",
					SpiStat.newMsgsLost - SpiStat.oldMsgsLost,
					0, 0, 0, 0, 0));

				SpiStat.oldMsgsLost = SpiStat.newMsgsLost;
			}
		}

		/*
		// -------------------------------------------------------
		// watchdog for stalled interrupts on the SPI bus.
		// -------------------------------------------------------
		*/

		if (abs(tickGet() - SpiHdr.LastTick) > SpiHdr.Interval) {

			SPIDEBUG(("spiDaemon: watchdog, tick timeout = %d\n",
				SpiHdr.LastTick, 0, 0, 0, 0, 0));

			/*
			// ---------------------------------------------------
			// check if the interrupt routine is stalled.
			// ---------------------------------------------------
			*/

			iv = intLock();

			if (SpiHdr.State == SPIDEV_STATE_BUSY) {

				if (SpiHdr.IsStalled == FALSE) {

					/*
					// -------------------------------------------
					// interrupt is apparently working, re-arm.
					// -------------------------------------------
					*/

					SpiHdr.IsStalled = TRUE;
					SpiHdr.StalledIndex = -1;

				} else {

					/*
					// -------------------------------------------
					// interrupt is not working.
					// -------------------------------------------
					*/

					if (SpiHdr.StalledIndex == -1) {

						/*
						// ---------------------------------------
						// first try lowering the speed.
						// ---------------------------------------
						*/

						SpiHdr.StalledIndex = SpiHdr.RunCB->Index;

						*M360_CPM_SPMODE(M_ADRS) =
							(*M360_CPM_SPMODE(M_ADRS) & 0xfff0) | 0x0800;

					} else {

						/*
						// ---------------------------------------
						// lowering the speed didn't help.
						// SPI command was a complete failure.
						// ---------------------------------------
						*/

						SpiHdr.IsStalled = FALSE;
						SpiHdr.StalledIndex = -1;

						intUnlock(iv);

						spiCancel(SpiHdr.RunCB->Id);

						iv = intLock();
					}
				}

			} else {

				SpiHdr.IsStalled = TRUE;
			}

			intUnlock(iv);

			/*
			// ---------------------------------------------------
			// setup for next timeout.
			// ---------------------------------------------------
			*/

			SpiHdr.LastTick = tickGet();
		}
	}
}


/*
// ---------------------------------------------------------------
// Function: spiAllocate
//
// Purpose:
//
// Description:
//
// Architecture:
//
// Relationship:
//
// Returns:
//
// Exception:
//
// Concurrency:
//
// ---------------------------------------------------------------
*/
int
spiAllocate(void)
{
	register SPI_CB *cb = SpiCB;
	int i;

	/*
	// -----------------------------------------------------------
	// acquire exclusive access to spi library routine.
	// -----------------------------------------------------------
	*/

	semTake(SpiHdr.mutex, WAIT_FOREVER);

	/*
	// -----------------------------------------------------------
	// search for available control block.
	// -----------------------------------------------------------
	*/

	for (cb = SpiCB, i = 0; i < SpiMaxCB; ++i, ++cb) {
		if (cb->State == SPICB_STATE_FREE) {
			CB_CLEAR(cb);
			break;
		}
	}

	/*
	// -----------------------------------------------------------
	// release spi library semaphore.
	// -----------------------------------------------------------
	*/

	semGive(SpiHdr.mutex);

	return (i >= SpiMaxCB) ? -1 : i;
}


/*
// ---------------------------------------------------------------
// Function: spiFree
//
// Purpose:
//
// Description:
//
// Architecture:
//
// Relationship:
//
// Returns:
//
// Exception:
//
// Concurrency:
//
// ---------------------------------------------------------------
*/
int
spiFree(int id)
{
	SPI_CB *cb;

	if ((id < 0) || (id >= SpiMaxCB))
		return ERROR;

	/*
	// -----------------------------------------------------------
	// acquire exclusive access to spi library routine.
	// -----------------------------------------------------------
	*/

	semTake(SpiHdr.mutex, WAIT_FOREVER);

	/*
	// -----------------------------------------------------------
	// free control block.
	// -----------------------------------------------------------
	*/

	cb = SpiCB + id;
	cb->State = SPICB_STATE_FREE;
	CB_CLEAR(cb);

	/*
	// -----------------------------------------------------------
	// release spi library semaphore.
	// -----------------------------------------------------------
	*/

	semGive(SpiHdr.mutex);

	return OK;
}


/*
// ---------------------------------------------------------------
// Function: spiSched
//
// Purpose:
//
// Description:
//
// Architecture:
//
// Relationship: This routine can only be called at task level.
//
// Returns:
//
// Exception:
//
// Concurrency:
//
// ---------------------------------------------------------------
*/
int
spiSched(int id, SPI_CMD *cmd, int ncmds, int mode, FUNCPTR op)
{
	int iv;
	SPI_CB *cb;

	SPIDEBUG(("spiSched: id=%d ncmds=%d\n", id, ncmds, 0, 0, 0, 0));

	if ((id < 0) || (id >= SpiMaxCB))
		return ERROR;

	cb = SpiCB + id;
	cb->Cmd = cmd;
	cb->Count = ncmds;
	cb->Index = 0;
	cb->Return = 0;
	cb->Error = 0;
	cb->Next = 0;
	cb->State = SPICB_STATE_QUEUE;
	cb->SyncMode = mode;
	cb->NotifyOp = (mode != SPI_SYNC) ? op : 0;

	/*
	// -----------------------------------------------------------
	// clear any pending condition on control block.
	// -----------------------------------------------------------
	*/

	semTake(cb->sem, NO_WAIT);

	/*
	// -----------------------------------------------------------
	// acquire exclusive access to spi library routine.
	// -----------------------------------------------------------
	*/

	semTake(SpiHdr.mutex, WAIT_FOREVER);

	/*
	// -----------------------------------------------------------
	// disable interrupts.
	// -----------------------------------------------------------
	*/

	iv = intLock();

	CB_ENQUEUE(&SpiHdr, cb);

	if (SpiHdr.State == SPIDEV_STATE_IDLE) {

		SpiHdr.State = SPIDEV_STATE_BUSY;

		CB_SCHED(&SpiHdr);

		spiStart();
	}

	/*
	// -----------------------------------------------------------
	// enable interrupts.
	// -----------------------------------------------------------
	*/

	intUnlock(iv);

	/*
	// -----------------------------------------------------------
	// release spi library semaphore.
	// -----------------------------------------------------------
	*/

	semGive(SpiHdr.mutex);

	return OK;
}


/*
// ---------------------------------------------------------------
// Function: spiCancel
//
// Purpose:
//
// Description:
//
// Architecture:
//
// Relationship:
//
// Returns:
//
// Exception:
//
// Concurrency:
//
// ---------------------------------------------------------------
*/
int
spiCancel(int id)
{
	int iv;
	SPI_CB *cb;
	SPI_CB *CBPrev;

	SPIDEBUG(("spiCancel:id=%d\n", id, 0, 0, 0, 0, 0));

	if ((id < 0) || (id >= SpiMaxCB))
		return ERROR;

	cb = SpiCB + id;

	/*
	// -----------------------------------------------------------
	// acquire spi library semaphore.
	// -----------------------------------------------------------
	*/

	semTake(SpiHdr.mutex, WAIT_FOREVER);

	/*
	// -----------------------------------------------------------
	// disable interrups.
	// -----------------------------------------------------------
	*/

	iv = intLock();

	switch (cb->State) {

	case SPICB_STATE_REPEAT:
	case SPICB_STATE_RUN:

		SpiHdr.RunCB = 0;

		/*
		// ---------------------------------------------------
		// turn off chip select.
		// ---------------------------------------------------
		*/

		if ((cb->Index < cb->Count) && (cb->Cmd+cb->Index)->CsOff)
			(*(cb->Cmd+cb->Index)->CsOff)(cb);

		/*
		// ---------------------------------------------------
		// wait until the previous command is completed.
		// ---------------------------------------------------
		*/

		while (*M360_CPM_CR(M_ADRS) & 0x1);

		/*
		// ---------------------------------------------------
		// close RXB.
		// ---------------------------------------------------
		*/

		*M360_CPM_CR(M_ADRS) = 0x0751;

		/*
		// ---------------------------------------------------
		// mark state of control block
		// ---------------------------------------------------
		*/

		cb->Next = 0;
		cb->Error = ECANCELED;
		cb->Return = ERROR;
		cb->State = SPICB_STATE_ABORT;

		/*
		// ---------------------------------------------------
		// prepare the semaphore for next time.
		// ---------------------------------------------------
		*/

		/* should we do a semGive() here? */

		/*
		// ---------------------------------------------------
		// may have to kick start SPI again ...
		// ---------------------------------------------------
		*/

		CB_SCHED(&SpiHdr);

		if (SpiHdr.RunCB) {

			spiStart();

		} else {

			SpiHdr.State = SPIDEV_STATE_IDLE;
		}

		break;

	case SPICB_STATE_QUEUE:

		/*
		// ---------------------------------------------------
		// remove control block from queue.
		// ---------------------------------------------------
		*/

		if (SpiHdr.CBHead == cb) {

			SpiHdr.CBHead = cb->Next;

			if (SpiHdr.CBTail == cb)
				SpiHdr.CBTail = cb->Next;

		} else {

			for (CBPrev = SpiHdr.CBHead;
				 CBPrev->Next && CBPrev->Next != cb;
				 CBPrev = CBPrev->Next) ;

			if (CBPrev->Next == cb)
				CBPrev->Next = cb->Next;

			if (SpiHdr.CBTail == cb)
				SpiHdr.CBTail = CBPrev;
		}

		cb->Next = 0;
		cb->Error = EINTR;
		cb->Return = -1;
		cb->State = SPICB_STATE_COMPLETE;

		break;
	}

	/*
	// -----------------------------------------------------------
	// enable interrupts.
	// -----------------------------------------------------------
	*/

	intUnlock(iv);

	/*
	// -----------------------------------------------------------
	// release spi library semaphore.
	// -----------------------------------------------------------
	*/

	semGive(SpiHdr.mutex);

	return OK;
}


/*
// ---------------------------------------------------------------
// Function: spiSync
//
// Purpose:
//
// Description:
//
// Architecture:
//
// Relationship:
//
// Returns:
//
// Exception:
//
// Concurrency:
//
// ---------------------------------------------------------------
*/
int
spiSync(int id, int timeout)
{
	SPI_CB *cb = SpiCB + id;

	SPIDEBUG(("spiSync enter timeout=%d: \n", timeout, 0, 0, 0, 0, 0));

	semTake(cb->sem, timeout);

	SPIDEBUG(("spiSync exit: \n", 0, 0, 0, 0, 0, 0));

	if (cb->Return)
		printf("SyncError = %d\n", cb->Error);

	return cb->Error;
}


/*
// ---------------------------------------------------------------
// Function: spiError
//
// Purpose:
//
// Description:
//
// Architecture:
//
// Relationship:
//
// Returns:
//
// Exception:
//
// Concurrency:
//
// ---------------------------------------------------------------
*/
int
spiError(int id)
{
	SPI_CB *cb;

	if ((id < 0) || (id >= SpiMaxCB))
		return ENOENT;

	cb = SpiCB + id;

	return cb->Error;
}


/*
// ---------------------------------------------------------------
// Function: spiStart
//
// Purpose:
//
// Description:
//
// Architecture:
//
// Relationship:
//
// Returns:
//
// Exception:
//
// Concurrency:
//
// ---------------------------------------------------------------
*/
void
spiStart(void)
{
	SCC_BUF *pBd;
	SPI_CB *cb = SpiHdr.RunCB;
	SPI_CMD *cmd = cb->Cmd + cb->Index;

	SPIDEBUG(("spiStart:\n", 0, 0, 0, 0, 0, 0));

	/*
	// -------------------------------------------------------
	// assert chip select
	// -------------------------------------------------------
	*/

	if (cmd->CsOn)
		(*cmd->CsOn)(cb);

	/*
	// -------------------------------------------------------
	// run preprocessing routine
	// -------------------------------------------------------
	*/

	if (cmd->PreOp)
		cb->State = (*cmd->PreOp)(cb);
	else
		cb->State = SPICB_STATE_QUEUE;

	switch (cb->State) {

	case SPICB_STATE_ERROR:
		break;

	case SPICB_STATE_QUEUE:
		break;

	case SPICB_STATE_COMPLETE:
		break;

	case SPICB_STATE_REPEAT:
		break;
	}

	/*
	// -------------------------------------------------------
	// control block is now in the run state.
	// -------------------------------------------------------
	*/

	cb->State = SPICB_STATE_RUN;

	/*
	// -------------------------------------------------------
	// prepare to transmit.
	// -------------------------------------------------------
	*/

	*M360_CPM_SPMODE(M_ADRS) = cmd->Mode;
			/* set spmode */
	*M360_SPI_M_MRBLR(M_ADRS) = cmd->RxSize;
			/* set receive buffer length */
	*M360_CPM_CR(M_ADRS) = 0x0051;
			/* execute init rx & tx parameters */

	/*
	// -------------------------------------------------------
	// setup receive buffer descriptor.
	// -------------------------------------------------------
	*/

	pBd = (SCC_BUF *) (M_ADRS + *M360_SPI_M_RXBASE(M_ADRS));
	pBd->dataLength = 0;
	pBd->dataPointer = cmd->RxBuf;
	pBd->statusMode = 0xA000;
	pBd->statusMode = 0xB000;

	/*
	// -------------------------------------------------------
	// setup transmit buffer descriptor.
	// -------------------------------------------------------
	*/

	pBd = (SCC_BUF *) (M_ADRS + *M360_SPI_M_TXBASE(M_ADRS));
	pBd->dataLength = cmd->TxSize;
	pBd->dataPointer = cmd->TxBuf;
	pBd->statusMode = 0xA800;

	/*
	// -------------------------------------------------------
	// initialize SPI interrupt masks
	// -------------------------------------------------------
	*/

	*M360_CPM_SPIE(M_ADRS) = 0xFF;
			/* clear spi event register */
	*M360_CPM_SPIM(M_ADRS) = SPI_EVENT_TXE|SPI_EVENT_BSY|SPI_EVENT_RXB;
			/* enable TXE, BSY and RXB interrupts */

	/*
	// -------------------------------------------------------
	// start transmit.
	// -------------------------------------------------------
	*/

	*M360_CPM_SPCOM(M_ADRS) = 0x80;
	return;
}


/*
// ---------------------------------------------------------------
// Function: spiIntr
//
// Purpose: SPI interrupt handler
//
// Description: This routine is interrupt handler for SPI core.
//
// Architecture:
//
// Relationship:
//
// Returns:
//
// Exception:
//
// Concurrency:
//
// ---------------------------------------------------------------
*/
void
spiIntr(SPI_HDR *h)
{
	int spie;
	SPI_CB *cb;
	SPI_CMD *cmd;
	SCC_BUF *pBd;

	/*
	// -----------------------------------------------------------
	// read SPI event register.
	// -----------------------------------------------------------
	*/

	spie = *M360_CPM_SPIE(M_ADRS) & 0xff;
			/* read interrupt events */
	*M360_CPM_SPIE(M_ADRS) = spie;
			/* clear corresponding events */

	/*
	SPIDEBUG(("spiIntr: spie=%02x\n", spie, 0, 0, 0, 0, 0));
	*/

	/*
	// -----------------------------------------------------------
	// handle SPI events.
	// -----------------------------------------------------------
	*/

#if 0
	if ((spie & SPI_EVENT_TXE) || (spie & SPI_EVENT_BSY)) {

		SPIDEBUG(("spiIntr: txe and/or bsy\n", 0, 0, 0, 0, 0, 0));

		/*
		// -------------------------------------------------------
		// transmit underrun or receive overrun -
		// both which should never happen since I don't enable
		// them.
		// -------------------------------------------------------
		*/

		if (h->RunCB) {

			/*
			// ---------------------------------------------------
			// remove current control block from the run queue.
			// ---------------------------------------------------
			*/

			cb = h->RunCB;
			h->RunCB = 0L;

			/*
			// ---------------------------------------------------
			// indicate an error occurred.
			// ---------------------------------------------------
			*/

			cb->Error = EIO;
			cb->Return = ERROR;
			cb->State = SPICB_STATE_ERROR;

			/*
			// ---------------------------------------------------
			// notify upper layer.
			// ---------------------------------------------------
			*/

			switch (cb->SyncMode) {
			case SPI_SYNC:
				semGive(cb->sem);
				break;

			case SPI_ASYNC_ISR:
				if (cb->NotifyOp)
					(*cb->NotifyOp)(cb);
				break;

			case SPI_ASYNC_TASK:
				if (cb->NotifyOp)
					spiDefer(cb->NotifyOp, cb->Id, cb->Index);
				break;
			}
		}
	}
#endif

	if (spie & SPI_EVENT_RXB) {

		/*
		// -------------------------------------------------------
		// SPI transaction completed successfully.
		// -------------------------------------------------------
		*/

		/*
		SPIDEBUG(("spiIntr: rxb - complete\n", 0, 0, 0, 0, 0, 0));
		*/

		SpiHdr.IsStalled = FALSE;

		/*
		// -------------------------------------------------------
		// handle receive completion event.
		// -------------------------------------------------------
		*/

		if (h->RunCB) {

			cb = h->RunCB;

			cmd = cb->Cmd + cb->Index;

			/*
			// ---------------------------------------------------
			// negate chip select.
			// ---------------------------------------------------
			*/

			if (cmd->CsOff)
				(*cmd->CsOff)(cb);

			/*
			// ---------------------------------------------------
			// run postprocessing routine.
			// ---------------------------------------------------
			*/

			if (cmd->PostOp)
				cb->State = (*cmd->PostOp)(cb);
			else
				cb->State = SPICB_STATE_COMPLETE;

			/*
			// ---------------------------------------------------
			// depending on the control block state,
			// prepare next command.
			// ---------------------------------------------------
			*/

			switch (cb->State) {

			case SPICB_STATE_ERROR:

				/*
				SPIDEBUG(("spiIntr: error\n", 0, 0, 0, 0, 0, 0));
				*/

				/*
				// -----------------------------------------------
				// the current control block has an error,
				// remove it from the list and get the next
				// control block in line to run.
				// -----------------------------------------------
				*/

				h->RunCB = 0L;

				/*
				// -----------------------------------------------
				// notify upper layer that command was completed.
				// -----------------------------------------------
				*/

				switch (cb->SyncMode) {
				case SPI_SYNC:
					semGive(cb->sem);
					break;

				case SPI_ASYNC_ISR:
					if (cb->NotifyOp)
						(*cb->NotifyOp)(cb);
					break;

				case SPI_ASYNC_TASK:
					if (cb->NotifyOp)
						spiDefer(cb->NotifyOp, cb->Id, cb->Index);
					break;
				}

				break;

			case SPICB_STATE_QUEUE:

				/*
				SPIDEBUG(("spiIntr: ready cb=%x\n", cb, 0, 0, 0, 0, 0));
				*/

				/*
				// -----------------------------------------------
				// the current control block has completed but
				// wants to remain in the run queue, place the
				// current control block at the end of the list
				// and get the next control block in line to run.
				// -----------------------------------------------
				*/

				h->RunCB = 0L;

				CB_ENQUEUE(h, cb);

				break;

			case SPICB_STATE_COMPLETE:

				/*
				SPIDEBUG(("spiIntr: complete\n", 0, 0, 0, 0, 0, 0));
				*/

				/*
				// -----------------------------------------------
				// the current control block has completed,
				// remove it from the list and get the next
				// control block in line to run.
				// -----------------------------------------------
				*/

				h->RunCB = 0L;

				/*
				// -----------------------------------------------
				// notify higher level that command was completed.
				// -----------------------------------------------
				*/

				switch (cb->SyncMode) {
				case SPI_SYNC:
					semGive(cb->sem);
					break;

				case SPI_ASYNC_ISR:
					if (cb->NotifyOp)
						(*cb->NotifyOp)(cb);
					break;

				case SPI_ASYNC_TASK:
					if (cb->NotifyOp)
						spiDefer(cb->NotifyOp, cb->Id, cb->Index);
					break;
				}

				break;

			case SPICB_STATE_REPEAT:
			case SPICB_STATE_RUN:

				/*
				SPIDEBUG(("spiIntr: repeat\n", 0, 0, 0, 0, 0, 0));
				*/

				/*
				// -----------------------------------------------
				// the current control block wants to run again
				// -- leave the control block on the run queue.
				// -----------------------------------------------
				*/

				break;
			}
		}
	}

	/*
	// -----------------------------------------------------------
	// schedule the next command block.
	// -----------------------------------------------------------
	*/

	if (h->RunCB == 0) {

		CB_SCHED(h);

		/*
		SPIDEBUG(("spiIntr: sched h=%x h->RunCB=%x\n",
			h, h->RunCB, 0, 0, 0, 0));
		*/
	}

	/*
	// -----------------------------------------------------------
	// if possible, transmit next command.
	// -----------------------------------------------------------
	*/

	if (h->RunCB == 0) {

		/*
		SPIDEBUG(("spiIntr: idle\n", 0, 0, 0, 0, 0, 0));
		*/

		SpiHdr.State = SPIDEV_STATE_IDLE;

	} else {

		/*
		SPIDEBUG(("spiIntr: busy\n", 0, 0, 0, 0, 0, 0));
		*/

		SpiHdr.State = SPIDEV_STATE_BUSY;

		cb = h->RunCB;

		cmd = cb->Cmd + cb->Index;

		/*
		// -------------------------------------------------------
		// assert chip select.
		// -------------------------------------------------------
		*/

		if (cmd->CsOn)
			(*cmd->CsOn)(cb);

		/*
		// -------------------------------------------------------
		// run preprocessing routine.
		// -------------------------------------------------------
		*/

		if (cmd->PreOp)
			cb->State = (*cmd->PreOp)(cb);
		else
			cb->State = SPICB_STATE_QUEUE;

		switch (cb->State) {

		case SPICB_STATE_ERROR:
			break;

		case SPICB_STATE_QUEUE:
			break;

		case SPICB_STATE_COMPLETE:
			break;

		case SPICB_STATE_REPEAT:
			break;
		}

		/*
		// ---------------------------------------------------
		// control block is now in the run state.
		// ---------------------------------------------------
		*/

		cb->State = SPICB_STATE_RUN;

		/*
		// -------------------------------------------------------
		// prepare to transmit.
		// -------------------------------------------------------
		*/

		*M360_CPM_SPMODE(M_ADRS) = cmd->Mode;
				/* set spmode */
		*M360_SPI_M_MRBLR(M_ADRS) = cmd->RxSize;
				/* set receive buffer length */
		*M360_CPM_CR(M_ADRS) = 0x0051;
				/* execute init rx & tx parameters */

		/*
		// -------------------------------------------------------
		// setup receive buffer descriptor.
		// -------------------------------------------------------
		*/

		pBd = (SCC_BUF *) (M_ADRS + *M360_SPI_M_RXBASE(M_ADRS));
		pBd->dataLength = 0;
		pBd->dataPointer = cmd->RxBuf;
		pBd->statusMode = 0xB000;

		/*
		// -------------------------------------------------------
		// setup transmit buffer descriptor.
		// -------------------------------------------------------
		*/

		pBd = (SCC_BUF *) (M_ADRS + *M360_SPI_M_TXBASE(M_ADRS));
		pBd->dataLength = cmd->TxSize;
		pBd->dataPointer = cmd->TxBuf;
		pBd->statusMode = 0xA800;

		/*
		// -------------------------------------------------------
		// start transmit.
		// -------------------------------------------------------
		*/

		*M360_CPM_SPCOM(M_ADRS) = 0x80;
	}

	*M360_CPM_CISR(M_ADRS) |= CPIC_CIXR_SPI;

	return;
}
