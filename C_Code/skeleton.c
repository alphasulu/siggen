/***********************************************************
*  skeleton.c
*  Example for ping-pong processing
*  Caution: It is intended, that this file ist not runnable. 
*  The file contains mistakes and omissions, which shall be
*  corrected and completed by the students.
*
*   F. Quint, HsKA
************************************************************/
#include <stdio.h>
#include <csl.h>
#include <csl_mcbsp.h>
#include <csl_irq.h>
#include <csl_edma.h>
#include <dsk6713_led.h> /*manuell addiert*/
#include "config_AIC23.h"
#include "skeleton.h"
#include <Siggencfg.h> /*edited on 06.03.2017 in order to add handle of IRQ and SEM to be able to process in main*/

#define BUFFER_LEN 500
/* Ping-Pong buffers. Place them in the compiler section .datenpuffer */
/* How do you place the compiler section in the memory?     */
#pragma DATA_SECTION(Buffer_in_ping, ".datenpuffer");
short Buffer_in_ping[BUFFER_LEN];
#pragma DATA_SECTION(Buffer_in_pong, ".datenpuffer");
short Buffer_in_pong[BUFFER_LEN];
#pragma DATA_SECTION(Buffer_out_ping, ".datenpuffer");
short Buffer_out_ping[BUFFER_LEN];
#pragma DATA_SECTION(Buffer_out_pong, ".datenpuffer");
short Buffer_out_pong[BUFFER_LEN];

//Configuration for McBSP1 (data-interface)
MCBSP_Config datainterface_config = {
		/* McBSP Control Register */
        MCBSP_FMKS(SPCR, FREE, NO)              |	//  Freilauf
        MCBSP_FMKS(SPCR, SOFT, NO)          	|   // Serial port clock stops immediately during emulation hal
        MCBSP_FMKS(SPCR, FRST, YES)             |	// Framesync ist ein
        MCBSP_FMKS(SPCR, GRST, YES)             |	// Reset aus, damit l�uft der Samplerate- Generator
        MCBSP_FMKS(SPCR, XINTM, XRDY)             |	// Transmit interrupt mode bit. 1h -> Interrupt is generated by end of block or end of frame
        MCBSP_FMKS(SPCR, XSYNCERR, NO)          |	// empf�ngerseitig keine �berwachung der Synchronisation
        MCBSP_FMKS(SPCR, XRST, YES)             |	// Sender l�uft (kein Reset- Status)	
        MCBSP_FMKS(SPCR, DLB, OFF)              |	// Loopback (Kurschluss) nicht aktiv
        MCBSP_FMKS(SPCR, RJUST, RZF)            |	// rechtsb�ndige Ausrichtung der Daten im Puffer
        MCBSP_FMKS(SPCR, CLKSTP, DISABLE)       |	// Clock startet ohne Verz�gerung auf fallenden Flanke (siehe auch PCR-Register)
        MCBSP_FMKS(SPCR, DXENA, OFF)            |	// DX- Enabler wird nicht verwendet
        MCBSP_FMKS(SPCR, RINTM, RRDY)           |	// Sender Interrupt wird durch "RRDY-Bit" ausgel�st
        MCBSP_FMKS(SPCR, RSYNCERR, NO)          |	// senderseitig keine �berwachung der Synchronisation
        MCBSP_FMKS(SPCR, RRST, YES),			// Empf�nger l�uft (kein Reset- Status)
		/* Empfangs-Control Register */
        MCBSP_FMKS(RCR, RPHASE, SINGLE)         |	// Nur eine Phase pro Frame
        MCBSP_FMKS(RCR, RFRLEN2, DEFAULT)       |	// L�nge in Phase 2, unrelevant
        MCBSP_FMKS(RCR, RWDLEN2, DEFAULT)       |	// Wortl�nge in Phase 2, unrelevant
        MCBSP_FMKS(RCR, RCOMPAND, MSB)          |	// kein Compandierung der Daten (MSB first)
        MCBSP_FMKS(RCR, RFIG, NO)               |	// Rahmensynchronisationspulse (nach dem ersten Puls)) startet die �bertragung neu
        MCBSP_FMKS(RCR, RDATDLY, 0BIT)          |	// keine Verz�gerung (delay) der Daten
        MCBSP_FMKS(RCR, RFRLEN1, OF(1))         |	// L�nge der Phase 1 --> 1 Wort
        MCBSP_FMKS(RCR, RWDLEN1, 16BIT)         |	// 0 -> Receive word length 16 bits
        MCBSP_FMKS(RCR, RWDREVRS, DISABLE),		// 32-bit Reversal nicht genutzt
		/* Sende-Control Register */
        MCBSP_FMKS(XCR, XPHASE, SINGLE)         |	// Transmit phases bit 1-> dual phase
        MCBSP_FMKS(XCR, XFRLEN2, DEFAULT)       |	// L�nge in Phase 2, unrelevant
        MCBSP_FMKS(XCR, XWDLEN2, DEFAULT)       |	// Wortl�nge in Phase 2, unrelevant
        MCBSP_FMKS(XCR, XCOMPAND, MSB)          |	// kein Compandierung der Daten (MSB first)
        MCBSP_FMKS(XCR, XFIG, NO)               |	// Rahmensynchronisationspulse (nach dem ersten Puls)) startet die �bertragung neu
        MCBSP_FMKS(XCR, XDATDLY, 0BIT)          |	// keine Verz�gerung (delay) der Daten
        MCBSP_FMKS(XCR, XFRLEN1, OF(1))         |	// L�nge der Phase 1 --> 1 Wort
        MCBSP_FMKS(XCR, XWDLEN1, 16BIT)         |	// Wortl�nge in Phase 1 --> 16 bit
        MCBSP_FMKS(XCR, XWDREVRS, DISABLE),		// 32-bit Reversal nicht genutzt
		/* Sample Rate Generator Register */
        MCBSP_FMKS(SRGR, GSYNC, DEFAULT)        |	// Einstellungen nicht relevant da
        MCBSP_FMKS(SRGR, CLKSP, DEFAULT)        |	// der McBSP1 als Slave l�uft
        MCBSP_FMKS(SRGR, CLKSM, DEFAULT)        |	// und den Takt von aussen 
        MCBSP_FMKS(SRGR, FSGM, DEFAULT)         |	// vorgegeben bekommt.
        MCBSP_FMKS(SRGR, FPER, DEFAULT)         |	// --
        MCBSP_FMKS(SRGR, FWID, DEFAULT)         |	// --
        MCBSP_FMKS(SRGR, CLKGDV, DEFAULT),		// --
		/* Mehrkanal */
        MCBSP_MCR_DEFAULT,				// Mehrkanal wird nicht verwendet
        MCBSP_RCER_DEFAULT,				// dito
        MCBSP_XCER_DEFAULT,				// dito
		/* Pinout Control Register */
        MCBSP_FMKS(PCR, XIOEN, SP)              |	// Pin wird f�r serielle Schnittstelle verwendet (alternativ GPIO)
        MCBSP_FMKS(PCR, RIOEN, SP)              |	// Pin wird f�r serielle Schnittstelle verwendet (alternativ GPIO)
        MCBSP_FMKS(PCR, FSXM, EXTERNAL)         |	// Framesync- Signal f�r Sender kommt von extern (Slave)
        MCBSP_FMKS(PCR, FSRM, EXTERNAL)         |	// Framesync- Signal f�r Empf�nger kommt von extern (Slave)
        MCBSP_FMKS(PCR, CLKXM, INPUT)           |	// Takt f�r Sender kommt von extern (Slave)
        MCBSP_FMKS(PCR, CLKRM, INPUT)           |	// Takt f�r Empf�nger kommt von extern (Slave)
        MCBSP_FMKS(PCR, CLKSSTAT, DEFAULT)      |	// unrelevant da PINS keine GPIOs
        MCBSP_FMKS(PCR, DXSTAT, DEFAULT)        |	// unrelevant da PINS keine GPIOs
        MCBSP_FMKS(PCR, FSXP, ACTIVEHIGH)       |	// Framesync senderseitig ist "activehigh"
        MCBSP_FMKS(PCR, FSRP, ACTIVEHIGH)       |	// Framesync empf�ngerseitig ist "activehigh"
        MCBSP_FMKS(PCR, CLKXP, FALLING)         |	// Datum wird bei fallender Flanke gesendet
        MCBSP_FMKS(PCR, CLKRP, RISING)			// Datum wird bei steigender Flanke �bernommen
};

/* template for a EDMA configuration */
EDMA_Config configEDMARcvPing = {
	/*Option Parameter*/
    EDMA_FMKS(OPT, PRI, LOW)          |  // auf beide Queues verteilen
    EDMA_FMKS(OPT, ESIZE, 8BIT)       |  // Element size
    EDMA_FMKS(OPT, 2DS, NO)            |  // kein 2D-Transfer
    EDMA_FMKS(OPT, SUM, NONE)          |  // Quell-update mode -> FEST (McBSP)!!!
    EDMA_FMKS(OPT, 2DD, NO)            |  // 2kein 2D-Transfer
    EDMA_FMKS(OPT, DUM, INC)           |  // Ziel-update mode
    EDMA_FMKS(OPT, TCINT,YES)         |  // EDMA interrupt erzeugen?
    EDMA_FMKS(OPT, TCC, OF(0))         |  // Transfer complete code (TCC)
    EDMA_FMKS(OPT, LINK, YES)          |  // Link Parameter nutzen?
    EDMA_FMKS(OPT, FS, NO),               // Frame Sync nutzen?
    /* Source Parameter*/
    EDMA_FMKS(SRC, SRC, OF(0)),           // Quell-Adresse
    /*Counter Parameter*/
    EDMA_FMK (CNT, FRMCNT, 0)       |  // Anzahl Frames
    EDMA_FMK (CNT, ELECNT, BUFFER_LEN),   // Anzahl Elemente
    /*Destination Parameter*/
    (Uint32)Buffer_in_ping,       		  // Ziel-Adresse
    /*Index Parameter*/
    EDMA_FMKS(IDX, FRMIDX, DEFAULT)    |  // Frame index Wert
    EDMA_FMKS(IDX, ELEIDX, DEFAULT),      // Element index Wert
    /*Reload Parameter*/
    EDMA_FMK (RLD, ELERLD, BUFFER_LEN)       |  // Reload Element
    EDMA_FMK (RLD, LINK, 0)            // Reload Link
};

EDMA_Config configEDMARcvPong = {
	/*Option Parameter*/
    EDMA_FMKS(OPT, PRI, LOW)          |  // auf beide Queues verteilen
    EDMA_FMKS(OPT, ESIZE, 8BIT)       |  // Element size
    EDMA_FMKS(OPT, 2DS, NO)            |  // kein 2D-Transfer
    EDMA_FMKS(OPT, SUM, NONE)          |  // Quell-update mode -> FEST (McBSP)!!!
    EDMA_FMKS(OPT, 2DD, NO)            |  // 2kein 2D-Transfer
    EDMA_FMKS(OPT, DUM, INC)           |  // Ziel-update mode
    EDMA_FMKS(OPT, TCINT,YES)         |  // EDMA interrupt erzeugen?
    EDMA_FMKS(OPT, TCC, OF(0))         |  // Transfer complete code (TCC)
    EDMA_FMKS(OPT, LINK, YES)          |  // Link Parameter nutzen?
    EDMA_FMKS(OPT, FS, NO),               // Frame Sync nutzen?
    /* Source Parameter*/
    EDMA_FMKS(SRC, SRC, OF(0)),           // Quell-Adresse
    /*Counter Parameter*/
    EDMA_FMK (CNT, FRMCNT, 0)       |  // Anzahl Frames
    EDMA_FMK (CNT, ELECNT, BUFFER_LEN),   // Anzahl Elemente
    /*Destination Parameter*/
    (Uint32)Buffer_in_pong,       		  // Ziel-Adresse
    /*Index Parameter*/
    EDMA_FMKS(IDX, FRMIDX, DEFAULT)    |  // Frame index Wert
    EDMA_FMKS(IDX, ELEIDX, DEFAULT),      // Element index Wert
    /*Reload Parameter*/
    EDMA_FMK (RLD, ELERLD, BUFFER_LEN)       |  // Reload Element
    EDMA_FMK (RLD, LINK, 0)            // Reload Link
};

/* template for a EDMA configuration */
EDMA_Config configEDMAXdPing = {
	/*Option Parameter*/
    EDMA_FMKS(OPT, PRI, LOW)          |  // auf beide Queues verteilen
    EDMA_FMKS(OPT, ESIZE, 8BIT)       |  // Element size
    EDMA_FMKS(OPT, 2DS, NO)            |  // kein 2D-Transfer
    EDMA_FMKS(OPT, SUM, INC)          |  // Quell-update mode -> Pong or Ping
    EDMA_FMKS(OPT, 2DD, NO)            |  // 2kein 2D-Transfer
    EDMA_FMKS(OPT, DUM, NONE)           |  // Ziel-update mode -> Fest (MCBSP)
    EDMA_FMKS(OPT, TCINT,YES)         |  // EDMA interrupt erzeugen?
    EDMA_FMKS(OPT, TCC, OF(0))         |  // Transfer complete code (TCC)
    EDMA_FMKS(OPT, LINK, YES)          |  // Link Parameter nutzen?
    EDMA_FMKS(OPT, FS, NO),               // Frame Sync nutzen?
    /* Source Parameter*/
    (Uint32)Buffer_out_ping,           // Quell-Adresse
    /*Counter Parameter*/
    EDMA_FMK (CNT, FRMCNT, 0)       |  // Anzahl Frames
    EDMA_FMK (CNT, ELECNT, BUFFER_LEN),   // Anzahl Elemente
    /*Destination Parameter*/
    EDMA_FMKS (DST, DST, OF(0)),       		  // Ziel-Adresse
    /*Index Parameter*/
    EDMA_FMKS(IDX, FRMIDX, DEFAULT)    |  // Frame index Wert
    EDMA_FMKS(IDX, ELEIDX, DEFAULT),      // Element index Wert
    /*Reload Parameter*/
    EDMA_FMK (RLD, ELERLD, BUFFER_LEN)       |  // Reload Element
    EDMA_FMK (RLD, LINK, 0)            // Reload Link
};

EDMA_Config configEDMAXdPong = {
	/*Option Parameter*/
    EDMA_FMKS(OPT, PRI, LOW)          |  // auf beide Queues verteilen
    EDMA_FMKS(OPT, ESIZE, 8BIT)       |  // Element size
    EDMA_FMKS(OPT, 2DS, NO)            |  // kein 2D-Transfer
    EDMA_FMKS(OPT, SUM, INC)          |  // Quell-update mode -> Pong or Ping
    EDMA_FMKS(OPT, 2DD, NO)            |  // 2kein 2D-Transfer
    EDMA_FMKS(OPT, DUM, NONE)           |  // Ziel-update mode -> Fest (MCBSP)
    EDMA_FMKS(OPT, TCINT,YES)         |  // EDMA interrupt erzeugen?
    EDMA_FMKS(OPT, TCC, OF(0))         |  // Transfer complete code (TCC)
    EDMA_FMKS(OPT, LINK, YES)          |  // Link Parameter nutzen?
    EDMA_FMKS(OPT, FS, NO),               // Frame Sync nutzen?
    /* Source Parameter*/
    (Uint32)Buffer_out_pong,           // Quell-Adresse
    /*Counter Parameter*/
    EDMA_FMK (CNT, FRMCNT, 0)       |  // Anzahl Frames
    EDMA_FMK (CNT, ELECNT, BUFFER_LEN),   // Anzahl Elemente
    /*Destination Parameter*/
    EDMA_FMKS (DST, DST, OF(0)),       		  // Ziel-Adresse
    /*Index Parameter*/
    EDMA_FMKS(IDX, FRMIDX, DEFAULT)    |  // Frame index Wert
    EDMA_FMKS(IDX, ELEIDX, DEFAULT),      // Element index Wert
    /*Reload Parameter*/
    EDMA_FMK (RLD, ELERLD, BUFFER_LEN)       |  // Reload Element
    EDMA_FMK (RLD, LINK, 0)            // Reload Link
};

/* Transfer-Complete-Codes for EDMA-Jobs */
int tccRcvPing;
int tccRcvPong;
int tccXdPing;
int tccXdPong;/*manuell addiert am 06.03.2017*/

/* EDMA-Handles; are these really all? */
EDMA_Handle hEdmaRcvPing;
EDMA_Handle hEdmaRcvPong;
EDMA_Handle hEdmaXdPing;
EDMA_Handle hEdmaXdPong;
MCBSP_Handle hMcbsp=0;
								
								
								
main()
{
	
	CSL_init();  
	
	/* Configure McBSP0 and AIC23 */
	Config_DSK6713_AIC23();
	
	/* Configure McBSP1*/
	hMcbsp = MCBSP_open(MCBSP_DEV1, MCBSP_OPEN_RESET);
    MCBSP_config(hMcbsp, &datainterface_config);
    
	/* configure EDMA */
    config_EDMA();

    /* finally the interrupts */
    config_interrupts();

    MCBSP_start(hMcbsp, MCBSP_XMIT_START | MCBSP_RCV_START, 0xffffffff);

} /* finished*/



void config_EDMA(void)
{
	/* Konfiguration der EDMA zum Lesen*/
	hEdmaRcvPing = EDMA_open(EDMA_CHA_REVT1, EDMA_OPEN_RESET);  // EDMA Channel for REVT1
	hEdmaRcvPong = EDMA_allocTable(-1);               // Reload-Parameters


	configEDMARcvPing.src = MCBSP_getRcvAddr(hMcbsp);          //  source addr
	tccRcvPing = EDMA_intAlloc(-1);                        // next available TCC
	configEDMARcvPing.opt |= EDMA_FMK(OPT,TCC,tccRcvPing);     // set it
	EDMA_config(hEdmaRcvPing, &configEDMARcvPing);



	/*We need another channel to perform link transfer in double buffering mode later*/
	configEDMARcvPong.src = MCBSP_getRcvAddr(hMcbsp);
	tccRcvPong = EDMA_intAlloc(-1);
	configEDMARcvPong.opt |=  EDMA_FMK(OPT,TCC,tccRcvPong);
	EDMA_config(hEdmaRcvPong, &configEDMARcvPong);



	/*Konfiguration zum Schreiben*/
	hEdmaXdPing = EDMA_open(EDMA_CHA_XEVT1, EDMA_OPEN_RESET);
	hEdmaXdPong = EDMA_allocTable(-1);
	tccXdPing = EDMA_intAlloc(-1);
	configEDMAXdPing.opt |= EDMA_FMK(OPT, TCC, tccXdPing);
	configEDMAXdPing.dst = MCBSP_getXmtAddr(hMcbsp);
	EDMA_config(hEdmaXdPing,&configEDMAXdPing);
	


	/* Konfiguration f�r Pong*/
	tccXdPong = EDMA_intAlloc(-1);
	configEDMAXdPong.opt |= EDMA_FMK(OPT, TCC, tccXdPong);
	configEDMAXdPong.dst = MCBSP_getXmtAddr(hMcbsp);
	EDMA_config(hEdmaXdPong, &configEDMAXdPong);


	/* link transfers ping -> pong -> ping */
	EDMA_link(hEdmaRcvPing, hEdmaRcvPong);  /* is that all? */ /**changed to EDMA_link(hEdmaRcv, xxxxx) to EDMA_link(hEdmaRcv, hEdmaReload)*/
	EDMA_link(hEdmaRcvPong, hEdmaRcvPing);
	EDMA_link(hEdmaXdPing, hEdmaXdPong); /*Added on Tuesday 14.03.2017 to link reload parameter to */
	EDMA_link(hEdmaXdPong, hEdmaXdPing);

	/* do you want to hear music? */


	/* enable EDMA TCC */
	EDMA_intClear(tccRcvPing);
	EDMA_intEnable(tccRcvPing);
	/* some more? */
	EDMA_intClear(tccRcvPong);
	EDMA_intEnable(tccRcvPong);

	EDMA_intClear(tccXdPing);
	EDMA_intEnable(tccXdPing);

	EDMA_intClear(tccXdPong);
	EDMA_intEnable(tccXdPong);

	/* which EDMAs do we have to enable? */
	EDMA_enableChannel(hEdmaRcvPing);
	EDMA_enableChannel(hEdmaXdPing);
}


void config_interrupts(void)
{
	IRQ_map(IRQ_EVT_EDMAINT, 8); // IRG_EVT_EDMAINT
	IRQ_clear(IRQ_EVT_EDMAINT);
	IRQ_enable(IRQ_EVT_EDMAINT);
	IRQ_globalEnable();
}


void EDMA_interrupt_service(void)
{
	static int rcvPingDone=0; // static
	static int rcvPongDone=0;
	static int xmtPingDone=0;
	static int xmtPongDone=0;
	
	if(EDMA_intTest(tccRcvPing)) {
		EDMA_intClear(tccRcvPing); /* clear is mandatory */
		rcvPingDone=1;
	}
	else if(EDMA_intTest(tccRcvPong)) {
		EDMA_intClear(tccRcvPong);
		rcvPongDone=1;
	}
	else if(EDMA_intTest(tccXdPing)){
		EDMA_intClear(tccXdPing);
		xmtPingDone=1;
	}
	else if(EDMA_intTest(tccXdPong)){
		EDMA_intClear(tccXdPong);
		xmtPongDone=1;
	}
	
	//........... // transmit ping and pong
	
	if(rcvPingDone && xmtPingDone) {
		rcvPingDone=0;
		xmtPingDone=0;
		// processing in SWI
		SWI_post(&SWI_process_ping);
	}
	else if(rcvPongDone && xmtPongDone) {
		rcvPongDone=0;
		xmtPongDone=0;
		// processing in SWI
		SWI_post(&SWI_process_pong);
	}
	//SWI_post(&PRD_LEDToggle);
}

void process_ping_SWI(void)
{
	int i;
	for(i=0; i<BUFFER_LEN; i++)
		*(Buffer_out_ping+i) = *(Buffer_in_ping+i);
}

void process_pong_SWI(void)
{
	int i;
	for(i=0; i<BUFFER_LEN; i++)
		*(Buffer_out_pong+i) = *(Buffer_in_pong+i);
}

void SWI_LEDToggle(void)
{
	SEM_postBinary(&SEM_LEDToggle);	
}

void tsk_led_toggle(void)
{
	/* initializatoin of the task */
	/* nothing to do */
	
	/* process */
	while(1) {
		SEM_pendBinary(&SEM_LEDToggle, SYS_FOREVER);
		
		DSK6713_LED_toggle(1);
	}
}
