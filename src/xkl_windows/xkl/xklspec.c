/* $Log: xklspec.c,v $
 * Revision 1.6  2002-06-13 11:11:20-04  tiede
 * support 4096 point FFT
 *
 * Revision 1.4  1999/05/20 19:49:44  vkapur
 * no significant changes made
 *
 * Revision 1.3  1999/02/17 23:54:37  vkapur
 * changed showoops->ShowOops
 *
 * Revision 1.2  1998/07/17 08:00:32  krishna
 * Added RCS header, and hifted spectrum and smoothed spectrum up --
 * spectrums were off by 4dB.
 * 
 *
 */

#include "xspec_util.h"
#include "xinfo.h"
#include "xklspec.h"
#include "spectrum.h"
//#include "../lspecto/lspgetf0.h"

/* NOTE: see paramlist in xkl.c for information about the spectro->params array*/

/* this is a collection of the functions that were in makefbank + quickspec*/
/* basically what's needed to generate the data for the spectrum window*/
/* They have been modified to use the spectro structure so that all of*/
/* the globals now reside on spectro and nwave has been eliminated*/ 
/* which wave is active is handled by which spectro structure is passed*/
/* to the callback */


/*	quickspec(arg1,arg2)
 *		arg1	pointer to beginning of waveform chunk
 *		arg2	type of spectrum to be computed, 1=CB, 2=SPECTO, 3=LPC
 *		answer	placed in fltr[lenfltr]
 *
 *	spatfilt()
 *		input	spectrum in array fltr[lenfltr]
 *		output	fltr[lenfltr] after spatial filtering
 *
 *	distance(arg1,arg2,arg3)
 *		arg1	pointer to unknown spectrum
 *		arg2	pointer to reference spectrum
 *		arg3	type of spectral distance, 1=EUCLID, 2=SLOPE, 3=RESID
 */


/* this code includes the modules that draw the two waveforms and the*/
/* spectrum */

/* 1. IF "type_spec" IS EQUAL TO 1, COMPUTE CRITICAL-BAND SPECTRUM */
/*    Input is sizwin-point integer waveform chunk in iwave[] */
/*      Do first difference preemphasis			      */
/*      Multiply by a Hamming window			      */
/*      Compute 256-point dft, put mag in array dft[128]      */
/*      Perform weighted sum of dft samples to		      */
/*      place nchan critical-band spectrum in fltr[nchan]     */
/*        where output is dB-times-10			      */
/*        and total energy is in fltr[nchan]		      */

/* 3. IF "type_spec" IS EQUAL TO 2 or 5, COMPUTE SPECTROGRAM SPECTRUM   */
/*    Input is sizwin-point integer waveform chunk in iwave[] */
/*      Do first difference preemphasis (2 only)	      */
/*      Multiply by a Hamming window (2 only)		      */
/*      Compute 256-point dft, put mag in array dft[128] (2 only) */
/*      Perform weighted sum of dft samples to		      */
/*      place nchan critical-band spectrum in fltr[nchan]     */
/*        where output is dB-times-10			      */
/*        and total energy is in fltr[nchan]		      */

/* 4. IF "type_spec" IS EQUAL TO 3, COMPUTE LPC SPECTRUM      */
/*    Input is sizwin-point integer waveform chunk in iwave[] */
/*      Do first difference preemphasis			      */
/*      Multiply by a Hamming window			      */
/*	Call lpc() to get coefficients			      */
/*	Pad coefficient array with zeros		      */
/*      Compute 256-point dft, put mag in array dft[128]      */


/**************************************************************/
/* spectrum_pixmap(INFO *info,SPECTRO *spectro, FILE *postfp) */
/* draw grid and labels for spectrum window*/
/**************************************************************/
void spectrum_pixmap(INFO *info,XSPECTRO *spectro, FILE *postfp)
{
  int grdox,grdoy,grdxr,grdyb;
  int i,fmax,x,y,y0;
  int x2;
  float xinc,yinc,fx,fy;
  char str[15];
  int yps;
  float ps_scale;
  
  if(postfp != NULL) { /* need to set ps_scale*/
    if(postfp == PS4_fp) ps_scale = spectro->Gscale;
    else ps_scale = spectro->gscale;
  } 
  
  grdoy = 0;
  /* add another line for text so the label can be centered
     below the tick marks   */
  /*grdyb = spectro->yb[SPECTRUM] - spectro->hchar;*/
  grdyb = spectro->yb[SPECTRUM] - 2 * spectro->hchar;
  if(postfp!=NULL)
    grdyb = spectro->yb[SPECTRUM] - 2 * 
      (float)spectro->hchar * ps_scale;
  
  grdox = spectro->xr[SPECTRUM] * SPEC_OX;
  grdxr = spectro->xr[SPECTRUM] * SPEC_XR;
  
  if(postfp == NULL){
    /* spectrum background */

   XSetForeground(info->display,info->gc,
                  info->color[2].pixel);

   XFillRectangle(info->display,info->pixmap,
     info->gc, 0,0,
     spectro->xr[SPECTRUM],spectro->yb[SPECTRUM]); 

   XSetForeground(info->display,info->gc,
                  info->color[1].pixel);
 }

   yps = spectro->yb[3];/* filp y for postscript*/
  
     /* outline */

  if(postfp != NULL) {
  fprintf(postfp,"%d setlinejoin\n",1);
  fprintf(postfp, "%d %d m %d %d k\n",
             grdox ,yps - grdoy ,grdox , yps - grdyb );
  fprintf(postfp, "%d %d m %d %d k\n",
             grdox ,yps - grdyb ,grdxr , yps - grdyb );
  fprintf(postfp, "%d %d m %d %d k\n",
             grdxr ,yps - grdoy ,grdxr , yps - grdyb );
  fprintf(postfp, "%d %d m %d %d k\n",
             grdox ,yps - grdoy ,grdxr , yps - grdoy );

  /* rotate the y axis label */
  fprintf(postfp,"gsave\n");

  /* at this point things have already been rotated 90 */
  /* there must be an easier way of doing this but I */
  /* don't know what it is */

  fprintf(postfp,"%d %d translate 90 rotate\n",spectro->xr[3],0);

    fprintf(postfp, "%d %d m (%s) o\n",
    (int) (spectro->yb[SPECTRUM] * .5 ) - 5 * 
    (int)((float)spectro->wchar * ps_scale),
    spectro->xr[3] - (grdox - (int)((float)spectro->wchar * ps_scale * 4)),
            "REL AMP (dB)");

   fprintf(postfp,"grestore \n");
  /* back to normal */
 
   
   fprintf(postfp, "%d %d m (%s) o\n",
    (int)(grdox + (grdxr - grdox) / 2   - spectro->wchar * 4 * ps_scale),
   yps - spectro->yb[SPECTRUM] + 1, "FREQ (kHz)");
   
   }/*postscript*/


   else {
   XDrawLine(info->display, info->pixmap,
          info->gc,grdox, grdoy, grdox, grdyb);
   XDrawLine(info->display, info->pixmap,
          info->gc,grdox, grdyb, grdxr, grdyb);
   XDrawLine(info->display, info->pixmap,
          info->gc,grdxr, grdoy, grdxr, grdyb);
   XDrawLine(info->display, info->pixmap,
          info->gc,grdox, grdoy, grdxr, grdoy);
   XDrawString(info->display,info->pixmap,info->gc,
   (int)grdox - 4 * spectro->wchar,grdoy + spectro->hchar,"dB",
      strlen("dB")  );
   XDrawString(info->display,info->pixmap,info->gc,
      grdox + (grdxr- grdox)/2 - (spectro->wchar * 4 ),
      spectro->yb[SPECTRUM] - 1,"FREQ (kHz)",
      strlen("FREQ (kHz)"));
    }

    /* gridlines */

    fmax = spectro->spers/2000.0;

    xinc = (float)(grdxr - grdox)/(spectro->spers/2000.0);

    fx =  (float)grdox + xinc ;

    x = fx + .5;
    x2 = fx - xinc/2.0 + .5;

    for( i = 1; i <=fmax; i++){
     sprintf(str,"%d", i);

      if(postfp != NULL){
      fprintf(postfp, " .5 setlinewidth\n");
      fprintf(postfp, "%d %d m %d %d k\n",
             x ,yps - grdoy ,x , yps - grdyb );
      fprintf(postfp, "%d %d m %d %d k\n",
             x2 ,yps - grdoy ,x2 , yps - grdyb );

      fprintf(postfp, " 1.0 setlinewidth\n");      
      fprintf(postfp, "%d %d m (%s) o\n",
             x - spectro->wchar / 2, 
             (int)(yps - (spectro->yb[SPECTRUM] - 
        (float)spectro->hchar * ps_scale)) + 1 ,str);
       
      }
      else{
       XDrawLine(info->display, info->pixmap,
          info->gc,x, grdoy, x, grdyb);
 

      XDrawLine(info->display, info->pixmap,
          info->gc,x2, grdoy, x2, grdyb);


         XDrawString(info->display,info->pixmap,info->gc,
          x - spectro->wchar / 2,
          spectro->yb[SPECTRUM] - spectro->hchar - 1,str,strlen(str));
       } 

       fx += xinc ;

       x = fx + .5;
       x2 = fx - xinc/2.0 + .5; 

     }/* vertical grid */

     yinc = (float)grdyb/SPEC_DB;  /* display 60 dB */

     y0 = grdyb - 5.0 * yinc + .5; /* offset zero */

     y = y0;

     x = grdox - 3 * spectro->wchar;
     for(i = 1; i <= 8; i++){

	sprintf(str,"%d", (i - 1) * 10);

      if(postfp != NULL){
       fprintf(postfp," .5 setlinewidth\n");
       fprintf(postfp, "%d %d m %d %d k\n",
            grdox ,yps - y ,grdxr , yps - y );
       fprintf(postfp," 1.0 setlinewidth\n");
       fprintf(postfp, "%d %d m (%s) o\n", 
               x, yps -  y - spectro->hchar / 2, str);
       
      }

       else{
         XDrawLine(info->display, info->pixmap,
          info->gc,grdox, y, grdxr, y);

         sprintf(str,"%d", (i - 1) * 10);
         XDrawString(info->display,info->pixmap,info->gc,
          x , y + spectro->hchar / 2, str, strlen(str));
       }
    
     fy = i * 10 * yinc;
     y =  y0 - fy - .5;
       
    }/* horizontal grid */
 
}/* end spectrum_pixmap */
/*************************************************************/
/*            new_spectrum(spectro)                          */
/*************************************************************/
void new_spectrum(XSPECTRO *spectro){

INFO *info;
int firstsamp;
int i;

    info = spectro->info[SPECTRUM];

    /* everytime there is a new spectrum set all the spectrum
       cursor values to -1 */

    spectro->specfreq = -1;
    spectro->specamp = -1.0;
    spectro->savspecfreq = -1;
    spectro->savspecamp = -1.0;


    /* if an average is currently being displayed
       switch back to previous mode this allows resize of averaging*/
 
    if(spectro->option == 'a' || spectro->option == 'A' ||
       spectro->option == 'k' ){
                  spectro->option = spectro->savoption;
                  spectro->history = 0; 
		}

   /* based on the spectrum combination */
   /* spectro->fltr is calculated if cursor is not to close to end of file*/

   /* look at the top of xkl.c for info on spectro->params[] */

   /* if the params value is the same for all spectra( ex.fd coeff)
      then it is accessed directly in quickspec otherwise it is
      set in this module (ex. sizwin) 
   */


    if(spectro->option == 'd'){
   /*DFT magnitude*/
    /* assume that sizwin and sizfft are compatible(setparam)*/
    spectro->sizwin = spectro->params[WD];
    spectro->sizfft = spectro->params[SD];
    spectro->type_spec = DFT_MAG;
    spectro->nsdftmag = (spectro->sizfft >> 1);
    firstsamp = spectro->saveindex - (spectro->sizwin >> 1);
    if(spectro->sizwin <= spectro->sizfft){
     if(firstsamp >= 0 && firstsamp <= spectro->totsamp - 1){
        spectro->spectrum = 1;
        quickspec(spectro,&spectro->iwave[firstsamp]);
     }/* new spectrum */
     else { spectro->spectrum = 0;
     }/* index too close to end of file for dft */
    }/* window size ok for dft */
    else{
      spectro->spectrum = 0;
      writetext("Window greater than DFT, reset parameters.\n");
      }/*spectro->sizwin > spectro->sizfft*/
     }/*d*/

    else if(spectro->option == 's'){
    /*spectrogram-like*/
      if(spectro->history) copysav(spectro);
      dofltr(spectro,SPECTROGRAM,spectro->params[WS]);
    }/*s*/

   else if(spectro->option == 'S'){
   /*spectrogram-like + DFT*/
    /*DFT savfltr*/
    savdft(spectro,spectro->params[WS]);/*make sizwin the same*/
    /*spectrogram-like fltr*/
    dofltr(spectro,SPECTROGRAM,spectro->params[WS]);
   }/*S*/

    else if(spectro->option == 'c'){
    /*critical-band*/
      if(spectro->history){
         copysav(spectro);
        }/*history*/
 
    dofltr(spectro,CRIT_BAND,spectro->params[WC]);
    }/*c*/

   else if(spectro->option == 'j'){
   /*critical-band + DFT*/
   /*DFT savfltr*/
    savdft(spectro,spectro->params[WC]);
   /*critical-band fltr*/
    dofltr(spectro,CRIT_BAND,spectro->params[WC]);
  }/*j*/


   else if(spectro->option == 'T'){
   /* put cb in fltr */ 
   dofltr(spectro,CRIT_BAND,spectro->params[WC]);
   /* if good spectrum,put slope in savfltr */
    if(spectro->spectrum){
     for(i = 0; i < spectro->lenfltr; i++)
           spectro->savfltr[i] = 0;
     tilt(&spectro->fltr[13],24,&spectro->savfltr[13]);
     /* make sure savfltr gets drawn */
      spectro->savspectrum = 1;
      spectro->lensavfltr = spectro->lenfltr;
      spectro->savspectrum = spectro->spectrum;
      spectro->savsizwin = spectro->sizwin;
      spectro->savspers = spectro->spers;
      strcpy(spectro->savname , spectro->wavename);
    }
   else {spectro->savspectrum = 0;}
   }/*T*/

   else if(spectro->option == 'l'){
   /*spectrogram-like + DFT*/
    /*DFT savfltr*/
    savdft(spectro,spectro->params[WL]);
    /*linear prediction*/
    dofltr(spectro,LPC_SPECT,spectro->params[WL]);
   }/*l*/

    /* store the value used for this spectrum for postscript*/

   spectro->fd = spectro->params[FD]; 
  
}/* end new_spectrum */
/****************************************************************/
/*                 copysav(XSPECTRO *spectro)                   */
/****************************************************************/
void copysav(XSPECTRO *spectro){
int i;
      spectro->lensavfltr = spectro->lenfltr;
      spectro->savspectrum = spectro->spectrum;
      spectro->savsizwin = spectro->sizwin;
      spectro->savspers = spectro->spers;
      spectro->savfd = spectro->fd;
      strcpy(spectro->savname , spectro->wavename);
      for( i = 0; i <= spectro->lensavfltr; i++){
         spectro->savfltr[i] = spectro->fltr[i];
         if(spectro->option == 'c')
	   spectro->savnfr[i] = spectro->nfr[i];
      }


}/* end copysav*/
/*****************************************************************/
/*         dofltr(XSPECTRO *spectro,int type, int winsize)         */
/*****************************************************************/
void dofltr(XSPECTRO *spectro,int type, int winsize) {
/* set up window size and check against dft size */
/* call quickspec and update fltr if cursor is not too close to end*/
int firstsamp;

    spectro->sizwin = winsize;
    spectro->sizfft = 256;
    if(spectro->sizwin > spectro->sizfft)
        spectro->sizfft = 512;
    spectro->type_spec = type;
    spectro->nsdftmag = spectro->sizfft / 2;
    firstsamp = spectro->saveindex - spectro->sizwin / 2;
    if(spectro->sizwin <= spectro->sizfft){
     if(firstsamp >= 0 && firstsamp <= spectro->totsamp - 1){
        spectro->spectrum = 1;
        quickspec(spectro,&spectro->iwave[firstsamp]);
      }/* new spectrum */
     else {
        spectro->spectrum = 0; 
      }/* index too close to end of file for dft */
     }/* window size ok for dft */
    else{
      spectro->spectrum = 0;
      writetext("Window greater that DFT reset parameters.\n");
      }/*spectro->sizwin > spectro->sizfft*/

   }/* end dofltr */
/**************************************************************/
/*                savdft(XSPECTRO *spectro, int winsize)      */
/**************************************************************/
void savdft(XSPECTRO *spectro,int winsize ){
int i,firstsamp;

/* do dft type spectrum and put it in savfltr (S and j and l)*/
    spectro->sizwin = winsize;
    spectro->sizfft = 256;
    if(spectro->sizwin > spectro->sizfft)
        spectro->sizfft = 512;    
    spectro->type_spec = DFT_MAG;
    spectro->nsdftmag = spectro->sizfft / 2;
    firstsamp = spectro->saveindex - spectro->sizwin / 2;
    if(spectro->sizwin <= spectro->sizfft){
     if(firstsamp >= 0 && firstsamp <= spectro->totsamp - 1){
        quickspec(spectro,&spectro->iwave[firstsamp]);
        spectro->lensavfltr = spectro->lenfltr;
        spectro->savspectrum = 1;
        spectro->savsizwin = spectro->sizwin;
        spectro->savspers = spectro->spers;
        strcpy(spectro->savname , spectro->wavename);
        /* Rms is in [spectro->lensavfltr] */
        for( i = 0; i <= spectro->lensavfltr; i++)
              spectro->savfltr[i] = spectro->fltr[i]; 
     }/* new spectrum */
     else {
        spectro->savspectrum = 0;
      }/* index too close to end of file for dft */
     }/* window size ok for dft */
    else{
      spectro->savspectrum = 0;
      writetext("Window greater that DFT reset parameters.\n");
      }/*spectro->sizwin > spectro->sizfft*/

}/* end savdft*/

/***************************************************************/
/*      draw_spectrum(spectro,int resize,FILE *postfp)         */
/***************************************************************/
void draw_spectrum(XSPECTRO *spectro, int resize, FILE *postfp)
{
  float grdox,grdoy,grdxr,grdyb;
  int arox1,arox2,aroy1,aroy2,aroy3;
  int i,n;
  XPoint points[4096];
  INFO *info;
  float xinc,yinc,x,y,halfsrate;
  int tmpfreq,asterisk;  /* freq of formants could be neg */
  int drawsav;  /* flag for drawing second fltr (or history)*/
  char str[200], str1[50],str2[50],str3[50],str4[50],str5[50];
  int ytext,y0,yps;
  float winms;
  char botlabel[30];
  float ps_scale;
  int drawarrows; /* flag for drawing formant markers*/
  time_t curtime;
  char datestr[50];
  int titleLoc;

  /* draw spectral data on screen or to postscript file, the resize */
  /* flag is so the formant info is written to the text window only once*/
  
  
  if (spectro->iwave == NULL) return;
  
  /* this string is for first difference */
  strcpy(str5,"");
  
  drawarrows = 1;
  if(postfp != NULL) { /* need to set ps_scale*/
    drawarrows = 0;   /* 'g' don't draw arrows */
    if(postfp == PS4_fp)  {
       ps_scale = spectro->Gscale;
       titleLoc = 305;
    }
    else {
       ps_scale = spectro->gscale;
       titleLoc = 330;
    }
  } 
  
  /*NOTE:  X considers y = 0 to be at the top of the screen,
    postscript considers y = 0 to be at the bottom of the page*/
  
  /* actually print label after getform */
  /* label for formant info */
  if(spectro->option == 's' || spectro->option == 'S' ){
    strcpy(botlabel,"Smoothed");  }
  else if(spectro->option == 'c' || spectro->option == 'j' ||
          spectro->option == 't' || spectro->option == 'T'){
    strcpy(botlabel,"CB"); }
  else if(spectro->option == 'l'){
    strcpy(botlabel,"LPC"); }

  info = spectro->info[SPECTRUM];  /*clear*/
  spectrum_pixmap(spectro->info[SPECTRUM],spectro,postfp);

  /* in most cases draw fltr and savfltr */
  drawsav = 1;
  if(spectro->option == 'd' ||
     ((spectro->option == 'c' || spectro->option == 's') &&
              !spectro->history )     ){
       /* just draw spectro->fltr */
       drawsav = 0;
     }     

  grdoy = 0;
  grdyb = (float)spectro->yb[SPECTRUM] - 2 * spectro->hchar - 1;
  
  if(postfp!=NULL){
     grdyb = (float)spectro->yb[SPECTRUM] - 
                   (float)spectro->hchar * 2.0 * ps_scale - 1;
  }
  

  grdox = (float)spectro->xr[SPECTRUM] * SPEC_OX;
  grdxr = (float)spectro->xr[SPECTRUM] * SPEC_XR;

     halfsrate = spectro->spers / 2.0; 

     yinc = (float) grdyb/SPEC_DB;   /*display 60 dB */

     y0 = grdyb - 5.0 * yinc + .5; /* offset zero */
     /* y offset for postscript(yfltr isn't flipped) */
     
     yps = (float)spectro->hchar + 1 + 5.0 * yinc + .5;
     if(postfp != NULL){
       yps = (float)spectro->hchar * ps_scale + 1 + 5.0 * yinc + .5;
     }

/* for postscript output put sampling rate in bottom right corner */
   if(postfp != NULL && (spectro->spectrum || spectro->savspectrum) ){
      ytext = grdyb - 4 * (float)spectro->hchar * ps_scale;

      sprintf(str1,"%.1fms",spectro->savetime);
      sprintf(str2,"SF = %d Hz",(int)spectro->spers);
      sprintf(str3,"FD = %d",spectro->fd);
      sprintf(str4,"SG = %d", spectro->params[SG]);
      sprintf(str5,"");
      ps_toplabel(spectro,postfp,grdxr,ytext,str1,str2,str3,str4,str5);
  
    /* put date and filename outside of bounding box */  
    time(&curtime);
    strcpy(datestr, ctime(&curtime)); datestr[24] = ' ';
      fprintf(postfp, "%d %d m (%s) o\n", 45, (int) 
	      ((titleLoc + 2*spectro->hchar) * ps_scale), 
	      datestr);
      fprintf(postfp, "%d %d m (File: %s) o\n", 45, (int) 
	      ((titleLoc + spectro->hchar) * ps_scale), 
	      spectro->wavefile);
      fprintf(postfp, "%d %d m (Current Dir: %s) o\n", 45, (int) 
	      (titleLoc * ps_scale), curdir);

   }/* file name for postscript */

if (spectro->spectrum) {    /* draw based on spectro->option */

/* draw fltr */
     x = 0.0;
     xinc = halfsrate / spectro->lenfltr;
     for(i = 0; i < spectro->lenfltr; i++){
       if(spectro->option == 'c' || spectro->option == 'j'||
          spectro->option == 'T' || spectro->option == 'l')
          x = spectro->nfr[i];
      /* save for postscript output*/
       spectro->xfltr[i] = x / halfsrate * (grdxr - grdox) ;
      points[i].x = spectro->xfltr[i] + .5 + grdox;
      spectro->yfltr[i] = (float)spectro->fltr[i]/10.0 * yinc;
      points[i].y = y0 - spectro->yfltr[i] + .5;
      x += xinc;    
     } 


   if (postfp != NULL) {      /* SMOOTHED SPECTRUM */

    /* KKG 4/16/98.  Added +12 to shift up spectrum */

      fprintf(postfp, " 1 setlinewidth\n");
      fprintf(postfp,"%f %f m \n",
	      spectro->xfltr[0] + grdox, spectro->yfltr[0] + yps + 12);
      for(i = 1; i < spectro->lenfltr - 1; i++)
	 fprintf(postfp,"%f %f l\n",
		 spectro->xfltr[i] + grdox,spectro->yfltr[i] + yps + 12);
      fprintf(postfp, " 1 setlinewidth\n");
   }
   else {
      XDrawLines(info->display,info->pixmap,
		 info->gc, points,spectro->lenfltr,CoordModeOrigin);
   }

  winms = (float)spectro->sizwin/spectro->spers * 1000.0;
  sprintf(str2,"win:%.1fms",winms);

/* do toplabel */
     if(spectro->option == 'd' || spectro->option == 'a' ||
	spectro->option == 'A' || spectro->option == 'k') {

	ytext =  grdoy + 2 * spectro->hchar;
	if (postfp != NULL) {
	   ytext =  grdoy + 2 * (float)spectro->hchar * ps_scale;
	}

	if (spectro->option == 'd') {
	   strcpy(str1,"DFT");
	   sprintf(str3,"F0 = %d Hz",
		   getf0(spectro->fltr,spectro->sizfft,(int)spectro->spers));
	   sprintf(str4,"Rms = %d dB",spectro->fltr[spectro->lenfltr]/10); 
	} /*DFT*/
	if (spectro->option == 'k' || spectro->option == 'a' ||
	    spectro->option == 'A') {
	   sprintf(str3,"start %d",spectro->avgtimes[0]);
	   sprintf(str4,"end %d",spectro->avgtimes[1]);
	   if (spectro->option == 'A') {
	      /* need to list all times below */
	      strcpy(str1,"Avg DFT-spect (A)");
	      strcpy(str3,"");
	      strcpy(str4,"");
	   }
	   else if (spectro->option == 'k') strcpy(str1,"Avg DFT-spect (kn)");
	   else if (spectro->option == 'a') strcpy(str1,"Avg DFT-spect (a)");
       
   }/*avg*/

    if(postfp != NULL){
      ps_toplabel(spectro,postfp,grdxr,ytext,str1,str2,str3,str4,str5);
     /* if 'A' list all times */
       if(spectro->option == 'A'){
       ytext = grdoy + 3 * (float)spectro->hchar * ps_scale; 
          for(i = 0; i <= spectro->avgcount; i++){
          ytext += (float)spectro->hchar * ps_scale;
          sprintf(str3,"time%d: %d",i+1,spectro->avgtimes[i]);
          fprintf(postfp, "%d %d m (%s) o\n",
         (int)grdxr + spectro->wchar,spectro->yb[SPECTRUM] - ytext,str3); 
        }/*times*/ 
       }/*'A'*/   
    }/* postscript */
   else{
      sc_toplabel(spectro,info,grdxr,ytext,str1,str2,str3,str4,str5);
   /* if 'A' list all times */
      ytext = grdoy + 3 * spectro->hchar; 
       if(spectro->option == 'A'){
         for(i = 0; i <= spectro->avgcount; i++){
          ytext += spectro->hchar;
          sprintf(str3,"time%d: %d",i+1,spectro->avgtimes[i]);
          XDrawString(info->display,info->pixmap,info->gc,
           (int)grdxr + spectro->wchar ,ytext,str3,strlen(str3)  ); 
	}/*times*/
       }/*'A'*/
    }/* screen */

 }/*toplabel*/

 
  /* find formants */
  if(spectro->option == 's' || spectro->option == 'S' ||
     spectro->option == 'l' || spectro->option == 'j'||
     spectro->option == 'c' || spectro->option == 'T'){

        /* formant arrows */
	arox1 = (float)spectro->xr[SPECTRUM] * .008;
	arox2 = (float)spectro->xr[SPECTRUM] * .004;
	aroy1 = (float)spectro->yb[SPECTRUM] * .02;
	aroy2 = (float)spectro->yb[SPECTRUM] * .015;
	aroy3 = (float)spectro->yb[SPECTRUM] * .045;

      ytext =  grdoy + 7 * spectro->hchar;
       if(postfp != NULL){
          ytext =  grdoy + 7 * (float)spectro->hchar * ps_scale;
       }

/*bottom label*/
if(postfp != NULL){
  
    fprintf(postfp, "%d %d m (%s) o\n",
    (int)grdxr + spectro->wchar,spectro->yb[SPECTRUM] - ytext,botlabel);  
    ytext += (float)spectro->hchar * ps_scale;
    fprintf(postfp, "%d %d m (%s) o\n",
      (int)grdxr + spectro->wchar,spectro->yb[SPECTRUM] - ytext,str2); 
    ytext += (float)spectro->hchar * ps_scale;

    fprintf(postfp, "%d %d m (%s) o\n", (int)grdxr + spectro->wchar,
	    spectro->yb[SPECTRUM] - ytext, "FREQ AMP");
 }/*postscript*/
else{
   XDrawString(info->display,info->pixmap,info->gc,
      (int)grdxr + spectro->wchar ,ytext,botlabel,strlen(botlabel)  );
    ytext += spectro->hchar;
   XDrawString(info->display,info->pixmap,info->gc,
      (int)grdxr + spectro->wchar ,ytext,str2,strlen(str2)  );
    ytext += spectro->hchar;
   XDrawString(info->display,info->pixmap,info->gc,
      (int)grdxr + spectro->wchar ,ytext,"FREQ AMP",
      strlen("FREQ AMP")  );
 }/*screen*/

    getform(spectro);
    /* formant info used to go to test window
       now it is printed out with spectrum values 'V'
    if(!resize && !postfp) 
         writetext("\n");
     */


    if(postfp != NULL){
      ytext += (float)spectro->hchar * ps_scale;
    }
    else{
    ytext += spectro->hchar;
    }
       for(i = 0; i < spectro->nforfreq; i++){
         tmpfreq = spectro->forfreq[i];
         asterisk = 0;
         if(tmpfreq < 0){
            asterisk = 1;
            tmpfreq = -tmpfreq;
	 }
         x = (float)tmpfreq / halfsrate * (grdxr - grdox) + grdox + .5;
         y = y0 - (float)spectro->foramp[i]/10. * yinc -
             spectro->hchar/2 ;

         points[0].x = x ;            points[0].y = y ;             
         points[1].x = x - arox1 ;    points[1].y = y - aroy1 ;
         points[2].x = x - arox2 ;    points[2].y = y - aroy2 ;
         points[3].x = x - arox2 ;    points[3].y = y - aroy3 ;
         points[4].x = x + arox2 ;    points[4].y = y - aroy3 ;
         points[5].x = x + arox2 ;    points[5].y = y - aroy2 ;
         points[6].x = x + arox1 ;    points[6].y = y - aroy1 ;
         points[7].x = x ;            points[7].y = y ;


         /* print formants */
           sprintf(str,"%d %d",tmpfreq,(int)(spectro->foramp[i]/10) );
         /* text window
           if(!resize && !postfp)
              {writetext(str); if(asterisk){writetext(" *");}writetext("\n");}
         */

       if(postfp != NULL){
         if (drawarrows) {
         fprintf(postfp," .5 setlinewidth\n");
         fprintf(postfp,"%d %d m \n",
              points[0].x,spectro->yb[SPECTRUM] - points[0].y);
         for(n = 1; n < 8; n++)
                     {fprintf(postfp,"%d %d l\n", points[n].x,
                                 spectro->yb[SPECTRUM] - points[n].y);}
         fprintf(postfp," 1.0 setlinewidth\n");
	 }
         if (asterisk) {
	   fprintf(postfp, "%d %d m (%s) o\n",
           (int)grdxr  ,spectro->yb[SPECTRUM] - ytext,"*");}
         fprintf(postfp, "%d %d m (%s) o\n",(int) grdxr + spectro->wchar, 
		 spectro->yb[SPECTRUM] - ytext,str); 
       } /*write postscript*/
      else{
       XDrawLines(info->display,info->pixmap,
          info->gc, points,8,CoordModeOrigin);
       if(asterisk){XDrawString(info->display,info->pixmap,info->gc,
               (int)grdxr  ,ytext,"*",1  );}
       XDrawString(info->display,info->pixmap,info->gc,
             (int)grdxr + spectro->wchar ,ytext,str,strlen(str)  );

        }/* draw to screen*/


         if(postfp != NULL){
                ytext += (float)spectro->hchar * ps_scale;
	 }
         else {
                ytext += spectro->hchar;
	 }
	 }/* peaks */
       } /* arrows */

}/*good spectrum*/

 if(drawsav && spectro->savspectrum){ 
     x = 0.0;
     xinc =  spectro->savspers / 2.0 / spectro->lensavfltr;
     spectro->savclip = 0;
     for(i = 0; i < spectro->lensavfltr; i++){
       if(spectro->option == 'c' || spectro->option == 'T')
       {
	 	if(spectro->option == 'T')
	 		{
				x = spectro->nfr[i];
     		}
     	else
         	{
          		x = spectro->savnfr[i];
	 		}
	 		}
       spectro->xsav[i] = x / halfsrate * (grdxr - grdox); 
       points[i].x = spectro->xsav[i] + .5 + grdox;    
       if(points[i].x <=  (int)grdxr  ){
           spectro->ysav[i] = (float)spectro->savfltr[i]/10. * yinc;      
           points[i].y =  y0 - spectro->ysav[i] + .5 ; 
           spectro->savclip++;
	 }/* clip if drawing 8k on 5k */ 
      x += xinc;    
     }

/* do toplabel*/
if(spectro->option == 'S' || spectro->option == 'j' ||
   spectro->option == 'l' || spectro->option == 's' ||
   spectro->option == 'c'){

      winms = (float)spectro->savsizwin/spectro->savspers * 1000.0;
      sprintf(str2,"win:%.1fms",winms);
      ytext =  grdoy +  1 * spectro->hchar;


    if(spectro->option == 's' || spectro->option == 'c'){
    /* do toplabel, History*/

      strcpy(str1,"History");
      if(postfp != NULL){
       strcpy(str1,"Dashed");                   
      }
      strcpy(str3,spectro->savname);
      sprintf(str4,"%.1f ms",spectro->oldtime);
      if(spectro->savfd != -1){
        sprintf(str5,"FD = %d",spectro->savfd);
      }

     }/*toplabel, History*/

    else if(spectro->option == 'S' || spectro->option == 'j' ||
      spectro->option == 'l' ){
      /* do toplabel, DFT F0 rms */
      strcpy(str1,"DFT");
      /* assume that the DFT was done with same sizfft as fltr */
      sprintf(str3,"F0 = %d Hz",
              getf0(spectro->savfltr,spectro->sizfft,(int)spectro->spers));
      sprintf(str4,"Rms = %d dB",spectro->savfltr[spectro->lensavfltr]/10); 
    }
 
   if(postfp != NULL){
      ps_toplabel(spectro,postfp,grdxr,ytext,str1,str2,str3,str4,str5);
    }/* postscript */
   else{
      sc_toplabel(spectro,info,grdxr,ytext,str1,str2,str3,str4,str5);
    }/* screen */


}/* toplabel , history or DFT with c,s,l */


/* draw sav spectrum*/

     if(postfp != NULL){ /* DFT spectra */
	fprintf(postfp, " 1 setlinewidth\n");
	if(spectro->option == 's' || spectro->option == 'c'){
	   fprintf(postfp,"[2] 0 setdash\n");
	}/* dashed line */
	fprintf(postfp,"%f %f m \n",
           spectro->xsav[0] + grdox,spectro->ysav[0] + yps);

    /* KKG 4/16/98.  Added +12 to shift up spectrum */

         for(i = 1; i < spectro->savclip - 1; i++)
                     fprintf(postfp,"%f %f l\n",spectro->xsav[i] + 
                      grdox, spectro->ysav[i] + yps + 12);
     if(spectro->option == 's' || spectro->option == 'c'){
        fprintf(postfp,"[] 0 setdash\n");
     }/* dashed line */
     fprintf(postfp, " 1 setlinewidth\n");
 }
  else{
   XDrawLines(info->display,info->pixmap,
          info->gc, points,spectro->savclip,CoordModeOrigin);
 }

 }/* savfltr and good savspectrum*/


if(  (spectro->option == 'c' || spectro->option == 's') &&
              !spectro->history   ){ 
               spectro->history = 1;
               spectro->savspectrum = 0; }

}/* end draw_spectrum */

/*************************************************************************/
/*ps_toplabel(spectro, postfp, grdxr,ytext, str1, str2, str3,str4)       */
/*************************************************************************/

void ps_toplabel(XSPECTRO *spectro, FILE *postfp, float grdxr,int ytext, char *str1, char *str2, char *str3, char *str4, char *str5)
{

  float ps_scale;
  if (postfp != NULL) {/* need to set ps_scale*/
    if(postfp == PS4_fp) ps_scale = spectro->Gscale;
    else ps_scale = spectro->gscale;
  }
    

  fprintf(postfp, "%d %d m (%s) o\n",
	  (int)grdxr + spectro->wchar,spectro->yb[SPECTRUM] - ytext,str1);
  ytext += (float)spectro->hchar * ps_scale;
  fprintf(postfp, "%d %d m (%s) o\n",
	  (int)grdxr + spectro->wchar,spectro->yb[SPECTRUM] - ytext,str2);
  ytext += (float)spectro->hchar * ps_scale;
  fprintf(postfp, "%d %d m (%s) o\n",
	  (int)grdxr + spectro->wchar,spectro->yb[SPECTRUM] - ytext,str3);
  ytext += (float)spectro->hchar * ps_scale;
  fprintf(postfp, "%d %d m (%s) o\n",
	  (int)grdxr + spectro->wchar,spectro->yb[SPECTRUM] - ytext,str4);
  if(strlen(str5) >= 1){
    ytext += (float)spectro->hchar * ps_scale;
    fprintf(postfp, "%d %d m (%s) o\n",
	    (int)grdxr + spectro->wchar,spectro->yb[SPECTRUM] - ytext,str5);
  }/* only print out FD with s or c */
  
 
}/* end ps_toplabel */
/**************************************************************************/
/*sc_toplabel(spectro, info, grdxr,ytext,str1, str2, str3,str4)*/
/**************************************************************************/
void sc_toplabel(XSPECTRO *spectro, INFO *info, float grdxr,int ytext,char *str1, char *str2, char *str3,char *str4, char *str5)
{
  
  XDrawString(info->display,info->pixmap,info->gc,
	      (int)grdxr + 2 * spectro->wchar ,ytext,str1,strlen(str1)  ); 
  ytext += spectro->hchar;
  XDrawString(info->display,info->pixmap,info->gc,
	      (int)grdxr + 2 * spectro->wchar ,ytext,str2,strlen(str2) );
  ytext += spectro->hchar;
  XDrawString(info->display,info->pixmap,info->gc,
	      (int)grdxr + 2 * spectro->wchar ,ytext,str3,strlen(str3) );
  ytext += spectro->hchar;
  XDrawString(info->display,info->pixmap,info->gc,
	      (int)grdxr + 2 * spectro->wchar ,ytext,str4,strlen(str4) );
  if(strlen(str5) >= 1){
    ytext += spectro->hchar;
    XDrawString(info->display,info->pixmap,info->gc,
		(int)grdxr + 2 * spectro->wchar ,ytext,str5,strlen(str5) );
  }
}

/*****************************************************************/
/*                           
Mel scale with lin/log break at 700.  Seems consistent with Klatt's
PlotSpec while keeping 1000 Mel at 1000 Hz.  Klatt didn't actually
specify a factor.  He just used relative Mel values to determine
placement along the X axis.   g.l
	real*4 mel,melfac
	mel(f) = melfac * log10(1. + (f/700.))
	data melfac /2595.0375/     ! = 1000./log10(1.+1000./700.)* 
*/
/*******************************************************************/
/*wave_pixmap(INFO *info,XSPECTRO *spectro,int grid, int index, FILE *postfp);*/
/*******************************************************************/
void wave_pixmap(INFO *info,XSPECTRO *spectro,int grid, int index, FILE *postfp){
/* draw waveform on screen or to post script file */
/* index indicates which window is to be drawn */
/* grid states: 0 not to print grid,1 print grid */
XPoint *points; /*    short x, y  */
int n,i,l,k;
int t,inc;
int xmax, ymax;
float xinc,x;
int yps ; /* switch X ycoord to postscript ycoord */
float map,min,max,mid;
Region region;
XRectangle rectangle;

/* index = which window */

/* get window size in pixels */
    xmax = findxmax(spectro,index);
    ymax = findymax(spectro,index);


yps = spectro->yb[index] ;

/* load into temp array */
i = spectro->startindex[index];  
t = xoffset(spectro) - 1;

  /* might be upside down need to check */

  if(xmax > spectro->sampsused[index]){
     inc = xmax / spectro->sampsused[index];
     spectro->step = 1;
     }
  else{
     spectro->step = spectro->sampsused[index] / xmax ;
     inc = 1; 
  }

  points = (XPoint *) malloc( sizeof(XPoint) * spectro->sampsused[index]);
  
  mid = spectro->wavemax - (spectro->wavemax - spectro->wavemin) / 2.0;
  max = mid + 
  (spectro->wavemax - spectro->wavemin) / 2.0 * spectro->wvfactor[index];
  min = mid - 
  (spectro->wavemax - spectro->wavemin) / 2.0 * spectro->wvfactor[index];
 

  map = (float)ymax / (max - min );

 i = spectro->startindex[index]; 

 xinc = (float)xmax / (float)spectro->sampsused[index];
 x = xoffset(spectro);

 for(n = 0; n <  spectro->sampsused[index]; n++){


         points[n].y = (short)((bborder(spectro,index) - 
                    spectro->axisdist - 1) -
         (map * ((float)(spectro->iwave[i] >> 4) - min) + .5)) ;


         points[n].x = x + .5;

         x += xinc;
         i++;
   }/* all points */


 if(postfp != NULL) { 
   if (grid)  draw_axes(spectro,info,index,postfp);

/* set up clipping if the y axis is scaled */
  rectangle.x = xoffset(spectro);  rectangle.width = xmax;
  rectangle.y = yoffset(spectro) - 1;  rectangle.height = ymax;
  

  fprintf(postfp," %d %d %d %d rectclip\n",
  xoffset(spectro),yps - (bborder(spectro,index) - spectro->axisdist  - 1), 
  xmax , ymax);

  fprintf(postfp," 1 setlinewidth\n");

  fprintf(postfp,"%d %d m\n",points[0].x, yps - points[0].y);
  fprintf(postfp,"/x %f def\n",  (float)points[0].x + xinc);
  
  fprintf(postfp,"/str 20 string def\n");
  fprintf(postfp,
  "%d{currentfile str readline pop cvi dup x exch k x exch m \n",
                (spectro->sampsused[index] - 1) );

  fprintf(postfp,"x %f add /x exch def}repeat\n",xinc);
  
   
    n = 0;
    for(i = 1; i < spectro->sampsused[index]; i ++){
       /*if(n == 20){ fputc('\n',postfp); n = 0;}*/
      fprintf(postfp,"%d\n",yps - points[i].y);
     n++;
     }

  fprintf(postfp,"\n");

  fprintf(postfp," 1.0 setlinewidth\n");

}/* write values out to postscript file */


  else{
   /* draw to screen*/

   /*clear*/
   XSetForeground(info->display,info->gc,
                  info->color[2].pixel);

   XFillRectangle(info->display,info->pixmap,
     info->gc, 0,0,
     spectro->xr[index],spectro->yb[index]); 


   XSetForeground(info->display,info->gc,
                  info->color[1].pixel);


/*
   XDrawLine(info->display,info->pixmap,
          info->gc,xoffset(spectro) - spectro->axisdist,
                  y1,rborder(spectro,index),y1);
*/

   draw_axes(spectro,info,index,NULL);

  /* set up clipping region if the y axis is scaled */
  if(spectro->wvfactor[index] != 1.0){
  region = XCreateRegion();
  rectangle.x = xoffset(spectro);  rectangle.width = xmax;
  rectangle.y = yoffset(spectro) - 1;    rectangle.height = ymax;
  XUnionRectWithRegion(&rectangle,region, region);
  XSetRegion(info->display,info->gc,region);
  }

if(spectro->sampsused[index] <= 10000){
           XDrawLines(info->display,info->pixmap,
            info->gc,points,spectro->sampsused[index],CoordModeOrigin);
	 }
else{

  /* limit number of points sent to drawlines to 10000 (sgi)*/
   k = spectro->sampsused[index] / 10000;

      for(l = 0; l < k;  l++){
          if( l == 0){
            XDrawLines(info->display,info->pixmap,
            info->gc, &points[ l * 10000],10000,CoordModeOrigin);
	  }
      
          else{
            XDrawLines(info->display,info->pixmap,
            info->gc, &points[ l * 10000 - 1],10000 + 1,CoordModeOrigin);
	  }
	}

/*    leftovers   */
     XDrawLines(info->display,info->pixmap,
            info->gc, &points[ l * 10000 - 1],
            (spectro->sampsused[index] % 10000) + 1,CoordModeOrigin);

 }/* send to drawlines in chunks */

  if(spectro->wvfactor[index] != 1.0){
        XSetClipMask(info->display,info->gc,None);
        XDestroyRegion(region);
   
}

 }/* draw to screen */

   free(points);

 }/* end wave_pixmap */

/***********************************************************************/
/*void dft(short iwave[],float win[],float *dftmag,int npts1,int npts2,*/
/*                        int fdifcoef){                               */
/***********************************************************************/
 
/*      DFT.C         D.H. Klatt */

/*      Compute npts2-point dft magnitude spectrum (after M.R. Portnoff) */

/*        Input: npts1 fixed-point waveform samples in "iwave[1,...,npts1]"
                  and a floating-point window in "win[1,...,npts1]"

		  If npts1 > npts2, fold waveform re npts2
CHANGE 2-Jan-87:  For first difference preemphasis, set fdifsw = 1  END OUT
		  For first difference preemphasis, set fdifcoef = {0,...,100}
		   where 0 is no preemphasis, and 100 is exact first difference.
		  For no windowing and no preemphasis, fdifcoef = -1 */

/*        Output: npts2/2 floating-point spectral magnitude-squared
                  samples returned in floating array dftmag[0,...,npts2/2-1]
		  (npts2 must be a power of 2, and npts1 must be even) */


void dft(short iwave[],float win[],float *dftmag,int npts1,int npts2,
         int fdifcoef){

        float xrsum;  /* this used to be a global*/
        int npts,mn,nskip,nsize,nbut,nhalf;
        float theta,c,s,wr,wi,tempr,dsumr,dsumi,scale,xre,xro;
        float xie,xio;
        float xri,xii,xfdifcoef;
        int i1,i,j,k,n,nrem,itest,isum,nstage,tstage;
        float *xr, *xi;
        float xtemp;

/*    Multiply waveform by window and place alternate samples
      in xr and xi */

        npts = npts1 >> 1;
	npts2 = npts2 >> 1;
        xr = dftmag;
        xi = dftmag + npts2;


	i1 = 0;
	xfdifcoef = 0.01 * (float)fdifcoef;
        for (i=0; i<npts2; i++) {
	    xr[i] = 0.;
	    xi[i] = 0.;
	}
        for (i=0; i<npts; i++) {
            j = i + i;
            k = j + 1;

            if (fdifcoef >= 0) {
/*	      Use first difference preemphasis and window function */
          xri = (float) (iwave[j] - (xfdifcoef * (float) iwave[j-1]));
          xii = (float) (iwave[k] - (xfdifcoef * (float) iwave[j]));
	        xr[i1] += xri * win[j];
	        xi[i1] += xii * win[k];
	}
	    else if (fdifcoef == -1) {
/*	      No first-difference preemphasis, window contains lpc coefs */
		xr[i1] += win[j];
		xi[i1] += win[k];
	      }
            else {
/*	      No first difference preemphasis, use window function */
                xri = iwave[j];
                xii = iwave[k];
	        xr[i1] += xri * win[j];
	        xi[i1] += xii * win[k];
	      }

	    i1++;
	    if (i1 >= npts2)    i1 = 0;
	  }
 
/*    Bit-reverse input sequence */
        i = 1;
        n = npts2 >> 1;
        while (i < (npts2 - 1)) {
                j = n;
                if (i < j) {
                    xtemp = xr[j];
                    xr[j] = xr[i];
                    xr[i] = xtemp;
                    xtemp = xi[j];
                    xi[j] = xi[i];
                    xi[i] = xtemp;
                }
                nrem = n;
                itest = npts2;
                isum = -npts2;
                while (nrem >= 0) {
                    isum = isum + itest;
                    itest = itest >> 1;
                    nrem = nrem - itest;
                }
                n = n + itest - isum;
                i = i + 1;
	      }
/*    End of bit-reversal algorithm */



/*    Begin FFT */
        nstage = 0;
        nskip = 1;
        while (nskip < npts2) {
  loop:     nstage++;
            tstage = nstage;/* scope problem with -O, d.h.*/
            nsize = nskip;
            nskip = nskip << 1;
/*        The following loop does butterflies which do not
          require complex arith */
            for (i = 0; i < npts2; i += nskip) {
                j = i + nsize;
                xtemp = xr[j];
                xr[j] = xr[i] - xr[j];
                xr[i] = xr[i] + xtemp;
                xtemp = xi[j];
                xi[j] = xi[i] - xi[j];
                xi[i] = xi[i] + xtemp;
	      }
            if(tstage < 2){
              goto loop;
	    }
            theta = TWOPI / nskip;
            c = cos(theta);
            s = sin(theta);
            wr = 1.0;
            wi = 0.0;

/*    Do remaining butterflies */
            for (nbut = 2; nbut <= nsize; nbut++){
                tempr = wr;
                wr = (wr * c) + (wi * s);
                wi = (wi * c) - (tempr * s);
                for(i = nbut - 1; i < npts2; i += nskip) {
                    j = i + nsize;
                    dsumr = (xr[j] * wr) - (xi[j] * wi);
                    dsumi = (xi[j] * wr) + (xr[j] * wi);
                    xr[j] = xr[i] - dsumr;
                    xi[j] = xi[i] - dsumi;
                    xr[i] += dsumr;
                    xi[i] += dsumi;
		  }
	      }
         }

/*    Set coefficients for scrambled real input */
        scale = 0.5;
        wr = 1.0;
        wi = 0.0;
        nhalf = npts2 >> 1;
        tempr = xr[0];
        xr[0] = tempr + xi[0];
        xi[0] = tempr - xi[0];
        theta = TWOPI / (npts2+npts2);
        c = cos(theta);
        s = sin(theta);
        for (n=1; n<nhalf; n++) {
            tempr = wr;
            wr = (wr*c) + (wi*s);
            wi = (wi*c) - (tempr*s);
            mn = npts2 - n;
            xre = (xr[n] + xr[mn]) * scale;
            xro = (xr[n] - xr[mn]) * scale;
            xie = (xi[n] + xi[mn]) * scale;
            xio = (xi[n] - xi[mn]) * scale;
            dsumr = (wr*xie) + (wi*xro);
            dsumi = (wi*xie) - (wr*xro);
            xr[n] = xre + dsumr;
            xi[n] = xio + dsumi;
            xr[mn] = xre - dsumr;
            xi[mn] = dsumi - xio;
	  }

/*    Take magnitude squared */
        xi[0] = 0.;
        xrsum = 0.;
        for (i=0; i<npts2; i++) {
            xr[i] = 0.000000001 * ((xr[i] * xr[i]) + (xi[i] * xi[i]));
            xrsum = xrsum + xr[i];
	  }

      }/* end */


/*******************end Dennis' dft.c*************************/



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*		         Q U I C K S P E C				 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* edited to use spectro structure July '93 d. hall*/

int quickspec(XSPECTRO *spectro, short iwave[])
{

  float scratch[1024];
  double pcerror, energy, floatw;
  static float floatwlast, xfdcoef;
  int ier,iwlast,count;
  int pdft,pdftend,nch,pw;
  long nsum;
  float sum,sumlow;


  if (spectro->type_spec == DFT_MAG ||
      spectro->type_spec == DFT_MAG_NO_DB) {
    
    spectro->lenfltr = spectro->sizfft / 2;
    /*makefbank(spectro);*/
  }/*DFT_MAG or DFT_MAG_NO_DB*/
  

  /*    Make filters */
  else if (spectro->type_spec == CRIT_BAND) {
    spectro->lenfltr = spectro->sizfft / 2;
    makefbank(spectro);
    spectro->lenfltr = spectro->nchan;
  }/*CRIT_BAND*/

  else if ((spectro->type_spec == SPECTROGRAM) ||
	   (spectro->type_spec == SPECTO_NO_DFT)) {
    spectro->lenfltr = spectro->sizfft / 2;
    makefbank(spectro);
  }/*SPECTROGRAM or SPECTO_NO_DFT*/

  /*    Make Hamming window  */
  /*does a new window need to be calculated*/
  if( spectro->oldsizwin != spectro->sizwin ||
      spectro->oldwintype!= spectro->params[NW]){
    
    spectro->oldsizwin = spectro->sizwin;
    spectro->oldwintype = spectro->params[NW];
    
    /* need to think about different sized ffts */
    
    mkwnd(spectro);
  }


  /*    Linear Prediction */
  if (spectro->type_spec == LPC_SPECT) {
    spectro->lenfltr = (spectro->sizfft >> 1);
    
    for(nch = 0; nch < 1024; nch++)
      scratch[nch] = 0.0;
    
    spectro->nchan = spectro->lenfltr;

    makefbank(spectro);	/* Actually just set nfr[] */
           

    /*	  Multiply waveform times window, do first difference, use dftmag[] */
    iwlast = 0;
    xfdcoef = 0.01 * (float)(spectro->params[FD]);
    for (nch = 1; nch < spectro->sizfft; nch++) {
      if (spectro->params[FD] > 0) {
	floatw = (float)iwave[nch];
	floatwlast = iwave[nch - 1];
	/*  Use 0.95 to make unbiased estimate of F1  for synthetic stim */
	spectro->dftmag[nch] = spectro->hamm[nch] *
          (floatw - (xfdcoef * floatwlast));
      }
      else {
	spectro->dftmag[nch] = spectro->hamm[nch] * 
	  (float)(iwave[nch]);
      }
    }
    
    
    /* Call Fortran lpc analysis subroutine, coefs in dftmag[256]... */
    pcode(spectro->dftmag,spectro->sizwin,spectro->params[NC],
	  &spectro->dftmag[spectro->sizfft],
	  scratch,&pcerror,&energy,&ier);
    
    
    /*	  Move coefs down 1, make negative */
    for (nch= spectro->params[NC] - 1; nch >= 0; nch--) {
      spectro->dftmag[nch+ spectro->sizfft + 1] = - 
	(spectro->dftmag[nch+spectro->sizfft]);
    }
        
    
    spectro->dftmag[spectro->sizfft] = 1.0;
    if (ier != 0) {
      printf("Loss of significance, ier=%d\n", ier);
    }
    
    /*	  Pad acoef with zeros for dft */
    count = (spectro->sizfft << 1);
    for(nch=spectro->params[NC]+spectro->sizfft+1; nch <= count; nch++) {
      spectro->dftmag[nch] = 0.;
    }
    /*	  Call dft routine with firstdifsw=-1 to indicate lpc */
    dft(iwave,&spectro->dftmag[spectro->sizfft],spectro->dftmag,
	spectro->sizwin,spectro->sizfft,-1);
    
    
  }/* Linear Prediction*/
  

  /*    Compute magnitude spectrum */
  else if (spectro->type_spec != SPECTO_NO_DFT) {
 
    dft(iwave,spectro->hamm,spectro->dftmag,
	spectro->sizwin,spectro->sizfft,spectro->params[FD]);
  }

/*  DFT now done in all cases, next create filters */

  /*    If DFT magnitude spectrum, just convert to dB */
  if (spectro->type_spec == DFT_MAG) {
    /*	  Consider each dft sample as an output filter */
    for (nch=0; nch<spectro->lenfltr; nch++) {
      /* Scaled so smaller than cb */
      nsum = spectro->dftmag[nch] * 20000.;
      spectro->fltr[nch] = mgtodb(nsum,spectro->params[SG]);
    }
  }/*DFT_MAG*/

  /*    If LPC: invert, multiply by energy, and convert to dB */
  else if (spectro->type_spec == LPC_SPECT) {
    /*	  Multiply coefs by square root of energy */
    spectro->denergy = (double)energy * (double)pcerror;
	    spectro->logoffset =(int)
	      (-1450.0 + 100. * log10(spectro->denergy) + .5);
	    /*	  Consider each dft sample as an output filter */
	    for (nch = 0; nch<spectro->lenfltr; nch++) {
	      spectro->denergy = spectro->dftmag[nch];
	      spectro->fltr[nch] = (int)(spectro->logoffset - 100. * 
					 log10(spectro->denergy));
	      if (spectro->fltr[nch] < 0)    spectro->fltr[nch] = 0;
	    }
	    return(0);
  }/*LPC_SPEC*/

  /* Sum dft samples to create Critical-band or Spectrogram filter outputs */

	else if ((spectro->type_spec == CRIT_BAND)
	 || (spectro->type_spec == SPECTO_NO_DFT)
	 || (spectro->type_spec == SPECTROGRAM)) {
            pw = 0;
	    for (nch=0; nch<spectro->lenfltr; nch++) {
                pdft = spectro->nstart[nch];
                pdftend = pdft + spectro->ntot[nch];
                sum = SUMMIN;
/*            Compute weighted sum of relevent dft samples */
                while (pdft < pdftend) {
                    sum += cbskrt[spectro->pweight[pw++]] * 
                    spectro->dftmag[pdft++];
		  }
/*	      Add in weak contribution of high-freq channels */
/*	      Approximate tail of filter skirt by exponential */

	   spectro->attenpersamp = (0.4 * spectro->spers) /
                            (spectro->nbw[nch] * spectro->nsdftmag);
	    spectro->attenpersamp = 1.0 - spectro->attenpersamp;

                if(spectro->attenpersamp > 1.0){spectro->attenpersamp = 1.0;}
                else if(spectro->attenpersamp < 0.0)
                    {spectro->attenpersamp = 0.0;}
		if (pdftend < spectro->lenfltr) {
		    sumlow = 0.;
		    pdft = spectro->lenfltr - 1;
		    while (pdft >= pdftend) {
                    sumlow = (sumlow + spectro->dftmag[pdft--]) 
                    * spectro->attenpersamp;
                    
                    }
		    sum += (sumlow * cbskrt[spectro->sizcbskirt-1]);
		}
/*	      Add in weak contribution of low-freq channels   */
/*	      Approximate tail of filter skirt by exponential */
                pdftend = spectro->nstart[nch];
		if (pdftend > 0) {
		    sumlow = 0.;
		    pdft = 0;
		    while (pdft < pdftend) {
                    sumlow = 
                    (sumlow + spectro->dftmag[pdft++]) * spectro->attenpersamp;
                    }
		    sum += (sumlow * cbskrt[(spectro->sizcbskirt-1)>>1]);
		}
		if (spectro->type_spec == SPECTO_NO_DFT) {
   	            sum = sum * 18000.;	/* Scaled so bigger than dft */
		    nsum = sum;
		    spectro->fltr[nch] = mgtodb(nsum,spectro->params[SG]);
		}
		else {
   	            sum = sum * 18000.;	/* Scaled so bigger than dft */
		    nsum = sum;
		    spectro->fltr[nch] = mgtodb(nsum,spectro->params[SG]);
/*		  Look for maximum filter output in this frame */
	        if (spectro->fltr[nch] > spectro->fltr[spectro->lenfltr + 1]) {
		 spectro->fltr[spectro->lenfltr + 1] = spectro->fltr[nch];
		      }
		  }
	      }
	  }

/*    Compute overall intensity and place in fltr[lenfltr] */

        sum = SUMMIN;
        for (pdft=0; pdft < spectro->sizfft>>1; pdft++) {
            sum = sum + spectro->dftmag[pdft];
	  }
        nsum = sum * 10000.;	/* Scale factor so synth av=60 about right */
	
        spectro->fltr[spectro->lenfltr] = mgtodb(nsum,spectro->params[SG]);

return(0);
      }/* end quickspec */


/**********************************************************************/
/*           void   makefbank(XSPECTRO *spectro)                       */
/**********************************************************************/

/* used to take type_spec as a parameter */

    void makefbank(XSPECTRO *spectro) {
    int np,i1,i3,nsmin,n1000,nfmax;
    float x2,xfmax,fcenter,bwcenter,bwscal,bwscale,xbw1000,xsdftmag,linlogfreq;
    double a,b,c,d,temp1,temp2;


/* Initialization to make filter skirts go down further (to 0.00062) 
   put this in main, just do it once at startup 
    if (spectro->initskrt == 0) {
	spectro->initskrt++;

	for (np = 100; np<SIZCBSKIRT; np++) {
	    cbskrt[np] = 0.975 * cbskrt[np-1];
	}
	printf("\nMax down of cbskrt[] is %f  ",
                         cbskrt[SIZCBSKIRT-1]); 
    }
*/



/*    Highest frequency in input dft magnitude array */
	nfmax = (int)spectro->spers / 2;
	xfmax = nfmax;
	xsdftmag = spectro->nsdftmag;



if ((spectro->type_spec == LPC_SPECT) || ( spectro->type_spec== AVG_SPECT)) {

/*	    printf("\tCompute freq of each dft mag sample (nfr[%d])",
	     spectro->lenfltr); */
/*	  Reset nfr[] */
	    i3 = spectro->lenfltr / 2;	/* For rounding up */
	    for (i1=0; i1<spectro->lenfltr; i1++) {
		spectro->nfr[i1] = ((i1 * nfmax) + i3) / spectro->lenfltr;
	    }
	    return;
	  }

	if ( spectro->type_spec == CRIT_BAND) {
/*	  Do not include samples below 80 Hz */
	    nsmin = ((80 * spectro->nsdftmag) / nfmax) + 1;
/*	  Determine center frequencies of filters, use frequency spacing of
 *	  equal increments in log(1+(f/flinlog)) */
            linlogfreq = (float) spectro->params[FL];
	    temp1 = 1. + ((float)spectro->params[F1]/linlogfreq);
	    temp1 = log(temp1);
	    temp2 = 1. + ((float)spectro->params[F2]/linlogfreq);
	    temp2 = log(temp2);
	    a = temp2 - temp1;
	    b = (temp1/a) - 1.;
	    c = (xsdftmag*linlogfreq)/xfmax;
	}
	else {
	    nsmin = 1;
	}
	n1000 = (1000.0 * spectro->nsdftmag)/nfmax;
	xbw1000 = spectro->params[B1];
	xbw1000 = xbw1000/n1000;
	bwscal = (360.*xfmax)/xsdftmag;		/* 360 is a magic number */

	np = 0;
	spectro->nchan = 0;
	while (spectro->nchan < NFMAX) {

	if (spectro->type_spec == SPECTROGRAM || 
            spectro->type_spec == SPECTO_NO_DFT) {
/*	      Center frequency in Hz of filter i */
		fcenter = spectro->nchan;
/*	      Bandwidth in Hz of filter i */
		bwcenter = spectro->params[BS];
	    }

	    else {
/*	      Center frequency in Hz of filter i */

		d = a*(b + spectro->nchan + 1.0);
		fcenter = c * (exp(d) - 1.);

/*	      Bandwidth in Hz of filter i */
		bwcenter = xbw1000*fcenter;
		if (bwcenter < (float)spectro->params[B0]) {
		    bwcenter = spectro->params[B0];
	        }
	      }
/*	  See if done */
	    if (fcenter >= xsdftmag) {
		break;
	    }/*	  Center frequency and bandwidth in Hz of filter i */
	    spectro->nfr[spectro->nchan] = ((fcenter * xfmax) / xsdftmag) + .5;
	    spectro->nbw[spectro->nchan] = bwcenter;

/* printf("\nFILTER %d:  f=%4d bw=%3d:\n", nchan, nfr[nchan], nbw[nchan]); */

/*	  Find weights for each filter */
	    spectro->nstart[spectro->nchan] = 0;
	    spectro->ntot[spectro->nchan] = 0;

	    bwscale = bwscal/(10. * spectro->nbw[spectro->nchan]);
	    for (i1 = nsmin; i1 < spectro->nsdftmag; i1++) {
/*	      Let x2 be difference between i1 and filter center frequency */
		x2 = fcenter - i1;
		if (x2 < 0) {
		    x2 = -x2;
		    spectro->sizcbskirt = SIZCBSKIRT;	
                  /* Go down 22 dB on low side */
		}
		else spectro->sizcbskirt = SIZCBSKIRT>>1;	
                    /* Go down 32 dB on high side */

		i3 = x2 * bwscale;
		if (i3 < spectro->sizcbskirt) {
/*		  Remember which was first dft sample to be included */
		    if (spectro->nstart[spectro->nchan] == 0) {
			spectro->nstart[spectro->nchan] = i1;
		      }
/*		  Remember pointer to weight for this dft sample */
		     spectro->pweight[np] = i3; np++;

	    if (np >= NPMAX) {
		   printf("\nFATAL ERROR in MAKECBFB.C: 'NPMAX' exceeded\n");
		   printf("\t(while working on filter %d).  ", spectro->nchan);
		   printf("\tToo many filters or bw's too wide.\n");
		   printf("\tRedimension NPMAX and recompile if necessary.\n");
			exit(1);
		    }
/*		  Increment counter of number of dft samples in sum */
		    spectro->ntot[spectro->nchan]++;
		  }
	    }
	    spectro->nchan++;
	}
	spectro->sizcbskirt = SIZCBSKIRT;
/*    NCHAN is now set to be the total number of filter channels */


  }/* end makefbank */

/**************************************************************************
 *
 * void printfilters(XSPECTRO *spectro)
 *
 * Print filter characteristics, i.e. center freq. and bandwidth to text
 * window.
 *
 **************************************************************************/

void printfilters(XSPECTRO *spectro)
{
  
  int n3, n;
  char str[8000];
  char temp[200];
  Arg args[1];

  sprintf(str,"\t\t\t%s FILTER CHACTERISTICS:\n\n",spectro->wavename);
  strcat(str,"      n      FREQ      BW       n      FREQ      BW       n      FREQ      BW \n");
  
  n3 = (spectro->nchan+2)/3;
  for (n = 0; n < n3; n++) {
    sprintf(temp,"     %3d     %4d     %4d",
	    n+1, spectro->nfr[n], spectro->nbw[n]);
    strcat(str,temp);
    sprintf(temp,"     %3d     %4d     %4d", 
	    n+n3+1, spectro->nfr[n+n3], spectro->nbw[n+n3]);
    strcat(str,temp);
    if ((n+n3+n3) < spectro->nchan) {
      sprintf(temp,"     %3d     %4d     %4d\n",
	      n+n3+n3+1, spectro->nfr[n+n3+n3], spectro->nbw[n+n3+n3]);
      strcat(str,temp);
    }
    else{strcat(str,"\n");}
  }
  
  strcat(str,"\n");
  n = 0;
  XtSetArg(args[n],XmNvalue,str); n++;
  XtSetValues(spectro->fbw_text,args,n);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*		              M K W N D 				 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*    Make window */


void mkwnd(XSPECTRO *spectro)

{

	float xi, incr;
	int i,limit;

/* this shouldn't be needed */

	if ((spectro->sizwin > spectro->sizfft) || 
//            (spectro->sizwin <= 0) || (spectro->sizfft > 512)) {
	    (spectro->sizwin <= 0) || (spectro->sizfft > 4096)) {
	    printf("In mkwnd() of quickspec.c, illegal wsize=%d,dftsize=%d",
	    spectro->sizwin , spectro->sizfft);
	    exit(1);
	}

/*  printf(
       "\tMake Hamming window; width at base = %d points, pad with %d zeros\n",
	  wsize, power2-wsize); */
	xi = 0.;
	incr = TWOPI/(spectro->sizwin);
	limit = spectro->sizwin;
	/*if (spectro->halfwin == 1)    limit = limit >> 1;*/


/*    Compute non-raised cosine */
	for(i=0;i<limit;i++) {
	    if (spectro->params[NW] == 1) {
		spectro->hamm[i] = (1. -  cos(xi));
		xi += incr;
	      }
	    else {
		spectro->hamm[i] = 1.;	/* rectangular window */
	    }
	}

/*    Pad with zeros if window duration less than power2 points */
	while (i < spectro->sizfft) {
	    spectro->hamm[i++] = 0.;
	}
      }/* end mkwnd */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                        S P A T F I L T	                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Spatial filter fltr[] */

/* h[] is a combination of two processing steps:			    */
/*     1. Peak enhancement by lateral inhibition (numbers all div by 32):   */
/*	  (if 5 kHz, static256 pt. dft, max enhance 2 formants separ. by 320 Hz)  */
/* h[] = { -5, -10, -10,  -5,   6,  14,  20,  14,   6,  -5, -10, -10, -5 }; */
/*     2. Reduce effect of spectral slope (remove low freq comp):	    */
/* h[] = { -1,  -2,  -4,  -7,  -9, -10,  66, -10,  -9,  -7,  -4,  -2, -1 }; */

/*
spectro fltr

#define NSPATF	13
static int h[NSPATF] = {
	   -6, -12, -14, -12,  -3,   4,  86,   4,  -3, -12, -14, -12, -6 };
static int limit;

spatfilt() {

	int n,m,m1;

/c:    Do spatial filtering 
	for (n=0; n<noutchan; n++) {
	    spatfltr[n] = 0;
	    for (m=0; m<NSPATF; m++) {	c: An NSPATF-zero filter 
		m1 = n+m-(NSPATF>>1);		c:Pretend spectrum flat at ends 
		if (m1 < 0)    m1 = 0;
		if (m1 >= noutchan)    m1 = noutchan - 1;
		spatfltr[n] += (fltr[m1] * h[m]);
	    }
	 c: Scale answer: divide by 32, make positive 
	    spatfltr[n] = (spatfltr[n] >> 5) + 100;
	}
}end spatfilt*/





/*************************************************************************/
/*		             M G T O D B				 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Convert long integer to db */
int mgtodb(long nsum,int usergainoffset)  {
static  int dbtable[] = {
         0,  1,  2,  3,  5,  6,  7,  9, 10, 11,
        12, 13, 14, 16, 17, 18, 19, 20, 21, 22,
        23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
        33, 34, 35, 36, 37, 37, 38, 39, 40, 41,
        42, 43, 43, 44, 45, 46, 47, 47, 48, 49,
        50, 50, 51, 52, 53, 53, 54, 55, 56, 56,
        57, 58, 58, 59, 60};

	int db,temp,dbinc;

/*    Convert to db-times-10 */
/*    If sum is 03777777777L, db=660 */
/*    If sum is halved, db is reduced by 60 */
        db = 1440;
        if (nsum <= 0L) {
	  if (nsum < 0L) {
	    printf("Error in mgtodb of QUICKSPEC,");
	    printf("nsum=%ld is negative, set to 0\n", nsum);
	  }
	  return(0);
	}
	if (nsum > 03777777777L) {
	    printf ("Error in mgtodb of QUICKSPEC, nsum=%ld too big, truncate\n",
	     nsum);
	    nsum = 03777777777L;
        }
        while (nsum < 03777777777L) {
            nsum = nsum + nsum;
            db = db - 60;
        }
        temp = (nsum >> 23) - 077;
	if (temp > 64) {
	    temp = 64;
	    printf ("Error in mgtodb of QUICKSPEC, temp>64, truncate\n");
	}
        dbinc = dbtable[temp];
        db = db + dbinc;
	db = db >> 1;		    /* Routine expects mag, we have mag sq */
	db = db + usergainoffset - 400;/*User offset to spect gain*/
	if (db < 0) db = 0;
	return (db);
}/* end mgtodb */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                       T I L T		                         */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Replace set of points infltr[] by straight-line approx in outfltr[] */

int tilt(int *infltr, int npts, int *outfltr)
{
		
	int i,imin,imax,infltrmax;
	float xsum, ysum, xysum, x2sum;
	float slope;
	float yintc;

        short sloptruncsw;   /* should move this to spectro*/
       sloptruncsw = 0;

/* Adjust limits to include only peaks in range 1 kH to 5 kHz */
	imin = 0;
	imax = npts;
/*    Find first local max after 1000 Hz */
	infltrmax = infltr[0];
	for (i=0; i<imax; i++) {
		if (infltr[i] < infltr[i+1])    infltrmax = infltr[i+1]-60;
		else    break;
	}
/*    Ignore samples less than max-6 db */
/* OUT	for (i=0; i<imax; i++) {
		if (infltr[i] < infltrmax)    imin++;
		else    break;
	}
   END OUT */

/*    Find last local max before 5000 Hz, set infltrmax = peak value */
	infltrmax = infltr[imax-1];
	for (i=imax-1; i>imin; i--) {
		if (infltr[i] < infltr[i-1])    infltrmax = infltr[i-1];
		else    break;
	}
	infltrmax -= 60;
/*    Ignore samples less than max-6 db */
	if (sloptruncsw == 1) {
	    for (i=imax-1; i>imin; i--) {
		if (infltrmax > infltr[i])    imax--;
		else    break;
	    }
	}

	xsum = 0.0;
	ysum = 0.0;
	xysum = 0.0;
	x2sum = 0.0;

	for (i=imin; i<imax; i++){
		xsum = xsum + (i+1);
		ysum = ysum + infltr[i];
		xysum = xysum + (i+1) * infltr[i];
		x2sum = x2sum + (i+1) * (i+1);
	}

	slope = npts*xysum - xsum*ysum;
	slope = slope /( npts*x2sum -xsum*xsum);
	yintc = (ysum - slope*xsum) / npts;

	for (i=imin; i<imax; i++){
		outfltr[i] = slope*(i+1) + yintc;
	}

	i = 100 * slope;
	return(i);	/* Slope as integer value */
}/* end tilt */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*		            G E T F O R M . C		D. Klatt	 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */

/*		On VAX, located in directory [klatt.klspec]		*/


/* EDIT HISTORY:
 * 000001 31-Jan-84 DK	Initial creation
 * 000002 24-Mar-84 DK	Fix so hidden formant rejected if too close to peak
 * 000003 23-May-84 DK	Improved peak locating interpolation scheme
 * 000004 17-Apr-86 DK	Improve accuracy of freq estimate for hidden peaks
 */

/*               most of these are no an SPECTRO structure            */
/*	int dfltr[257];			Spectral slope */

/*        #define MAX_INDEX	  7*/
/*	int nforfreq;			number of formant freqs found   */
/*	int forfreq[16],foramp[16];	nchan and ampl of max in fltr[] */
/*	int valleyfreq[8],valleyamp[8];	nchan and ampl of min in fltr[] */

/*	int nmax_in_d;			number of maxima in deriv       */
/*	int ncdmax[8],ampdmax[8];	nchan and ampl of max in deriv  */
/*	int ncdmin[8],ampdmin[8];	nchan and ampl of min in deriv  */

/*	int hftot;			number of hidden formants found */
/*	int hiddenf[8],hiddena[8];	freq and ampl of hidden formant */


void getform(XSPECTRO *spectro) {

	int nch,lasfltr,lasderiv,direction,da,da1,da2,nhf;
	long nsum;
	int nchb,nchp,ff,df;
	int down = 1;
	int up = 2; 
        int index;


/*	  Pass 1: Compute spectral slope */
	    for (nch=2; nch<spectro->nchan-2; nch++) {
/*	      Approximate slope by function involving 5 consecutive channels */
		spectro->dfltr[nch] = 
                spectro->fltr[nch+2] + spectro->fltr[nch+1] -
                spectro->fltr[nch-1] - spectro->fltr[nch-2];
	    }
/*	  Pass 2: Find slope max, slope min; half-way between is a formant */
/*	   If max and min of same sign, this is a potential hidden formant */
	    direction = down;
	    spectro->nmax_in_d = 0;
	    lasderiv = spectro->dfltr[2];
	    spectro->hftot = 0;
	    for (nch=3; nch<spectro->nchan-2; nch++) {
	        if (spectro->dfltr[nch] > lasderiv) {
		    if ( (direction == down) && (spectro->nmax_in_d > 0) ) {
/*		      Found max and next min in deriv, see if hidden formant */
			if ((spectro->hiddenf[spectro->hftot] = testhidden(spectro)) > 0) {
                          index = (spectro->hiddenf[spectro->hftot]*spectro->sizfft)/spectro->spers;
		          spectro->hiddena[spectro->hftot] = spectro->fltr[index];
			    if (spectro->hftot < MAX_INDEX)   spectro->hftot++;
			}
		    }
		    direction = up;
	        }
	        else if (spectro->dfltr[nch] < lasderiv) {
		    if ((spectro->nmax_in_d > 0) && (direction == down)) {
/*		      On way down to valley */
			spectro->ampdmin[spectro->nmax_in_d-1] = spectro->dfltr[nch];
			spectro->ncdmin[spectro->nmax_in_d-1] = nch;
		    }
		    if ((direction == up) && (spectro->nmax_in_d < 7)) {
			spectro->ampdmin[spectro->nmax_in_d] = spectro->dfltr[nch];
			spectro->ncdmin[spectro->nmax_in_d] = nch;
/*		      Peak in deriv found, compute chan # and value of deriv */
			spectro->ampdmax[spectro->nmax_in_d] = lasderiv;
			spectro->ncdmax[spectro->nmax_in_d] = nch-1;
			if (spectro->nmax_in_d < MAX_INDEX)   spectro->nmax_in_d++;
		    }
		    direction = down;
	        }
	        lasderiv = spectro->dfltr[nch];
	    }
		
/*	  Pass 3: Find actual spectral peaks, fold hidden peaks into array */
	    direction = down;
	    spectro->nforfreq = 0;
	    nhf = 0;
	    lasfltr = spectro->fltr[0];
	    for (nch=1; nch<spectro->nchan; nch++) {
	        if (spectro->fltr[nch] > lasfltr) {
		    direction = up;
	        }
	        else if (spectro->fltr[nch] < lasfltr) {
		    if ((spectro->nforfreq > 0) && (direction == down)) {
/*		      On way down to valley */
			spectro->valleyamp[spectro->nforfreq-1] = spectro->fltr[nch];
		    }
		    if ((direction == up) && (spectro->nforfreq < 6)) {
			spectro->valleyamp[spectro->nforfreq] = spectro->fltr[nch];

/*		    Peak found, compute frequency */
/*		      Step 1: Work back to filter defining beginning of peak */
			for (nchb=nch-2; nchb>0; nchb--) {
			    if (spectro->fltr[nchb] < spectro->fltr[nch-1])    break;
			}
/*		      Step 2: Compute nearest filter of peak */
			nchp = (nchb + nch) >> 1;
			ff = spectro->nfr[nchp];
/*		      Step 3: Add half a filter if plateau has even # filters */
			if (nchp < ((nchb + nch + 1) >> 1)) {
			    ff = (spectro->nfr[nchp] + spectro->nfr[nchp+1]) >> 1;
			}
/*		      Step 4: Compute interpolation factor */
		        da1 = spectro->fltr[nchp] - spectro->fltr[nchb];
		        da2 = spectro->fltr[nchp] - spectro->fltr[nch];
		        da = da1 + da1;
			if (da2 > da1)    da = da2 + da2;
			nsum = ((da1-da2) * (spectro->nfr[nch-1] - spectro->nfr[nch-2]));
			df = nsum / da;
			ff += df;


			while ((nhf < spectro->hftot) && (spectro->hiddenf[nhf] < ff)
			&& (spectro->nforfreq < MAX_INDEX)) {
/*			  Hidden formant should be inserted here */
			    spectro->foramp[spectro->nforfreq] = spectro->hiddena[nhf];
			    spectro->forfreq[spectro->nforfreq] = - spectro->hiddenf[nhf];
                            spectro->nforfreq++; nhf++;
			}

			spectro->foramp[spectro->nforfreq] = lasfltr;
			spectro->forfreq[spectro->nforfreq] = ff;
			if (spectro->nforfreq < MAX_INDEX)    spectro->nforfreq++;
		    }
		    direction = down;
	        }
	        lasfltr = spectro->fltr[nch];
	    }
	    while ((nhf < spectro->hftot) && (spectro->nforfreq < MAX_INDEX)) {
/*	      Hidden formant should be inserted here */
		spectro->foramp[spectro->nforfreq] = spectro->hiddena[nhf];
		spectro->forfreq[spectro->nforfreq] = - spectro->hiddenf[nhf]; 
                spectro->nforfreq++;
                nhf++;                                          /* minus indic. hidd */
	    }
	    zap_insig_peak(spectro);
}



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*		        T E S T H I D D E N				 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Found loc max in deriv followed by loc min in deriv, see if hidden formant */

int testhidden(XSPECTRO *spectro) {

	int nchpeak;	/* channel number of potential hidden peak */
	int amphidpeak;	/* amplitude of hidden peak = (max+min)/2 in deriv */
	int delamp,delslope,nchvar,f,a,denom,foffset;

        int NCH_OF_PEAK_IN_DERIV, AMP_OF_PEAK_IN_DERIV;
        int NCH_OF_VALLEY_IN_DERIV , AMP_OF_VALLEY_IN_DERIV;

        NCH_OF_PEAK_IN_DERIV = spectro->ncdmax[spectro->nmax_in_d-1];
        AMP_OF_PEAK_IN_DERIV = spectro->ampdmax[spectro->nmax_in_d-1];
        NCH_OF_VALLEY_IN_DERIV = spectro->ncdmin[spectro->nmax_in_d-1];
        AMP_OF_VALLEY_IN_DERIV = spectro->ampdmin[spectro->nmax_in_d-1];




/*    A real peak is where derivative passes down through zero. */
/*    A hidden peak is where local max and succeeding local min of derivative */
/*    both are positive, or both are negative */
	if ((AMP_OF_PEAK_IN_DERIV <= 0) 
	 || (AMP_OF_VALLEY_IN_DERIV >= 0)) {

/*	   OUT printf("ap=%d av=%d fp=%d fv=%d\n",
	     AMP_OF_PEAK_IN_DERIV, AMP_OF_VALLEY_IN_DERIV, 
	     NCH_OF_PEAK_IN_DERIV, NCH_OF_VALLEY_IN_DERIV); END OUT */

/*	  See if diff in slope not simply noise */
	    delslope = AMP_OF_PEAK_IN_DERIV - AMP_OF_VALLEY_IN_DERIV;
	    if (delslope < 25) {
		return(-1);
	    }

/*	  Tentative channel hidden peak = mean of peak & valley locs in deriv */
	    nchpeak = (NCH_OF_PEAK_IN_DERIV + NCH_OF_VALLEY_IN_DERIV) >> 1;

/*	  Find amplitude of nearest formant peak (local spectral max) */
	    if (AMP_OF_PEAK_IN_DERIV < 0) {
/*	      Nearest peak is previous peak, find it */
		nchvar = nchpeak;
		while ( (spectro->fltr[nchvar-1] >= spectro->fltr[nchvar]) && (nchvar > 0) ) {
		    nchvar--;
		}
		delamp = spectro->fltr[nchvar] - spectro->fltr[nchpeak];
		foffset = -80;
	    }
	    else {
/*	      Nearest peak is next peak, find it */
		nchvar = nchpeak;
		while ( (spectro->fltr[nchvar+1] >= spectro->fltr[nchvar]) && (nchvar < 127) ) {
		    nchvar++;
		}
		delamp = spectro->fltr[nchvar] - spectro->fltr[nchpeak];
		foffset = 120;	/* hidden formant is actually between min
				   and next max */
	    }

/*	  See if amp of hidden formant not too weak relative to near peak */
	    if (delamp < 80) {
/*	      Exact location of hidden peak is halfway from max to min in deriv */
		amphidpeak = (AMP_OF_PEAK_IN_DERIV+AMP_OF_VALLEY_IN_DERIV)>>1;
		nchpeak = NCH_OF_PEAK_IN_DERIV;
		if (spectro->dfltr[nchpeak] > amphidpeak) {
		    while ( ((spectro->dfltr[nchpeak] - amphidpeak) >= 0)
		     && (nchpeak <= NCH_OF_VALLEY_IN_DERIV) ) {
			nchpeak++;
		    }
		    nchpeak--;
		}
		a = spectro->dfltr[nchpeak] - amphidpeak;
		denom = spectro->dfltr[nchpeak] - spectro->dfltr[nchpeak+1];
		if (denom > 0) {
		    f = spectro->nfr[nchpeak]
			 + ((a*(spectro->nfr[nchpeak+1]-spectro->nfr[nchpeak]))/denom);
		}
		else {
		    f = spectro->nfr[nchpeak];
		}

		return(f+foffset);
	    }
	}
	return(-1);
      }/* end testhidden */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*		        Z A P - I N S I G - P E A K			 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*	  Eliminate insignificant peaks (last one always significant) */
/*	  Must be down 6 dB from peaks on either side		      */
/*	  And valley on either side must be less than 2 dB	      */

/*	  Also eliminate weaker of two formant peaks within 210 Hz of */
/*	  each other, unless not enough formants in F1-F3 range	      */

void zap_insig_peak(XSPECTRO *spectro) {

	int nf,n,xxx,fcur,fnext;

	for (nf=0; nf<spectro->nforfreq-1; nf++) {

	    if ((nf == 0) || (spectro->foramp[nf-1] > spectro->foramp[nf]+60)) {
		if (spectro->foramp[nf+1] > spectro->foramp[nf]+60) {
		    if ((nf == 0) || (spectro->valleyamp[nf-1] > spectro->foramp[nf]-20)) {
			if (spectro->valleyamp[nf] > spectro->foramp[nf]-20) {
			    for (n=nf; n<spectro->nforfreq; n++) {
				spectro->forfreq[n] = spectro->forfreq[n+1];
				spectro->foramp[n] = spectro->foramp[n+1];
			    }
			    spectro->nforfreq--;
			}
		    }
		}
	    }

/*	  Zap weaker of 2 close peaks (hidden peak is alway weaker) */
	    if (nf < spectro->nforfreq-1) {
		fcur = spectro->forfreq[nf];
		if (fcur < 0)    fcur = -fcur;

		fnext = spectro->forfreq[nf+1];
		if (fnext < 0)    fnext = -fnext;

		if ((fnext - fcur) < 210) {

		    if (spectro->foramp[nf] > spectro->foramp[nf+1])    xxx = 1;
		    else    xxx = 0;

		    for (n=nf+xxx; n<spectro->nforfreq-1; n++) {
			spectro->forfreq[n] = spectro->forfreq[n+1];
			spectro->foramp[n] = spectro->foramp[n+1];
		    }
		    spectro->nforfreq--;
		    nf--;
		}
	    }

	}
      }/* end zap_insig_peak */
/*******************************************************************/
/*                      printwav(XSPECTRO *spectro,int window)      */
/*******************************************************************/
/* create a postscript file of waveform, window 1 or 2 */

void  printwav(XSPECTRO *spectro, int window, FILE *fp) 
{
  float scale, tmpscale, tmpscale1;
  time_t curtime;
  char datestr[50];

  /* 8 1/2 X 11 = 612 by 792  */
  /* leave a 1 in. border all around */
  /* need to make sure the BoundingBox */
  /* takes into account landscape orientation*/
  /* after 90 deg rotation, origin displaced 72 in y */
  
  scale = 1.0;
  /* do xr + yb fit on page (1 inch border, 792 x 612)*/
  if( spectro->xr[window] > 648  && spectro->yb[window] > 468){
    
    scale = (float)648 /(float)spectro->xr[window] ;
    tmpscale = (float)468 / (float)spectro->yb[window] ;
    if(tmpscale < scale)  scale = tmpscale;
    
  }/* check to see which scale prevails */
  else if(spectro->xr[window] > 648 ) {
    scale = (float)648 / (float)spectro->xr[window];
  }  
  else if(spectro->yb[window] > 468 ) {
    scale = (float)468 / (float)spectro->yb[window] ;
  }
  else {
    tmpscale = (float)648 / (float)spectro->xr[SPECTRUM];
    tmpscale1 = (float)468 / (float)spectro->yb[SPECTRUM];
    scale = ((tmpscale > tmpscale1)? tmpscale1 : tmpscale);
 }

  fprintf(fp,"%c%cBoundingBox: 72 72 %d %d\n",'%','%',
	  (int)( spectro->yb[window] * scale) + 72, 
	  (int)(spectro->xr[window] * scale) + 72); 
  fprintf(fp,"gsave\n");
  fprintf(fp,"/d {def} def \n");
  fprintf(fp,"/o {show} d /m {moveto} d\n");
  fprintf(fp,"/k {lineto stroke} d \n");
  
  /* put date and filename outside of bounding box */  

  fprintf(fp, "/Times-Roman findfont 13 scalefont setfont\n");
  fprintf(fp,"144 96 translate 90 rotate\n");

  time(&curtime);
  strcpy(datestr,ctime(&curtime) ); datestr[24] = ' ';
  fprintf(fp, "0 30 m (%s SF: %d Hz) o\n",datestr, (int)spectro->spers);
  fprintf(fp, "0 15 m (File: %s) o\n", spectro->wavefile);
  fprintf(fp, "0 0 m (Current Dir: %s) o\n", curdir);
  fprintf(fp,"grestore gsave\n");
  /* date and filename*/
  
  
  /* may not always scale char width properly but should work in most cases */
  fprintf(fp,
       "/Times-Roman findfont %d scalefont setfont\n",spectro->hchar);
  fprintf(fp,"%f %d translate 90 rotate\n", 
	  spectro->yb[window] * scale + 144,  72); 
  fprintf(fp,"%f %f scale\n",scale,scale);
  wave_pixmap(spectro->info[window],spectro,1, window, fp);
  
  fprintf(fp,"showpage grestore grestore\n");
}


/****************************************************************/
/*        printspectrum(spectro)                                */
/****************************************************************/
void printspectrum(XSPECTRO *spectro, FILE *fp)
{
  float scale, tmpscale, tmpscale1;
     
  scale = 1.0;
 
  /* do xr + yb fit on page (.5 inch border, 792 x 612)*/
  if ( spectro->xr[SPECTRUM] > 720  && spectro->yb[SPECTRUM] > 540){
    
    scale = (float)720 /(float)spectro->xr[SPECTRUM] ;
    tmpscale = (float)540 / (float)spectro->yb[SPECTRUM] ;
    if(tmpscale < scale) scale = tmpscale;
    
  } /* check to see which scale prevails */
  else if(spectro->xr[SPECTRUM] > 720 )
    scale = (float)720 / (float)spectro->xr[SPECTRUM];
  else  if(spectro->yb[SPECTRUM] > 540)
    scale = (float)540 / (float)spectro->yb[SPECTRUM] ;
  else {
    tmpscale = (float)720 / (float)spectro->xr[SPECTRUM];
    tmpscale1 = (float)540 / (float)spectro->yb[SPECTRUM];
    scale = ((tmpscale > tmpscale1)? tmpscale1 : tmpscale);
  }
  spectro->gscale = 1;

  fprintf(fp,"%c%cBoundingBox: 72 72 %d %d\n",'%','%',
	  (int)( spectro->yb[SPECTRUM] * scale) + 106, 
	  (int)(spectro->xr[SPECTRUM] * scale) + 36);

  fprintf(fp,"gsave\n");
  
  fprintf(fp,"/l {2 copy lineto stroke m} def\n");
  fprintf(fp,"/m {moveto} def /o {show} def\n");
  fprintf(fp,"/k {lineto stroke } def\n");
  
  /*fprintf(fp," .2 setlinewidth\n");*/
  
  /* may not always scale char width properly but should work in most cases */
  
  fprintf(fp,
	  "/Times-Roman findfont %d scalefont setfont\n", spectro->hchar);
  fprintf(fp,"%f %d translate 90 rotate\n", scale * spectro->yb[SPECTRUM] + 106,
	  36);
  fprintf(fp,"%f %f scale\n",scale, scale);
  draw_spectrum(spectro, 0, fp);
  fprintf(fp,"\nshowpage grestore grestore\n");

} /* end printspectrum */

/****************************************************************/
/*         fourspectra(XSPECTRO *spectro, FILE *fp)             */
/****************************************************************/

void fourspectra(XSPECTRO *spectro, FILE *fp)
{
  float scale;
  int ybsave, xrsave, startsave, usedsave;
  float timesave;
  char str[600];
  
  ybsave = spectro->yb[SPECTRUM];
  xrsave = spectro->xr[SPECTRUM];
  spectro->yb[SPECTRUM] = 300;
  spectro->xr[SPECTRUM] = 600;
  
  scale = .65; /* want to scale font */
  
  spectro->Gscale = 1;/* don't use scale in setfont */
  
  if (quadcount == 0) {
    fprintf(fp,"%c%cBoundingBox: 36 36 612 792\n",'%','%');
    fprintf(fp,"gsave\n");
    fprintf(fp, "/Times-Roman findfont %d scalefont setfont\n",
	    spectro->hchar);
    
    /* change quadrant here */
    /* 0|1 */
    /* 2|3 */
    fprintf(fp,"gsave\n");
    fprintf(fp,"%d %d translate 90 rotate\n", 315, 20);
    fprintf(fp,"%f %f scale\n",scale,scale);
    fprintf(fp,"%d %d translate \n",0,100);
  } /* set up and quad 0 */
  
  else if (quadcount == 1) {
    fprintf(fp,"grestore\n");
    fprintf(fp,"gsave\n");
    fprintf(fp,"%d %d translate 90 rotate\n", 315, 390);
    fprintf(fp,"%f %f scale\n",scale,scale);
    fprintf(fp,"%d %d translate \n",0,100);
  }
  else if (quadcount == 2) {
    fprintf(fp,"grestore\n");
    fprintf(fp,"gsave\n");
    fprintf(fp,"%d %d translate 90 rotate\n", 600, 20);
    fprintf(fp,"%f %f scale\n",scale,scale);
    fprintf(fp,"%d %d translate \n",0,100);
  }
  else if (quadcount == 3) {
    fprintf(fp,"grestore\n");
    fprintf(fp,"gsave\n");
    fprintf(fp,"%d %d translate 90 rotate\n",600,390);
    fprintf(fp,"%f %f scale\n",scale,scale);
    fprintf(fp,"%d %d translate \n",0,100);
  }
  
  if (quadcount != 4) {
    fprintf(fp,"/l {2 copy lineto stroke m} def\n");
    fprintf(fp,"/m {moveto} def /o {show} def\n");
    fprintf(fp,"/k {lineto stroke } def\n");
    
    draw_spectrum(spectro, 0, fp);
    spectro->yb[SPECTRUM] = ybsave;
    spectro->xr[SPECTRUM] = xrsave;
    
    ybsave = spectro->yb[1];
    xrsave = spectro->xr[1];
    spectro->yb[1] = 100;
    spectro->xr[1] = 575;
    fprintf(fp,"%d %d translate \n",0, -100);
    startsave = spectro->startindex[1];
    usedsave = spectro->sampsused[1];
    timesave = spectro->startime[1];

    spectro->startindex[1] = spectro->saveindex - 2*spectro->sizwin;
    spectro->sampsused[1] = spectro->sizwin * 4;

    if (spectro->startindex[1] < 0) spectro->startindex[1] = 0;
    if ((spectro->startindex[1] + spectro->sampsused[1]) > 
	(spectro->totsamp - 1))
      spectro->sampsused[1] = spectro->totsamp - 1 - 
	spectro->startindex[1];
    spectro->startime[1] = spectro->startindex[1] / 
      spectro->spers * 1000;

    wave_pixmap(spectro->info[1], spectro, 1, 1, fp);
    draw_hamm(spectro,1,spectro->info[1],fp);
    fprintf(fp,"\n");
    spectro->startindex[1] = startsave;
    spectro->sampsused[1] = usedsave;
    spectro->startime[1] = timesave;
    spectro->yb[1] = ybsave;
    spectro->xr[1] = xrsave; 
    quadcount++;
  }
  
  sprintf(str, "Small-size spectrum copied to quadrant %d of postscript file:\n\t%s\n", quadcount, spectro->tmp_name);
  writetext(str);
  
  if (quadcount == 4) {  /* get ready to start another page */
    quadcount = 0;
    fprintf(fp,"showpage grestore grestore\n");
  }
}

/****************************************************************
 *
 * void stepleft(XSPECTRO *spectro, int window)
 *
 * Move the cursor to the left by number of ms showing in window 1
 *
 ****************************************************************/

void stepleft(XSPECTRO *spectro, int window)
{
  int i;
  
  spectro->oldindex = spectro->saveindex;
  spectro->oldtime = spectro->savetime;
  if (spectro->sampsused[1] <= spectro->spers / 1000.0)
    spectro->savetime -= 1.0;
  else spectro->savetime -= (float)spectro->sampsused[1] / spectro->spers
	 * 1000.0;
  spectro->saveindex = spectro->savetime * spectro->spers / 1000.00 + .5;
  
  /* change savetime into time of index */
  spectro->savetime = (float)spectro->saveindex / spectro->spers * 1000.0;

  validindex(spectro);
  if (spectro->oldindex != spectro->saveindex) {
    new_spectrum(spectro);
    update(spectro->info[SPECTRUM],spectro, SPECTRUM); 
    for(i = 0; i < NUM_WINDOWS - 1; i++)
      update(spectro->info[i], spectro, i);
    writcurpos(spectro); /* time in text window */
  }
}

/****************************************************************
 *
 * void stepright(XSPECTRO *spectro, int window)
 *
 * Move the cursor to right by the number of ms showing in window 1
 *
 ****************************************************************/

void stepright(XSPECTRO *spectro, int window)
{
  int i;

  spectro->oldindex = spectro->saveindex;
  spectro->oldtime = spectro->savetime;
  if (spectro->sampsused[1] <= spectro->spers / 1000.0)
    spectro->savetime += 1.0;
  else
    spectro->savetime += (float)spectro->sampsused[1] / spectro->spers
      * 1000.0 ; 
  spectro->saveindex = spectro->savetime * spectro->spers / 1000.0  + .5;
  spectro->savetime = (float)spectro->saveindex / spectro->spers * 1000.0;

  validindex(spectro);
  if (spectro->oldindex != spectro->saveindex) {
    new_spectrum(spectro);
    update(spectro->info[SPECTRUM],spectro,SPECTRUM); 
    for(i = 0; i < NUM_WINDOWS - 1; i++)
      update(spectro->info[i], spectro, i);
    
    writcurpos(spectro); /* time in text window */
  }
}

/****************************************************************
 *
 * void zoomin(XSPECTRO *spectro, int window)
 *
 * Zoom in x (time) direction.
 *
 ****************************************************************/

void zoomin(XSPECTRO *spectro, int window)
{
  int oldstart, oldsampsused;

  if (window == SPECTRUM || window == SPECTROGRAM) return;

  oldstart = spectro->startindex[window];
  oldsampsused = spectro->sampsused[window] ;

  spectro->sampsused[window]  = spectro->sampsused[window] / 2;
  if (spectro->sampsused[window] / 2 < spectro->spers / 1000)
    spectro->sampsused[window] = spectro->spers / 1000.0  +.5;
  spectro->startindex[window] = spectro->saveindex -
    spectro->sampsused[window]/2;
  if (spectro->startindex[window] < 0) spectro->startindex[window] = 0;
  if ((spectro->startindex[window] + spectro->sampsused[window]) > 
      (spectro->totsamp - 1))
    spectro->sampsused[window] = spectro->totsamp - 1 - 
      spectro->startindex[window];

  if(oldstart != spectro->startindex[window] ||
     oldsampsused != spectro->sampsused[window] ){ /* zoom in */
    spectro->startime[window] = spectro->startindex[window] / 
      spectro->spers * 1000;
    wave_pixmap(spectro->info[window],spectro,1,window,NULL);
    update(spectro->info[window],spectro,window);
  } 
}

/****************************************************************
 *
 * void zoomout(XSPECTRO *spectro, int window)
 *
 * Zoom out x (time) direction.
 *
 ****************************************************************/

void zoomout(XSPECTRO *spectro, int window)
{
  int oldstart, oldsampsused;
   
  if (window == SPECTRUM || window == SPECTROGRAM) return;

  oldstart = spectro->startindex[window];
  oldsampsused = spectro->sampsused[window] ;

  spectro->startindex[window] = spectro->startindex[window] -
    spectro->sampsused[window] / 2 ;
  spectro->sampsused[window]  = spectro->sampsused[window] * 2;
  
  if (spectro->startindex[window] < 0)
    spectro->startindex[window] = 0;
  if ((spectro->startindex[window] + spectro->sampsused[window]) > 
      (spectro->totsamp - 1))
    spectro->sampsused[window] = spectro->totsamp - 1 - 
      spectro->startindex[window];
  
  if (oldstart != spectro->startindex[window] ||
     oldsampsused != spectro->sampsused[window]) {
    spectro->startime[window] = (float)spectro->startindex[window] / 
      spectro->spers * 1000.0;
    wave_pixmap(spectro->info[window],spectro,1,window,NULL);
    update(spectro->info[window],spectro,window);
  }
}

/*******************************************************************/
/*void  readspectrum(Widget w, XtPointer client_data, XtPointer call_data) */
/*******************************************************************/
void  readspectrum(Widget w, XtPointer client_data, XtPointer call_data){
/* add a new wave form and update allspectros*/
Arg args[3];
char *temp,mess[520],str[500];
XSPECTRO *spectro;
int i;
FILE *fp;
float val;
char fdtmp;

   XtUnmanageChild(filesb_dialog);

   spectro = (XSPECTRO *) client_data;

   XtSetArg(args[0],XmNvalue,&temp);
   XtGetValues(XmFileSelectionBoxGetChild(filesb_dialog,XmDIALOG_TEXT),args,1);

   strcpy(str,temp);
   XtFree(temp);

   XtRemoveAllCallbacks(filesb_dialog, XmNokCallback);
   XtRemoveAllCallbacks(filesb_dialog, XmNcancelCallback);

  /* need to check file spec!!!!!*/
  /* need to copy VMS stuff from wave_stuff */


  if(!(fp = fopen(str,"r")) ){
       sprintf(mess,"can't open: %s\n",str);
       ShowOops(mess);
       return ;   
     }/* can't find file */


   if(spectro->option != 's'){
       spectro->option = 's';
     }
    spectro->history = 0; /* don't copy into savfltr*/
    new_spectrum(spectro); 

    /* need to know sampling rate for scaling*/
    fscanf(fp,"%s\n",spectro->savname);
    fscanf(fp,"%f\n",&spectro->savspers);
    fscanf(fp,"%f\n",&spectro->oldtime); 
    fscanf(fp,"%d\n",&spectro->lensavfltr);
   
    fscanf(fp,"%d",&spectro->savsizwin);
    /* this may or maynot contain a first difference parameter */
    fscanf(fp,"%c",&fdtmp);
    if(fdtmp != '\n'){
      fscanf(fp,"%d\n",&spectro->savfd);
    }
    else{spectro->savfd = -1;}

    for( i = 0; i < spectro->lensavfltr; i ++)
           {fscanf(fp,"%f\n",&val);
            spectro->savfltr[i] = (int)(val * 10);
             }

    fclose(fp);
    sprintf(mess,"read in: %s\n", str);
    writetext(mess);


    spectro->history = 1;/* draw savfltr */
    spectro->savspectrum = 1;
   /* now draw savfltr based on sampling rate of file */
   draw_spectrum(spectro, 0, NULL);/* this draws onto the pixmap */    
   XCopyArea(spectro->info[SPECTRUM]->display,spectro->info[SPECTRUM]->pixmap,
           spectro->info[SPECTRUM]->win,spectro->info[SPECTRUM]->gc,
            0 ,0,spectro->xr[SPECTRUM],spectro->yb[SPECTRUM],0,0);
 
}/* end readspectrum */

/********************************************************************/
/*void  writespectrum(Widget w, XtPointer client_data, XtPointer call_data) */
/********************************************************************/
void  writespectrum(Widget w, XtPointer client_data, XtPointer call_data){
/* add a new wave form and update allspectros*/
Arg args[3];
char *temp,fname[520],mess[550],str[500];
XSPECTRO *spectro;
int i,n;
FILE *fp;
XmString mstr;
extern XmString save_dir;
XmFileSelectionBoxCallbackStruct *c_data = (XmFileSelectionBoxCallbackStruct *) call_data;

//   XtUnmanageChild(input_dialog);
   XtUnmanageChild(filesb_dialog);
   spectro = (XSPECTRO *) client_data;
   save_dir = XmStringCopy(c_data->dir);

   XtSetArg(args[0],XmNvalue,&temp);
//   XtGetValues(XmSelectionBoxGetChild(input_dialog,XmDIALOG_TEXT),args,1);
   XtGetValues(XmFileSelectionBoxGetChild(filesb_dialog,XmDIALOG_TEXT),args,1);

   strcpy(str,temp);
   XtFree(temp);

//   XtRemoveAllCallbacks(input_dialog, XmNokCallback);
//   XtRemoveAllCallbacks(input_dialog, XmNcancelCallback);
   XtRemoveAllCallbacks(filesb_dialog, XmNokCallback);
   XtRemoveAllCallbacks(filesb_dialog, XmNcancelCallback);

   if(spectro->option != 's'){
       spectro->option = 's';
       spectro->history = 0;
       new_spectrum(spectro);
       update(spectro->info[SPECTRUM],spectro,SPECTRUM);
     }
  /* is there a spectrum */
  if(!spectro->spectrum){
     writetext("Cursor out of range.\n");
     return;
   }
  
  sprintf(fname,"%s.spectrum",str);

 if(!(fp = fopen(fname,"r"))   ){
      /* can write with impunity */

    if(!(fp = fopen(fname,"w")) ){
       sprintf(mess,"can't open: %s\n",fname);
       ShowOops(mess);
       return ;  /* no file*/  
     }/* can't open file */

    else{
      /* opened for write */
     /*if changes are made here change dowritespectrum as well*/
    fprintf(fp,"%s\n",spectro->wavename);
    fprintf(fp,"%f\n",spectro->spers); /* samples per second */
    fprintf(fp,"%f\n",spectro->savetime);/*center of spectrum*/
    fprintf(fp,"%d\n",spectro->lenfltr);
    /* changed 6/96 to include first difference */
    fprintf(fp,"%d %d\n",spectro->sizwin,spectro->params[FD]);
    
  
    /* divide by 10 so values are consistant with draw_spectrum */
    for( i = 0; i < spectro->lenfltr; i ++)
           {fprintf(fp,"%.1f \n",(float)spectro->fltr[i]/10.0); }

    fclose(fp);
    sprintf(mess,"Wrote: %s.\n",fname);
    writetext(mess);
    }/*wrote file*/
  }/* don't have to worry about overwrite */

  else{
 /* was opened for read */
  fclose(fp);
    if(!(fp = fopen(fname,"w")) ){
       sprintf(mess,"can't open: %s\n",fname);
       ShowOops(mess);
       return ;   
     }/* can't open file */


   else{
    /* was opened for write */
   fclose(fp);   
   strcpy(spectro->tmp_name,fname);
   sprintf(mess,"OVERWRITE %s ??",fname);
   mstr = XmCvtCTToXmString(mess);
   n = 0;
   XtSetArg(args[n], XmNmessageString,mstr); n++;
   XtSetValues(warning,args,n);
   XmStringFree(mstr);
   mstr = XmCvtCTToXmString("OK");
   XtSetArg(args[0], XmNokLabelString,mstr);
   XtSetValues(warning,args,1);
   XmStringFree(mstr);
   XtRemoveAllCallbacks(input_dialog, XmNokCallback);
   XtRemoveAllCallbacks(input_dialog, XmNcancelCallback);  
   XtAddCallback(warning,XmNokCallback,dowritespectrum,(XtPointer)spectro);
   XtAddCallback(warning,XmNcancelCallback,cancelwarning,NULL);
   opendialog(warning);
   }/* popup warning window */
  }/*could read file check for write or overwrite */
}/* end writefltr*/

/*****/
void dowritespectrum(Widget w,XtPointer client_data, XtPointer call_data){
XSPECTRO *spectro;
int i;
FILE *fp;
char mess[520];

   XtUnmanageChild(warning);
   spectro = (XSPECTRO *) client_data;


   XtRemoveAllCallbacks(warning, XmNokCallback);
   XtRemoveAllCallbacks(warning, XmNcancelCallback);


/* file has already been checked so go ahead and open it */
   fp = fopen(spectro->tmp_name,"w");

    fprintf(fp,"%s\n",spectro->wavename);
    fprintf(fp,"%f\n",spectro->spers); /* samples per second */
    fprintf(fp,"%f\n",spectro->savetime);/*center of spectrum*/
    fprintf(fp,"%d\n",spectro->lenfltr);
    fprintf(fp,"%d\n",spectro->sizwin);
  
    /* divide by 10 so values are consistant with draw_spectrum */
    for( i = 0; i < spectro->lenfltr; i ++)
           {fprintf(fp,"%.1f \n",(float)spectro->fltr[i]/10.0); }

      fclose(fp);
      sprintf(mess,"Wrote: %s.\n",spectro->tmp_name);
      writetext(mess);

}/* end dowritespectrum */

/********************************************************************/
/*                    getavg(XSPECTRO *spectro)                     */
/********************************************************************/
void getavg(XSPECTRO *spectro)
{
  int nx, i, index;
  float cumscale,cumspec[256];
  char str[200], temp[20];
  INFO *info;
  int halfwin;
  int nxxmin, nxxmax;
  int msstep;
  
  halfwin = (spectro->params[WD] >> 2);
  
  msstep = 0.001* spectro->spers;
  
  if( spectro->option == 'k' || spectro->option == 'a') {
    if(spectro->option == 'k') {
      spectro->avgtimes[0] = (int)(spectro->savetime + .5) - spectro->params[KN];
      spectro->avgtimes[1] = (int)(spectro->savetime + .5) + spectro->params[KN];
    }
    /* set up nxxmin for 'a' and 'k'  */
    
    nxxmin = spectro->avgtimes[0] * spectro->spers / 1000.0 - halfwin;
    nxxmax = spectro->avgtimes[1] * spectro->spers / 1000.0 - halfwin;
    
    /* check to make sure the times are good first */
    for (i = spectro->avgtimes[0]; i <= spectro->avgtimes[1]; i++  ) {
      index = (i * spectro->spers / 1000.0 + .5) -
      halfwin;
      if(index < 0 || index >= spectro->totsamp){
	/* can't do dft */
	sprintf(str,"Invalid time: %d.\n",index);
	writetext(str);
	return;
      }
      
    }/* all times */
    
}/* 'a' and 'k' */
  
  else if( spectro->option == 'A'){
    for (i = 0; i <= spectro->avgcount; i++  ) {
      index = (spectro->avgtimes[i] * spectro->spers / 1000.0 + .5) -
	halfwin;
      if(index < 0 || index >= spectro->totsamp){
	/* can't do dft */
	sprintf(str,"Invalid time: %d\n",spectro->avgtimes[i]);
	writetext(str);
	return;
      }
    }/* all times */
    
  }/*A*/
  
  /* done checking now change stuff */
  
  spectro->sizwin = spectro->params[WD];  /* DFT window*/
  spectro->sizfft = spectro->params[SD]; /* user defined size dft */
  spectro->type_spec = DFT_MAG_NO_DB;    /*AVG_SPEC = DFT_MAG_NO_DB */
  spectro->nsdftmag = spectro->sizfft / 2;
  
  for (nx=0; nx < spectro->nsdftmag; nx++) cumspec[nx] = 0.;
  
  
  /* now do dfts */
  if( spectro->option == 'k' || spectro->option == 'a'){
    
    /* Step through waveform at 1 msec as suggested by KNS */
    /*		Change by ANANTHA	*/
    sprintf(str,"Averaging from %d to %d in 1 msec steps.\n",
	    spectro->avgtimes[0],spectro->avgtimes[1]);
    writetext(str);
    
    for (i = nxxmin; i <= nxxmax; i+= msstep  ) {
      /* this calls dft routine */              
      quickspec(spectro,&spectro->iwave[i]);
      /* Add floating mag sq spectrum into cum spect array */

      for (nx=0; nx < spectro->lenfltr; nx++) {
	cumspec[nx] += spectro->dftmag[nx];
      }
    }
    
  }/* 'a' or 'k' */
  
  else if( spectro->option == 'A'){
    nxxmin = 0;
    nxxmax = 0;
    strcpy(str,"avgerage times:");    
    for (i = 0; i <= spectro->avgcount; i++  ) {
      nxxmax += halfwin;
      sprintf(temp,"%d ",spectro->avgtimes[i]);
      strcat(str,temp);
      index = (spectro->avgtimes[i] * spectro->spers / 1000.0 + .5) -
	halfwin;
      /* this calls dft routine */              
      quickspec(spectro,&spectro->iwave[index]);
      /* Add floating mag sq spectrum into cum spect array */
      for (nx=0; nx < spectro->lenfltr; nx++) {
	cumspec[nx] += spectro->dftmag[nx];
      }
    }
    
    strcat(str,"\n");
    writetext(str);
  }/*'A'*/
  
  
  /* I don't feel comfortable with the way cumscale is handled 
     'A' gives different results that 'k' and 'a'   dh  */
  
  /* need to check time  to see if there are enough samples for dft*/
  /*DFT stored in savfltr */
  if (nxxmax - nxxmin < halfwin)  nxxmax = nxxmin + halfwin;
  cumscale = 20000.0 * (float)halfwin / (nxxmax - nxxmin);
  spectro->lensavfltr = spectro->lenfltr;
  spectro->savspectrum = 1;
  spectro->history = 1;
  spectro->savsizwin = spectro->sizwin;
  spectro->savspers = spectro->spers;
  for (nx=0; nx < spectro->lensavfltr; nx++) {
    spectro->savfltr[nx] = 
      mgtodb(cumspec[nx] * cumscale,spectro->params[SG]);
  }
  cumscale = (float)halfwin / 
    (float)(nxxmax - nxxmin);
    

    /*	      Smooth avg dft spectrum */
    spectro->spectrum = 1;
    for (nx=0; nx < spectro->lenfltr; nx++) 
      spectro->dftmag[nx] = cumspec[nx] * cumscale ;
    spectro->type_spec = SPECTO_NO_DFT;
    /* no dft so iwave index doesn't matter */
    quickspec(spectro,&spectro->iwave[512]);

if ( spectro->option == 'k') {
  
   spectro->oldindex = spectro->saveindex;
   spectro->oldtime = spectro->savetime;

   spectro->savetime = spectro->savetime + 
      (spectro->avgtimes[1] - spectro->avgtimes[0])/2;
   spectro->saveindex = spectro->savetime * spectro->spers / 1000.0 + .5;
   spectro->savetime = (float)spectro->saveindex / spectro->spers * 1000.0;
   validindex(spectro);
   if( spectro->oldindex != spectro->saveindex){
       for(i = 0; i < 3; i++){
           update(spectro->info[i], spectro, i);
         }
   }/* new index*/
}
 draw_spectrum(spectro, 0, NULL);
 info = spectro->info[SPECTRUM];
 
 XCopyArea(info->display,info->pixmap,
	   info->win,info->gc,
	   0,0,spectro->xr[SPECTRUM],spectro->yb[SPECTRUM],0,0);
 
}/* end getavg */



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*		            G E T F 0 . C		D.H. Klatt	 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*		On VAX, located in directory [klatt.klspec]		*/


/* EDIT HISTORY:
 * 000001 31-Jan-83 DK	Initial creation
 * 000002  6-Mar-84 DK	Fix serious bug in sorting of acceptable peaks
 * 000003  9-Mar-84 DK	Use comb filter to determine harmonic number of peaks
 * 000004 20-Jul-84 DK	Restrict range to 1500 Hz, select prominent harms, fix
 * 000005 24-Apr-86 DK	Add test to keep 'npeaks' within array size
 */
/*  in xklspec.h 
#define UP	2
#define DOWN	1
#define MAXNPEAKS	50
static int fpeaks[MAXNPEAKS];
*/

/*  Get estimate of fundamental frequency from dft mag spectrum */

int getf0(dftmag,npts,samrat) 
int dftmag[],npts,samrat; {

	int direction,npeaks,lasfltr,n;
	int dfpeaks[MAXNPEAKS],apeaks[MAXNPEAKS];
	int alastpeak,flastpeak,np,np1,mp,kp,dfp;
	int dfsum,dfmin,dfmax,dfn,nmax,nmaxa,ampmax;
	int da,da1,da2,df,npaccept;
	int dfsummax,dfnmax,dfnloc;

	direction = DOWN;
	npeaks = 0;
	lasfltr = dftmag[0];
/*    Look for local maxima only up to 1.5 kHz (was 3 kHz) */
	if (samrat > 0) {
	    nmax = (npts * 1500) / samrat;
/*	  Also find biggest amplitude in range from 0 to 900 Hz */
	    nmaxa = (npts * 900) / samrat;
	}
	else printf("In getf0(): somebody walked over samrat, set to zero");
	ampmax = 0;

/*    Begin search for set of local max freqs and ampls */
	for (n=1; n<nmax; n++) {
	    if (n < nmaxa) {
		if (dftmag[n] > ampmax)    ampmax = dftmag[n];
	    }
	    if (dftmag[n] > lasfltr) {
		direction = UP;
	    }
	    else if (dftmag[n] < lasfltr) {
		if ((direction == UP) && (npeaks < 49)) {
/*		  Interpolate the frequency of a peak by looking at
		  rel amp of previous and next filter outputs */
			da1 = dftmag[n-1] - dftmag[n-2];
			da2 = dftmag[n-1] - dftmag[n];
			if (da1 > da2) {
			    da = da1;
			}
			else {
			    da = da2;
			}
			if (da == 0) {
			    da = 1;
			}
			df = da1 - da2;
			df = (df * (samrat>>1)) / da;
			fpeaks[npeaks] = (((n-1) * samrat) + df) / npts;
			apeaks[npeaks] = lasfltr;
			if (npeaks < MAXNPEAKS-1)    npeaks++;
		}
		direction = DOWN;
	    }
	    lasfltr = dftmag[n];
	}

/*    Remove weaker of two peaks within minf0 Hz of each other */
#if 0
printf("\nInitial candidate set of %d peaks:\n", npeaks);
for (np=0; np<npeaks; np++) {
    printf("\t %d.   f = %4d   a = %3d\n", np+1, fpeaks[np], apeaks[np]);
}
#endif

	for (np=0; np<npeaks-1; np++) {

	    if (((np == 0) && (fpeaks[0] < 80))
	      || ((np > 0) && ((fpeaks[np] - fpeaks[np-1]) <= 50))) {
		if ((np > 0) && (apeaks[np] > apeaks[np-1])) np--;
		npeaks--;
		for (kp=np; kp<npeaks; kp++) {
		    fpeaks[kp] = fpeaks[kp+1];
		    apeaks[kp] = apeaks[kp+1];
		}
	    }
	}

/*    Remove initial weak peak before a strong one: */
	if ((apeaks[1] - apeaks[0]) > 200) {
	    npeaks--;
	    for (kp=0; kp<npeaks; kp++) {
		fpeaks[kp] = fpeaks[kp+1];
		apeaks[kp] = apeaks[kp+1];
	    }
	}

/*    Remove weak peak between two strong ones I: */
	for (np=1; np<npeaks-1; np++) {
/*	  See if a weak peak that should be ignorred */
	    if (((apeaks[np-1] - apeaks[np]) > 40)
	     && ((apeaks[np+1] - apeaks[np]) > 40)) {
		npeaks--;
		for (kp=np; kp<npeaks; kp++) {
		    fpeaks[kp] = fpeaks[kp+1];
		    apeaks[kp] = apeaks[kp+1];
		}
	    }
	}

/*    Remove weak peak between two strong ones II: */
	for (np=1; np<npeaks-1; np++) {
/*	  See if a weak peak that should be ignorred */
	    if ((apeaks[np-1] - apeaks[np]) > 80) {
		np1 = 1;
		while (((np+np1) < npeaks)
		  && (fpeaks[np+np1] < (fpeaks[np] + 400))) {
		    if ((apeaks[np+np1] - apeaks[np]) > 80) {
			npeaks--;
			for (kp=np; kp<npeaks; kp++) {
			    fpeaks[kp] = fpeaks[kp+1];
			    apeaks[kp] = apeaks[kp+1];
			}
			np--;
			goto brake1;
		    }
		    np1++;
		}
	    }
brake1:
	    np = np;
	}
#if 0
printf("\nFinal candidate set of %d peaks sent to comb filter:\n", npeaks);
for (np=0; np<npeaks; np++) {
    printf("\t %d.   f = %4d   a = %3d\n", np+1, fpeaks[np], apeaks[np]);
}
#endif

	if (npeaks > 1)        dfp = comb_filter(npeaks);
	else if (npeaks > 0)   dfp = fpeaks[0];
	else                   dfp = 0;
#if 0
printf("\nComb filter says f0 = %d\n", dfp);
printf("\nHit <CR> to continue:");
scanf("%c", &char1);
#endif

/*    Zero this estimate of f0 if little low-freq energy in spect */
	if (ampmax < 200) {
	    dfp = 0;
	}
/*    Or if f0 high and only a few peaks */
/* couldn't find any other reference to npaccept so*/
/* based on the above comment set it to npeaks d.h.*/
  npaccept = npeaks;
	if ( (npaccept <= 2) && (dfp > 300) )    dfp = 0;
	return(dfp);
      }



int comb_filter(npks) int npks; {

/* Look for an f0 with most harmonics present, restrict */
/* search to range from f0=60 to f0=400 */

#define NBINS	65
    float f0_hypothesis,fmin_freq,fmax_freq;
    static float f0_hyp[NBINS];
    int nharm,bin,min_freq,max_freq,cntr_freq,max_cntr_freq;
    int max_bin,max_count_harm,harm_found;
    int sum_f,sum_nharm,f0_estimate;
    int np,freq_harm,count_harm;

/* Initialization */
    if (f0_hyp[0] == 0) {
	f0_hyp[0] = 400.;
	for (bin=1; bin<NBINS; bin++) {
/*	  Compute center freq of each bin */
	    f0_hyp[bin] = .97 * f0_hyp[bin-1];
	}
    }

    if (npks > 6)    npks = 6;
    max_count_harm = 0;
    for (bin=0; bin<NBINS; bin++) {
	fmin_freq = 0.97 * f0_hyp[bin];
	cntr_freq = f0_hyp[bin];		/* for debugging only */
	fmax_freq = 1.03 * f0_hyp[bin];
	count_harm = 0;
	sum_nharm = 0;
	sum_f = 0;
	for (nharm=1; nharm<=6; nharm++) {
	    min_freq = (int) (fmin_freq * (float) nharm);
	    max_freq = (int) (fmax_freq * (float) nharm);
	    if (min_freq < 2500) {
		harm_found = 0;
		for (np=0; np<npks; np++) {
		    freq_harm = fpeaks[np];
		    if ((freq_harm > min_freq) && (freq_harm < max_freq)) {
			if (harm_found == 0) {
			    harm_found++;
			    count_harm++;
			    sum_nharm += nharm;
			    sum_f += freq_harm;
			}
		    }
		}
	    }
	}
/*	printf("%d ", count_harm); */
	if (count_harm > max_count_harm) {
	    max_count_harm = count_harm;
	    max_bin = bin;
	    max_cntr_freq = cntr_freq;
	    f0_estimate = sum_f / sum_nharm;
	}
    }
/*  printf("\n	Best comb freq = %d, f0 = %d\n",
     max_cntr_freq, f0_estimate); */

/*    Zero this estimate of f0 if the distribution of f0 estimates */
/*    is rather scattered */
	if ((3*max_count_harm) < (2*npks)) {
/*	    printf("\nZero f0=%3d", f0_estimate); */
	    f0_estimate = 0;
	}
/*	else {
	    printf("\n     f0=%3d", f0_estimate);
	}
	printf("  nh=%d np=", max_count_harm);
	for (np=0; np<npks; np++) {   spectro->option = spectro->savoption;
   spectro->history = 0;
	    printf("%4d, ", fpeaks[np]);
	} */
	return(f0_estimate);
  }


