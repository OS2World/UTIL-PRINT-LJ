 /******************************************************************************
 **
 **   LJ -- A printing utility for the HP LaserJet
 **
 **
 **   This program prints a series of files on the LaserJet printer.  The
 **   files are printed in a "landscape" font at 17 characters to the inch.
 **   To take advantage of this density, two "pages" of information from
 **   the file are printed on each piece of paper (left and right halves).
 **
 **   Usage is:       LJ  file1 file2 file3 ...
 **
 **   Where file# is a valid MS-DOS filename, included on the command line.
 **
 **   Originally written by Joe Barnhart and subsequently modifed by:
 **     Ray Duncan and Chip Rabinowitz.  This program has been placed in
 **     the public domain for use without profit.
 **
 **   Revised 9/86 for the Mark Williams C Programming System,
 **     Steven Stern, JMB Realty Corp.
 **
 **   Revised 11/86 for DOS wild card characters in the file name.
 ****************************************************************************
 **
 **   Revised 4/88 for the DATALIGHT C Compiler by Serge Stepanoff,
 **   STS Enterprises, 5469 Arlene Way, Livermore, CA.
 **
 **	The revised code uses the built in file wildcard expansion feature
 **	of DATALIGHT C, and makes use of some additional flags to
 **	turn off the Header line, optionally use the 7 point
 **	Prestige Elite font, and ssome oher features.  Typing "lj2" with
 **	no parameters will produce a short users' explanaion.
 **
 **	Program has been checked out on an OKIDATA
 **	LASERLINE printer (HP Laserjet compatible).
 *****************************************************************************
 ** Revised 10/16/89 by Jim Derr for the Turbo C 2.0 Compiler.
 ** Uses the wildargs obj module for file wild card expansion.
 ** Corrected some anomalies in the code that was causing invalid pages breaks.
 ** Added the -s command line option that will shade every other line on the
 ** printout for readability.
 *****************************************************************************
 ** Revised 1/11/91 by Jim Derr.
 ** added -l option to specify number of lines per page to print.
 ** added -i options to ignore formfeed characters.
 ** added -d option to allow for duplex printing.
 **          (Note: when printing duplex only one file at a time may be
 **           specified on the command line)
 *****************************************************************************
 ** Revised 7/31/92 by Mark Polly
 ** compiled under OS/2 2.0 using C set/2.
 ** removed the message "printing page nn, line nn to make it look good under
 ** IBM's Workframe
 **/

#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>

int     MAXLINE=66;        /* maximum lines per page on LaserJet */
int     MAXLINE_RESET=66;  /* maximum lines per page on LaserJet */
#define MAXVERT 69         /* maximum lines per page on LaserJet */


 FILE *lp; 

int line_no = 1;
int pagenum;

FILE *fp;               /* FILE pointer for the file to be printed */

int  hdrflag;		/* Header print suppress flag */
int  pgflag;		/* Page separator flag */
int  tab = 4;		/* default value of one tab stop */
int  pgside;		/* left/right half of page indicator  */
int  psprint = 0;   /* proportional using Helv 6 pt. */
int  shade_it = 0;  /* shade every other line */
int  book = 0;      /* indent left 6 additional spaces for punching*/
int  maxcol=85;
int  numbers = 0;
int  duplex=0;
int  formfeed=0;

int do_print=1;     /*flags for doing duplex printing*/

/*************************************************************************

main

*************************************************************************/

main(argc, argv)
int argc;
char *argv[];
{
int filenum;
char fname[70];
int first;
int jim;

   if (argc <= 1)
      {
      usage();
      exit(1);
      }


  lp = fopen("PRN","w+");

/* initialize the LaserJet for landscape printing */

   fprintf( lp,"\033E\033&l1O\033(s17H\033&l5.14C\033&l70F\033&l5E" );

   for (first = 1; first < argc; first++)  {
      if (*argv[first] != '/' && *argv[first] != '-') /* check for flags */
         break;					/* and exit if none */
      argv[first]++;
      switch (tolower(*argv[first]))  {
         case 'e':		/* Prestige Elite 7 point font */
            psprint = 1;
			maxcol=104;
            fprintf(lp,"\033&l1O\033(0U\033(s1p6vsb4T");
            break;
         case 'h':		/* suppress headers flag */
            hdrflag++;
			MAXLINE=69;
			MAXLINE_RESET=69;
            break;
         case 't':		/* Horizontal tab value */
            tab = atoi(++argv[first]);
            if (tab < 1 || tab > 8)  {
                printf("Invalid tab value specified -- defaulting to 8\n");
                tab = 8;
                }
			break;
         case 'l':
            MAXLINE = atoi(++argv[first]);
            if (MAXLINE < 1 || MAXLINE > MAXLINE_RESET)  {
                printf("Invalid lines/page value specified -- defaulting to MAX\n");
                MAXLINE = MAXLINE_RESET;
                }
			break;
         case 'p':      /* suppress page eject between files */
			pgflag++;
			break;
		 case 's':
			shade_it++;
			break;
         case 'n':
            numbers++;
            break;
         case 'b':
            book++;
            maxcol=79;
            break;
         case 'd':
            duplex=1;
            break;
         case 'i':
            formfeed++;
            break;
         default:
            printf("Invalid Flag Specified ('%s') -- ignored.....\n",
            		--argv[first]);
            break;
         }
      }

   printf("Lines per page set to %d\n",MAXLINE);

   if(duplex) {
     if((argc - 1) > first) {
        printf("Only one file at a time may be printed when using DUPLEX mode.\n");
        exit(1);
     }
   }

   for(filenum = first; filenum < argc; filenum++ )
      {
         copyname(fname, argv[filenum]);
         fp = fopen(fname ,"r");
         if( fp == NULL )
            {
            printf( "File %s doesn't exist.\n", fname );
            }
         else
            {
            printf( "Now printing %s\n", fname );
            printfile( fname );
            fclose( fp );
            }
      }    /* of loop through run-time args */

/*
   if (pgflag & pgside)
	fputc('\f', lp);
*/
   fprintf( lp, "\r\033E" );      /* clear LaserJet */
   fclose(lp);


}

/*************************************************************************

printfile (filename)

*************************************************************************/

printfile(filename)
char *filename;
{
   int retval = 0;
   int printed = 0;
   int pass=0;
   char c2;
   int f_feed=0;
   int phys_pages=0;
   int plus=1;
   int z;

   pagenum=1;

do_it_again:
   while( (feof(fp)==0) && (retval==0) )
	  {
      f_feed=0;
	  if (pgside == 0)  {
		 printed = 0;
		 c2 = fgetc(fp);
		 if(feof(fp)) break;
		 switch (c2) {
			case EOF:
			case '\377':
			case '\032':
				retval = -1;
				break;
			default:
				printed=1;
                if(do_print) dovert();
                if(psprint && do_print)
				   fprintf(lp,"\033&a0r0c120m0L\r");
                else if(book && do_print)
                   fprintf(lp, "\033&a0r0c85m7L\r"); /* set LaserJet to left half */
                else if(do_print)
                   fprintf(lp, "\033&a0r0c85m0L\r"); /* set LaserJet to left half */
                if(shade_it && do_print) shade();
				ungetc(c2,fp);
                header(filename, pagenum++);       /* title top of page */
				retval = printpage();                 /* print one page */
				pgside ^= 1;
		 }
	  }
      if(feof(fp)==0 && retval==0)
         {                                  /* if more to print... */
         if(psprint && do_print)
			fprintf(lp,"\033&a0r0c243m123L\r");
         else if(book && do_print)
            fprintf(lp, "\033&a0r0c175m97L\r");    /* LaserJet to right half */
         else if(do_print)
            fprintf(lp, "\033&a0r0c175m90L\r");    /* LaserJet to right half */
		 c2 = fgetc(fp);
         if(feof(fp)) break;
         switch (c2) {
			case EOF:
			case '\377':
			case '\032':
				retval = -1;
				break;
			default:
				ungetc(c2,fp);
                header(filename, pagenum++);          /* title top of page */
				retval = printpage();                 /* print one page */
				pgside ^= 1;
		 }
	  }
	  if (pgside == 0 && printed && do_print) {
		 fputc('\f', lp);
		 f_feed=1;
	  }
      else if (pgflag == 0 && printed && do_print)  {
		 fputc('\f', lp);
		 pgside = 0;
		 f_feed=1;
		 }
	  if(f_feed) {
		if(plus)
			++phys_pages;
		else
			--phys_pages;
	  }
	  if(duplex) do_print ^= 1;
	  }


	  if(duplex) {
		if(pass) {
			while(phys_pages) {
				fputc('\f',lp);
				--phys_pages;
			}
			return(0);
		}
		plus=0;
        if(!f_feed && retval == 0) fputc('\f',lp);
		fflush(lp);
		++pass;
		rewind(fp);
		retval=0;
		pagenum=1;
		do_print = 0;
        printf("\nFlip the paper and press any key when ready\n");
		z = getchar();
		goto do_it_again;
	  }
   return(0);
}

/*******************************************************************************

printpage
   print a logical page

*************************************************************************/

printpage()
{
   char c;
   int line,col;
   static int cont = 0;
   static char *cont1 = "---->";

   line = col = 0;
   if(cont)
      {
      if(do_print) fprintf(lp,cont1);
      col = strlen(cont1);
      cont = 0;
      }

   while( line < MAXLINE )
      {
	  c = fgetc(fp);
	  if(feof(fp)) return(-1);

      if(col>maxcol)
         {
         line++;
         switch(c)
            {
            case '\n':
            case '\r':
            case '\f':
            case EOF:
            case '\377':
            case '\032':
               break;
            default:
               if(line >= MAXLINE)
                  {
                  cont = 1;
                  ungetc(c,fp);
                  return(0);
                  }
               if(do_print) fprintf(lp,"\n%s",cont1);
               col = strlen(cont1);
               break;
            }
		 }

      if(col == 0) {
 /*********************************************/
 /* removed the following lines to make it work good under IBM's Workframe  *
 *         if(do_print) {
 *             printf("Printing page %4.4d line %4.4d\r",pagenum-1,line);
 *         }
 *         else {
 *             printf("Skipping page %4.4d line %4.4d\r",pagenum-1,line);
 *         }
 */
          if(numbers) {
              if(do_print) fprintf(lp,"%4.4d:",line_no);
              col=5;
          }
          ++line_no;
      }

      switch(c)
         {
         case '\n':           /* newline found */
            col = 0;          /* zero column and */
            line++;           /* advance line count */
			if( line < MAXLINE )
               if(do_print) fprintf(lp,"\n");
            break;
         case '\r':           /* CR found */
            break;            /* discard it */
         case '\t':           /* TAB found */
            do
               if(do_print) fputc(' ',lp);
            while ( (++col % tab) != 0 );
            break;
         case '\f':                      /* Page break or */
            if(formfeed) break;
			if(line != 0)
				line = MAXLINE;      /* force termination of loop */
            break;
         case EOF:            /* EOF mark */
         case '\377':         /* EOF mark */
         case '\032':         /* EOF mark */
            return(-1);
         default:              /* no special case */
            if(do_print) fputc(c,lp);       /* print character */
            col++;
            break;
      }
   }
   return(0);
}

/*************************************************************************

header
   print a page header

*************************************************************************/

header( filename, pagenum )
char *filename;
int pagenum;
{
   char datestr[11], timestr[11];
   if (hdrflag)  {
      return;		/* skip if flag set */
      }
   timestamp(timestr);
   datestamp(datestr);
   if(do_print) fprintf(lp,"\033&d0D");
   if(book && do_print)
	   fprintf(lp, "File: %-40s%s   %s  --  Page: %03d \n\n",
		  filename,datestr,timestr,pagenum);
    else if(do_print)
	   fprintf(lp, "File: %-40s%s   %s  --  Page: %03d       \n\n",
          filename,datestr,timestr,pagenum);
   if(do_print) fprintf(lp,"\033&d@");
}

timestamp( timestr )
char   *timestr;
{
   struct tm *tod;
   time_t tt;
   tt = time(NULL);
   tod = localtime(&tt);
   sprintf(timestr,"%02d:%02d", tod->tm_hour, tod->tm_min);
   return;
}

datestamp( datestr )
char  *datestr;
{
   struct tm *tod;
   time_t tt;
   tt = time(NULL);
   tod = localtime(&tt);
   sprintf(datestr,"%02d/%02d/%02d", tod->tm_mon + 1, tod->tm_mday,
	tod->tm_year + 1900);
   return;
}

/*************************************************************************

dovert()
   draw a vertical line down the center of the physical page

*************************************************************************/

dovert()
{
   int line = 1;

   if(psprint)
		fprintf(lp,"\033&a0r0c123m121L\r|");
   else
		fprintf(lp,"\033&a0r0c90m88L\r|");

   while(line++ < MAXVERT) fprintf(lp,"\n|");
}

/*************************************************************************

copyname
	copy a file name converting to upper case.

*************************************************************************/

copyname(to, from)
char *to, *from;
{
	while (*from)
		*to++ = toupper(*from++);
	*to = 0;
}

/************************************************************************/

usage()
{
	printf("\nUSAGE:\n\n");
	printf("C>LJ2 [flags] filename1 [filename2 ..........]\n");
	printf("\tItems shown in brackets are optional.");
	printf("\n\n\tFilename(s) may include wildcards");
	printf("\n\n\tFlags are:\n");
    printf("\t\t-e (or /e) select Helv proportional 6 PT font\n");
	printf("\t\t-h (or /h) suppress page headers\n");
	printf("\t\t-tx (or /tx) set tab stop value (where x is 1 to 8)\n");
	printf("\t\t-p (or /p) do not leave blank half pages between files\n");
	printf("\t\t-s (or /s) shade every other line\n");
	printf("\t\t-n (or /n) print line numbers\n");
	printf("\t\t-b (or /b) indent for punching\n");
    printf("\t\t-d (or /d) for duplex printing\n");
    printf("\t\t-lxx (or /lxx) set lines/page to x(where x is 1 to 66)\n");
    printf("\t\t-i (or /l) ignore formfeed characters\n");
}


shade()
{


	int i;
    fprintf (lp, "\033&f0S\033*p0x0Y\033*c3300a32b10G");
    for (i=1; i <= 35; i++)
        fprintf (lp, "\033*c2P\033&a+2R");
    fprintf (lp, "\033&f1S");

}


