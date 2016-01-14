#define _ASMLANGUAGE
#include "vxWorks.h"
#include "sysLib.h"
#include "asm.h"


    .globl  _Reg16And
    .globl  _Reg16Or
    .globl  _Reg32And
    .globl  _Reg32Or



/*
void Reg16And(unsigned short *addr, unsigned short data)
     *addr &= data
*/
_Reg16And:                 /*   16 bit register AND process    */

     link a6, #0
     moveml d3/a3, a7@-
     movel a6@(ARG1), a3   /*   get address in a3    */
     movel a6@(ARG2), d3   /*   get data in d3       */
     andw d3,a3@           /*   read & write         */
     moveml a7@+, d3/a3
     unlk a6
     rts


/*
void Reg16Or(unsigned short *addr, unsigned short data)
     *addr |= data
*/
_Reg16Or:                  /*   16 bit register OR process    */

     link a6, #0
     moveml d3/a3, a7@-
     movel a6@(ARG1), a3   /*   get address in a3    */
     movel a6@(ARG2), d3   /*   get data in d3       */
     orw d3,a3@            /*   read & write         */
     moveml a7@+, d3/a3
     unlk a6
     rts



/*
void Reg132And(unsigned int *addr, unsigned int data)
     *addr &= data
*/
_Reg32And:                 /*   32 bit register AND process    */

     link a6, #0
     moveml d3/a3, a7@-
     movel a6@(ARG1), a3   /*   get address in a3    */
     movel a6@(ARG2), d3   /*   get data in d3       */
     andl d3,a3@           /*   read & write         */
     moveml a7@+, d3/a3
     unlk a6
     rts


/*
void Reg32Or(unsigned int *addr, unsigned int data)
     *addr |= data
*/
_Reg32Or:                 /*   32 bit register OR process    */

     link a6, #0
     moveml d3/a3, a7@-
     movel a6@(ARG1), a3   /*   get address in a3    */
     movel a6@(ARG2), d3   /*   get data in d3       */
     orl d3,a3@            /*   read & write         */
     moveml a7@+, d3/a3
     unlk a6
     rts
