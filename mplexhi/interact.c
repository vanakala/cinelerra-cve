#include "main.h"
#include <stdlib.h>
#include <unistd.h>

/*************************************************************************
    Startbildschirm und Anzahl der Argumente

    Intro Screen and argument check
*************************************************************************/

static void Usage(char *str)
{
	fprintf( stderr, "lavtools mplex version " VERSION "\n" );
	fprintf( stderr, "Usage: %s [params] [<input file1> [<input file2>] <output file>\n\n", str);
	fprintf( stderr, "  where possible params are:\n" );
	fprintf( stderr, " -q      Quiet mode for unattended batch usage\n" );
	fprintf( stderr, " -n      Noisy (verbose) mode for debugging streams\n" );
	fprintf( stderr, " -m      Mpeg version (default: 1) [1..2]\n");
	fprintf( stderr, " -b num  Specify decoder buffers size in kB. (default: 46) [ 20...1000]\n" );
    fprintf( stderr, " -r num  Specify data rate of output stream in kbit/sec (default 0=Compute from source streams)\n" );
	fprintf( stderr, " -l num  Multiplex only num frames (default 0=multiplex all)\n");
	fprintf( stderr, " -v num  Specify a video timestamp offset in mSec\n");
	fprintf( stderr, " -a num  Specify an audio timestamp offset in mSec \n" );
	fprintf( stderr, " -s num  Specify sector size in bytes (default: 2324) [256..16384]\n");
	fprintf( stderr, " -V      Multiplex variable bit-rate (experimental)\n");
	fprintf( stderr, " -p num  Number of packets per pack (default: 20) [1..100]\n"  );
	fprintf( stderr, " -h      System header in every pack rather than just in first\n" );
	fprintf( stderr, " -f fmt  Set pre-defined mux format.\n");
	fprintf( stderr, "         [0 = Auto MPEG1, 1 = VCD, 2 = Auto MPEG2, 3 = SVCD, 4 = DVD]\n");
	fprintf( stderr, "         (N.b only 0 .. 3 currently implemented!*)\n" ); 
	fprintf( stderr, " -S size Maximum size of output file in M bytes (default: 680)\n" );
	fprintf( stderr, " -M      Generate a *single* multi-file program rather a program per file\n");
	fprintf( stderr, "         %%d in the output file name is replaced by a segment counter\n");
	fprintf( stderr, " -e      Vcdmplex style start-up (debugging tool)\n");
			
	exit (1);
}

int verbose = 1;
int opt_buffer_size = 46;
int opt_data_rate = 0;  /* 3486 = 174300B/sec would be right for VCD */
int opt_video_offset = 0;
int opt_audio_offset = 0;
int opt_sector_size = 2324;
int opt_VBR = 0;
int opt_mpeg = 1;
int opt_mux_format = 0;			/* VCD as default */
int opt_multifile_segment = 1;
int opt_always_system_headers = 0;
int opt_packets_per_pack = 20;
clockticks opt_max_PTS = 0LL;
int opt_emul_vcdmplex = 0;

/* Should fit nicely on an ordinary CD ... */
intmax_t max_system_segment_size =  680*1024*1024;

int intro_and_options(int argc, char *argv[])
{
    int n;
    printf("\n***************************************************************\n");
    printf(  "*               MPEG1/SYSTEMS      Multiplexer                *\n");
    printf(  "*               (C)  Christoph Moar, 1994/1995                *\n");
    printf(  "*               moar@informatik.tu-muenchen.de                *\n");
    printf(  "*               Technical University of Munich                *\n");
    printf(  "*               SIEMENS ZFE  ST SN 11 / T SN 6                *\n");
    printf(  "*                                                             *\n");
    printf(  "*  This program is free software. See the GNU General Public  *\n");
    printf(  "*  License in the file COPYING for more details.              *\n");
    printf(  "*  Release %s (%s)                                  *\n",MPLEX_VER,MPLEX_DATE);
    printf(  "***************************************************************\n\n");


  while( (n=getopt(argc,argv,"b:r:v:a:m:f:l:S:qiVnMe")) != EOF)
  {
    switch(n)
	  {
	  
	  case 'm' :
		opt_mpeg = atoi(optarg);
		if( opt_mpeg < 1 || opt_mpeg > 2 )
		  Usage(argv[0]);
  	
	  	break;
	  case 'q' :
		verbose = 0;
		break;
	
	  case 'n' :
		verbose = 2;
		break;
	
	  case 'V' :
	    opt_VBR = 1;
	    break;
	  
	  case 'h' :
	  	opt_always_system_headers = 1;
		break;

	  case 'b':
		opt_buffer_size = atoi(optarg);
		break;

	  case 'r':
		opt_data_rate = atoi(optarg);
		if( opt_data_rate < 0 )
			Usage(argv[0]);
		/* Convert from kbit/sec (user spec) to 50B/sec units... */
		opt_data_rate = (( opt_data_rate * 1000 / 8 + 49) / 50 ) * 50;
		break;

	  case 'v':
		opt_video_offset = atoi(optarg);
		if( opt_video_offset < -10000000 || opt_video_offset > 1000000 )
		  Usage(argv[0]);
		break;

	  case 'a' :
		opt_audio_offset = atoi(optarg);
		if( opt_video_offset < -10000000 || opt_video_offset > 1000000 )
		  Usage(argv[0]);
		break;
		
	  case 'l' : 
	  	opt_max_PTS = (clockticks)atoi(optarg) * (clockticks)CLOCKS;
		if( opt_max_PTS < 1  )
		  Usage(argv[0]);
		break;
		
	  case 'p' : 
	  	opt_packets_per_pack = atoi(optarg);
		if( opt_packets_per_pack < 1 || opt_packets_per_pack > 100  )
		  Usage(argv[0]);
		break;
		
	  
	  case 'f' :
	    opt_mux_format = atoi(optarg);
	    if( opt_mux_format < MPEG_MPEG1 || opt_mux_format > MPEG_SVCD )
	    	Usage(argv[0]);
		break;
	  case 's' :
		opt_sector_size = atoi(optarg);
		if( opt_sector_size < 256 || opt_sector_size > 16384 )
		  Usage(argv[0]);
		break;
	  case 'S' :
		max_system_segment_size = atoi(optarg);
		if( max_system_segment_size < 1  )
		  Usage(argv[0]);
		   max_system_segment_size *= 1024*1024; 
		break;
	  case 'M' :
	  	 opt_multifile_segment = 1;
		 break;
	  case 'e' :
	  	 opt_emul_vcdmplex = 1;
		 break;
	  default :
		Usage(argv[0]);
		break;
	  }
  }
  if (argc - optind < 2)
    {	
	  Usage(argv[0]);
    }
  return optind-1;
}


/*************************************************************************
    File vorhanden?

    File found?
*************************************************************************/

int open_file(name, bytes)			
char *name;
unsigned int *bytes;				
{
    FILE* datei;

    datei=fopen (name, "rw");
    if (datei==NULL)
    {	
	printf("File %s not found.\n", name);
	return (TRUE);
    }
    fseek (datei, 0, 2);
    *bytes = ftell(datei);
    fclose(datei);
    return (FALSE);
}



/******************************************************************
	Status_Info
	druckt eine Statuszeile waehrend des Multiplexens aus.

	prints a status line during multiplexing
******************************************************************/

void status_info (	unsigned int nsectors_a,
					unsigned int nsectors_v,
					unsigned int nsectors_p,
					unsigned long long nbytes,
					unsigned int buf_v,
					unsigned int buf_a,
					int verbose
				 )
{

	if( verbose > 0 )
	{
	  printf ("| %7d | %7d |",nsectors_a,nsectors_v);
	  if( opt_VBR )
		  printf( "   VBR   |");
	  else
		  printf (" %7d |",nsectors_p);
	  printf ("%11lld  |",nbytes);
	  printf (" %6d | %6d |",buf_a,buf_v);
	  printf ((verbose > 1?"\n":"\r"));
	  fflush (stdout);
	}
}

void status_header (void)
{
    status_footer();
    printf("|  Audio  |  Video  | Padding | Bytes  MPEG | Audio  | Video  |\n");
    printf("| Sectors | Sectors | Sectors | System File | Buffer | Buffer |\n");
    status_footer();
}


void status_message (int what, int decode_number)

{
  if( verbose == 1 )
	printf( "\n" );
  switch (what)
  {
  case STATUS_AUDIO_END:
  printf("|file  end|         |         |             |        |        |\n");
  break;
  case STATUS_AUDIO_TIME_OUT:
  printf("|TO%07d   |         |         |             |        |        |\n", decode_number);
  break;
  case STATUS_VIDEO_END:
  printf("|         |file  end|         |             |        |        |\n");
  break;
  case STATUS_VIDEO_TIME_OUT:
  printf("|         |TO%07d|         |             |        |        |\n", decode_number);
  }
}

void status_footer (void)
{
  printf("+---------+---------+---------+-------------+--------+--------+\n");
}
