#ifndef CHANNELTABLES_H
#define CHANNELTABLES_H

typedef struct
{
	char *name;
	int   freq;
} CHANLIST;


typedef struct 
{
    char             *name;
    CHANLIST 		 *list;
    int               count;
} CHANLISTS;

#define CHAN_COUNT(x) (sizeof(x)/sizeof(CHANLIST))

extern CHANLISTS chanlists[];

#define NTSC_AUDIO_CARRIER	4500
#define PAL_AUDIO_CARRIER_I	6000
#define PAL_AUDIO_CARRIER_BGHN	5500
#define PAL_AUDIO_CARRIER_MN	4500
#define PAL_AUDIO_CARRIER_D	6500
#define SEACAM_AUDIO_DKK1L	6500
#define SEACAM_AUDIO_BG		5500
#define NICAM728_PAL_BGH	5850
#define NICAM728_PAL_I		6552

// Norms
#define NTSC 0
#define PAL 1
#define SECAM 2

// Frequencies
#define NTSC_BCAST 0
#define NTSC_CABLE 1
#define NTSC_HRC 2
#define NTSC_BCAST_JP 3
#define NTSC_CABLE_JP 4
#define PAL_EUROPE 5
#define PAL_E_EUROPE 6
#define PAL_ITALY 7
#define PAL_NEWZEALAND 8
#define PAL_AUSTRALIA 9
#define PAL_IRELAND 10

#endif
