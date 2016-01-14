/*
// ---------------------------------------------------------------
// File: spiLtc1598.c
//
// Module: SPI LTC1598 8-Channel, 12-Bit ADC device interface.
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
#include "MuxUtHw.h"
#include "spiLib.h"
#include "spiLtc1598.h"


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
// Function: spiLtc1598Init
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
spiLtc1598Init(void)
{
	/*
	// -----------------------------------------------------------
	// disable all bank chip selects
	// -----------------------------------------------------------
	*/

	*MUX360_SPICS_ADR = 0x1f;
}


/*
// ---------------------------------------------------------------
// Function: spiCsOnLtc1598
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
spiCsOnLtc1598(SPI_CB *cb)
{
	SPI_CMD *cmd = cb->Cmd + cb->Index;

	SPIDEBUG(("spiCsOnLtc1598: cs=%x\n", cmd->SPI_ARG_PARM0 & 0xff, 0, 0, 0, 0, 0));

	*MUX360_SPICS_ADR = (unsigned char) cmd->SPI_ARG_PARM0;
}


/*
// ---------------------------------------------------------------
// Function: spiCsOffLtc1598
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
spiCsOffLtc1598(SPI_CB *cb)
{
	*MUX360_SPICS_ADR = 0x1f;
}


/*
// ---------------------------------------------------------------
// Function: spiPostLtc1598ChannelSelect
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
spiPostLtc1598ChannelSelect(SPI_CB *cb)
{
	/*
	// -----------------------------------------------------------
	// determine if this command block is completed.
	// -----------------------------------------------------------
	*/

	cb->Index++;

	return (cb->Index < cb->Count) ?
		SPICB_STATE_RUN : SPICB_STATE_COMPLETE;
}


/*
// ---------------------------------------------------------------
// Function: spiPreLtc1598ChannelSelect
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
spiPreLtc1598ChannelSelect(SPI_CB *cb)
{
	SPI_CMD *cmd = cb->Cmd + cb->Index;

	cmd->TxSize = 1;
	cmd->RxSize = cmd->TxSize;

	/*
	// -----------------------------------------------------------
	// format channel select device command.
	// -----------------------------------------------------------
	*/

	cmd->TxBuf = spiTxBuffer;
	cmd->RxBuf = spiRxBuffer;

	cmd->TxBuf[0] = (0x08 | (cmd->SPI_ARG_PARM0 & 0x0f));

	return SPICB_STATE_RUN;
}


/*
// ---------------------------------------------------------------
// Function: spiPostLtc1598Read
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
spiPostLtc1598Read(SPI_CB *cb)
{
	SPI_CMD *cmd = cb->Cmd + cb->Index;
	short d;
	unsigned int *pi;

	/*
	// -----------------------------------------------------------
	// extract value from ADC response.
	// -----------------------------------------------------------
	*/

	((char *) &d)[0] = cmd->RxBuf[0];
	((char *) &d)[1] = cmd->RxBuf[1];

	pi = (unsigned int *) cmd->SPI_ARG_PARM1;
	*pi = (((unsigned int) d >> 1) & 0x0fff);

	SPIDEBUG(("spiPostLtc1598Read: d=%x pi=%x *pi=%x RxBuf[0]=%x RxBuf[1]=%x\n",
		(unsigned int) d, pi, (unsigned int) *pi,
		cmd->RxBuf[0] & 0xff, cmd->RxBuf[1] & 0xff, 0));

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
// Function: spiPreLtc1598Read
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
spiPreLtc1598Read(SPI_CB *cb)
{
	SPI_CMD *cmd = cb->Cmd + cb->Index;

	cmd->TxSize = 2;
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

	return SPICB_STATE_RUN;
}


/*
// ---------------------------------------------------------------
// Function: spiLtc1598Read
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
spiLtc1598Read(int ChipSelect, int Channel, int *Data)
{
	int id;
	int ret;
	SPI_CMD *pcmd;
	SPI_CMD cmd[2];

	/*
	// -----------------------------------------------------------
	// block 0 - format select channel command.
	// -----------------------------------------------------------
	*/

	pcmd = cmd + 0;
	pcmd->Mode = SPICB_MODE_LTC1598;
	pcmd->SPI_ARG_PARM0 = Channel;
	pcmd->CsOff = (FUNCPTR) 0;
	pcmd->CsOn = (FUNCPTR) 0;
	pcmd->PostOp = (FUNCPTR) spiPostLtc1598ChannelSelect;
	pcmd->PreOp = (FUNCPTR) spiPreLtc1598ChannelSelect;

	/*
	// -----------------------------------------------------------
	// block 1 - format read command.
	// -----------------------------------------------------------
	*/

	pcmd = cmd + 1;
	pcmd->Mode = SPICB_MODE_LTC1598;
	pcmd->SPI_ARG_PARM0 = (unsigned int) ChipSelect;
	pcmd->SPI_ARG_PARM1 = (unsigned int) Data;
	pcmd->CsOff = (FUNCPTR) spiCsOffLtc1598;
	pcmd->CsOn = (FUNCPTR) spiCsOnLtc1598;
	pcmd->PostOp = (FUNCPTR) spiPostLtc1598Read;
	pcmd->PreOp = (FUNCPTR) spiPreLtc1598Read;

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

	if ((ret = spiSync(id, -1)) == ERROR) {

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
