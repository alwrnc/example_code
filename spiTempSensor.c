/*
// ---------------------------------------------------------------
// File: spiTempSensor.c
//
// Module: SPI temperature sensor device interface.
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
#include "config.h"
#include "m68360.h"
#include "m68360UtHw.h"
#include "spiLib.h"
#include "spiTempSensor.h"


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
// Function: spiTempSensorInit
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
spiTempSensorInit(void)
{
	/* setup ambient temperature sensor chip select */
	*M360_CPM_PCPAR(M_ADRS) &= ~PC_SPI_TMPSEL;
			/* general purpose i/o */
	*M360_CPM_PCDIR(M_ADRS) |= PC_SPI_TMPSEL;
			/* active output */
	*M360_CPM_PCDAT(M_ADRS) |= PC_SPI_TMPSEL;
			/* negate chip select */
}


/*
// ---------------------------------------------------------------
// Function: spiCsOnTempSensor
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
spiCsOnTempSensor(void)
{
	extern int Reg16And();

	Reg16And(M360_CPM_PCDAT(M_ADRS), ~PC_SPI_TMPSEL);
}


/*
// ---------------------------------------------------------------
// Function: spiCsOffTempSensor
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
spiCsOffTempSensor(void)
{
	extern int Reg16Or();

	Reg16Or(M360_CPM_PCDAT(M_ADRS), PC_SPI_TMPSEL);
}


/*
// ---------------------------------------------------------------
// Function: spiPostTempSensorRead
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
spiPostTempSensorRead(SPI_CB *cb)
{
	SPI_CMD *cmd = cb->Cmd + cb->Index;
	short t;
	unsigned int *pi;

	/*
	// -----------------------------------------------------------
	// determine temperature (in celsius) from value read.
	// -----------------------------------------------------------
	*/

	((char *) &t)[0] = cmd->RxBuf[4];
	((char *) &t)[1] = cmd->RxBuf[5];

	pi = (unsigned int *) cmd->SPI_ARG_PARM0;
	*pi = (((unsigned int) t & 0x0fff) >> 3) - 130;

	SPIDEBUG(("spiPostTempSensorRead: t=%x pi=%x *pi=%d\n", (unsigned int) t,
		(unsigned int) pi, (unsigned int) *pi, 0, 0, 0));

	/*
	// -----------------------------------------------------------
	// determine if this command block is completed.
	// -----------------------------------------------------------
	*/

	cb->Index++;

	return (cb->Index < cb->Count) ?
		SPICB_STATE_QUEUE : SPICB_STATE_COMPLETE;
}


/*
// ---------------------------------------------------------------
// Function: spiPreTempSensorRead
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
spiPreTempSensorRead(SPI_CB *cb)
{
	SPI_CMD *cmd = cb->Cmd + cb->Index;

	cmd->TxSize = 8;
	cmd->RxSize = cmd->TxSize;

	/*
	// -----------------------------------------------------------
	// format read instruction
	// -----------------------------------------------------------
	*/

	cmd->TxBuf = spiTxBuffer;
	cmd->RxBuf = spiRxBuffer;

	cmd->TxBuf[0] = 0;
	cmd->TxBuf[1] = 0;
	cmd->TxBuf[2] = 0;
	cmd->TxBuf[3] = 0;
	cmd->TxBuf[4] = 0x90;
	cmd->TxBuf[5] = 0;
	cmd->TxBuf[6] = 0;
	cmd->TxBuf[7] = 0;

	return SPICB_STATE_RUN;
}


/*
// ---------------------------------------------------------------
// Function: spiTempSensorRead
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
spiTempSensorRead(int *piCelsius)
{
	int id;
	int ret;
	SPI_CMD *pcmd;
	SPI_CMD cmd[1];

	/*
	// -----------------------------------------------------------
	// format read command.
	// -----------------------------------------------------------
	*/

	pcmd = cmd + 0;
	pcmd->Mode = SPICB_MODE_TEMPSENSOR;
	pcmd->SPI_ARG_PARM0 = (unsigned int) piCelsius;
	pcmd->CsOff = (FUNCPTR) spiCsOffTempSensor;
	pcmd->CsOn = (FUNCPTR) spiCsOnTempSensor;
	pcmd->PostOp = (FUNCPTR) spiPostTempSensorRead;
	pcmd->PreOp = (FUNCPTR) spiPreTempSensorRead;

	/*
	// -----------------------------------------------------------
	// allocate control block.
	// -----------------------------------------------------------
	*/

	if ((id = spiAllocate()) == ERROR)
		return ERROR;

	/*
	// -----------------------------------------------------------
	// schedule command block(s).
	// -----------------------------------------------------------
	*/

	if (spiSched(id,
		cmd, (sizeof(cmd)/sizeof(cmd)[0]), SPI_SYNC, 0L) == ERROR)
		return ERROR;

	/*
	// -----------------------------------------------------------
	// wait for comand(s) to be completed.
	// -----------------------------------------------------------
	*/

	if ((ret = spiSync(id, WAIT_FOREVER)) == ERROR) {

		/*
		// -------------------------------------------------------
		// cancel command if not already completed.
		// -------------------------------------------------------
		*/

		spiCancel(id);
	}

	/*
	// -----------------------------------------------------------
	// free control block.
	// -----------------------------------------------------------
	*/

	spiFree(id);

	/*
	// -----------------------------------------------------------
	// return status of read.
	// -----------------------------------------------------------
	*/

	return ret;
}
