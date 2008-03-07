/*******************************************************************************#
#	    guvcview              http://guvcview.berlios.de			#
#     Paulo Assis <pj.assis@gmail.com>						#
#										#
# This program is free software; you can redistribute it and/or modify         	#
# it under the terms of the GNU General Public License as published by   	#
# the Free Software Foundation; either version 2 of the License, or           	#
# (at your option) any later version.                                          	#
#                                                                              	#
# This program is distributed in the hope that it will be useful,              	#
# but WITHOUT ANY WARRANTY; without even the implied warranty of             	#
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  		#
# GNU General Public License for more details.                                 	#
#                                                                              	#
# You should have received a copy of the GNU General Public License           	#
# along with this program; if not, write to the Free Software                  	#
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA	#
#                                                                              	#
********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <pthread.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <SDL/SDL_audio.h>
#include <SDL/SDL_timer.h>
#include <linux/videodev.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <SDL/SDL_syswm.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <portaudio.h>

#include "v4l2uvc.h"
#include "avilib.h"

#include "prototype.h"

/*----------------------------- globals --------------------------------------*/
struct GLOBAL *global=NULL;
struct JPEG_ENCODER_STRUCTURE *jpeg_struct=NULL;

struct vdIn *videoIn=NULL;
VidState * s;

//const char *videodevice = NULL;
/* The main window*/
GtkWidget *mainwin;
/* A restart Dialog */
GtkWidget *restartdialog;

/* Must set this as global so they */
/* can be set from any callback.   */
/* When AVI is in capture mode we  */
/* can't change settings           */
GtkWidget *AVIComp;
GtkWidget *SndEnable; 
GtkWidget *SndSampleRate;
GtkWidget *SndDevice;
GtkWidget *SndNumChan;
GtkWidget *FiltMirrorEnable;
GtkWidget *FiltUpturnEnable;
GtkWidget *FiltNegateEnable;
GtkWidget *FiltMonoEnable;
/*must be called from main loop if capture timer enabled*/
GtkWidget *ImageFNameEntry;
GtkWidget *ImageType;
GtkWidget *CapAVIButt;
GtkWidget *AVIFNameEntry;
GtkWidget *FileDialog;

/*thread definitions*/
pthread_t mythread;
pthread_attr_t attr;

pthread_t sndthread;
pthread_attr_t sndattr;

/* parameters passed when restarting*/
const char *EXEC_CALL;

/*the main SDL surface*/
SDL_Surface *pscreen = NULL;

SDL_Surface *ImageSurf = NULL;
SDL_Overlay *overlay=NULL;
SDL_Rect drect;
SDL_Event sdlevent;

avi_t *AviOut;
BYTE *p = NULL;
BYTE * pim= NULL;
BYTE * pavi=NULL;

//char *avifile=NULL; /*avi filename passed through argument options with -n */
//int Capture_time=0; /*avi capture time passed through argument options with -t */

static Uint32 SDL_VIDEO_Flags =
	SDL_ANYFORMAT | SDL_DOUBLEBUF | SDL_RESIZABLE;

/*------------------------------ get time ------------------------------------*/
#if 0
double
ms_time (void)
{
  static struct timeval tod;
  gettimeofday (&tod, NULL);
  return ((double) tod.tv_sec * 1000.0 + (double) tod.tv_usec / 1000.0);

}
#elif 1 /*use this one*/
Uint32
ms_time (void)
{
  return ((Uint32) SDL_GetTicks()); /* gets time (ms) since SDL init */
}
#endif
/*----------------------- write conf (.guvcviewrc) file ----------------------*/
static
int writeConf(const char *confpath) {
	int ret=0;
	FILE *fp;
	if ((fp = fopen(confpath,"w"))!=NULL) {
		fprintf(fp,"# guvcview configuration file\n\n");
		fprintf(fp,"# video resolution - hardware supported (logitech) 320x240 640x480\n");
		fprintf(fp,"resolution=%ix%i\n",global->width,global->height);
		fprintf(fp,"# control window size: default %ix%i\n",WINSIZEX,WINSIZEY);
		fprintf(fp,"windowsize=%ix%i\n",global->winwidth,global->winheight);
		fprintf(fp,"# mode video format 'yuv' or 'jpg'(default)\n");
		fprintf(fp,"mode=%s\n",global->mode);
		fprintf(fp,"# frames per sec. - hardware supported - default( %i )\n",DEFAULT_FPS);
		fprintf(fp,"fps=%d/%d\n",global->fps_num,global->fps);
		fprintf(fp,"# bytes per pixel: default (0 - current)\n");
		fprintf(fp,"bpp=%i\n",global->bpp);
		fprintf(fp,"# hardware accelaration: 0 1 (default - 1)\n");
		fprintf(fp,"hwaccel=%i\n",global->hwaccel);
		fprintf(fp,"# video grab method: 0 -read 1 -mmap (default - 1)\n");
		fprintf(fp,"grabmethod=%i\n",global->grabmethod);
		fprintf(fp,"# video compression format: 0-MJPG 1-YUY2 2-DIB (BMP 24)\n");
		fprintf(fp,"avi_format=%i\n",global->AVIFormat);
		//fprintf(fp,"frequency=%i\n",freq);
		fprintf(fp,"# sound 0 - disable 1 - enable\n");
		fprintf(fp,"sound=%i\n",global->Sound_enable);
		fprintf(fp,"# snd_device - sound device id as listed by portaudio\n");
		fprintf(fp,"snd_device=%i\n",global->Sound_UseDev);
		fprintf(fp,"# snd_samprate - sound sample rate\n");
		fprintf(fp,"snd_samprate=%i\n",global->Sound_SampRateInd);
		fprintf(fp,"# snd_numchan - sound number of channels 0- dev def 1 - mono 2 -stereo\n");
		fprintf(fp,"snd_numchan=%i\n",global->Sound_NumChanInd);
		fprintf(fp,"# video filters: 0 -none 1- flip 2- upturn 4- negate 8- mono (add the ones you want)\n");
		fprintf(fp,"frame_flags=%i\n",global->Frame_Flags);
		printf("write %s OK\n",confpath);
		fclose(fp);
	} else {
	printf("Could not write file %s \n Please check file permissions\n",confpath);
	ret=1;
	}
	return ret;
}
/*----------------------- read conf (.guvcviewrc) file -----------------------*/
static
int readConf(const char *confpath) {
	int ret=1;
	char variable[20];
	char value[20];

	int i=0;
	int j=0;

	FILE *fp;

	if((fp = fopen(confpath,"r"))!=NULL) {
		char line[80];

	while (fgets(line, 80, fp) != NULL) {
		j++;
		if ((line[0]=='#') || (line[0]==' ') || (line[0]=='\n')) {
			/*skip*/
		} else if ((i=sscanf(line,"%[^#=]=%[^#\n ]",variable,value))==2){
			/* set variables */
			if (strcmp(variable,"resolution")==0) {
				if ((i=sscanf(value,"%ix%i",&(global->width),&(global->height)))==2)
				printf("resolution: %i x %i\n",global->width,global->height); 			
			} else if (strcmp(variable,"windowsize")==0) {
			if ((i=sscanf(value,"%ix%i",&(global->winwidth),&(global->winheight)))==2)
				printf("windowsize: %i x %i\n",global->winwidth,global->winheight);
			} else if (strcmp(variable,"mode")==0) {
			global->mode[0]=value[0];
			global->mode[1]=value[1];
			global->mode[2]=value[2];
			printf("mode: %s\n",global->mode);
			} else if (strcmp(variable,"fps")==0) {
			if ((i=sscanf(value,"%i/%i",&(global->fps_num),&(global->fps)))==1)
				printf("fps: %i/%i\n",global->fps_num,global->fps);
			} else if (strcmp(variable,"bpp")==0) {
			if ((i=sscanf(value,"%i",&(global->bpp)))==1)
				printf("bpp: %i\n",global->bpp);
			} else if (strcmp(variable,"hwaccel")==0) {
			if ((i=sscanf(value,"%i",&(global->hwaccel)))==1)
				printf("hwaccel: %i\n",global->hwaccel);
			} else if (strcmp(variable,"grabmethod")==0) {
			if ((i=sscanf(value,"%i",&(global->grabmethod)))==1)
				printf("grabmethod: %i\n",global->grabmethod);
			} else if (strcmp(variable,"avi_format")==0) {
			if ((i=sscanf(value,"%i",&(global->AVIFormat)))==1)
				printf("avi_format: %i\n",global->AVIFormat);
			} else if (strcmp(variable,"sound")==0) {
			if ((i=sscanf(value,"%i",&(global->Sound_enable)))==1)
				printf("sound: %i\n",global->Sound_enable);
			} else if (strcmp(variable,"snd_device")==0) {
			if ((i=sscanf(value,"%i",&(global->Sound_UseDev)))==1)
				printf("sound Device: %i\n",global->Sound_UseDev);
			} else if (strcmp(variable,"snd_samprate")==0) {
			if ((i=sscanf(value,"%i",&(global->Sound_SampRateInd)))==1)
						printf("sound samp rate: %i\n",global->Sound_SampRateInd);
			} else if (strcmp(variable,"snd_numchan")==0) {
			if ((i=sscanf(value,"%i",&(global->Sound_NumChanInd)))==1)
				printf("sound Channels: %i\n",global->Sound_NumChanInd);
			} else if (strcmp(variable,"frame_flags")==0) {
			if ((i=sscanf(value,"%i",&(global->Frame_Flags)))==1)
				printf("sound Channels: %i\n",global->Frame_Flags);
			}
		}    
		}
		fclose(fp);
	} else {
		printf("Could not open %s for read,\n will try to create it\n",confpath);
		ret=writeConf(confpath);
	}
	return ret;
}

/*------------------------- read command line options ------------------------*/
int readOpts(int argc,char *argv[]) {
	
	int i=0;
	char *separateur;
	char *sizestring = NULL;
	
	for (i = 1; i < argc; i++) {
	
		/* skip bad arguments */
		if (argv[i] == NULL || *argv[i] == 0 || *argv[i] != '-') {
			continue;
		}
		if (strcmp(argv[i], "-d") == 0) {
			if (i + 1 >= argc || *argv[i+1] =='-') {
				printf("No parameter specified with -d, using default.\n");
				//return (1);
			} else {
				snprintf(global->videodevice,15,"%s",argv[i + 1]);
			}
		}
		if (strcmp(argv[i], "-g") == 0) {
			/* Ask for read instead default  mmap */
			global->grabmethod = 0;
		}
		if (strcmp(argv[i], "-w") == 0) {
			/* disable hw acceleration */
			global->hwaccel = 0;
		}
		if (strcmp(argv[i], "-f") == 0) {
			if ( i + 1 >= argc || *argv[i+1] =='-') {
				printf("No parameter specified with -f, using default.\n");	
			} else {
				global->mode[0] = argv[i + 1][0];
				global->mode[1] = argv[i + 1][1];
				global->mode[2] = argv[i + 1][2];
			}
		}
		if (strcmp(argv[i], "-s") == 0) {
			if (i + 1 >= argc || *argv[i+1] =='-') {
			printf("No parameter specified with -s, using default.\n");
			//return(2);
			} else {

				sizestring = strdup(argv[i + 1]);

				global->width = strtoul(sizestring, &separateur, 10);
				if (*separateur != 'x') {
					printf("Error in size use -s widthxheight \n");
					//return(3);
				} else {
					++separateur;
					global->height = strtoul(separateur, &separateur, 10);
					if (*separateur != 0)
						printf("hmm.. dont like that!! trying this height \n");
				}
			}
			printf(" size width: %d height: %d \n",global->width, global->height);
		}
		if (strcmp(argv[i], "-n") == 0) {
			if (i + 1 >= argc || *argv[i+1] =='-') {
				printf("No parameter specified with -n. Ignoring option.\n");	
			} else {
				global->avifile = strdup(argv[i + 1]);
				splitPath(global->avifile,global->aviPath,global->aviName);
			}
		}
		if (strcmp(argv[i], "-t") == 0) {
			if (i + 1 >= argc || *argv[i+1] =='-') {
				printf("No parameter specified with -t.Ignoring option.\n");	
			} else {
				char *timestr = strdup(argv[i + 1]);
				global->Capture_time= strtoul(timestr, &separateur, 10);
				//sscanf(timestr,"%i",global->Capture_time);
				printf("capturing avi for %i seconds",global->Capture_time);
			}
		}
		if (strcmp(argv[i], "-h") == 0) {
			printf("usage: guvcview [-h -d -g -f -s -c -C -S] \n");
			printf("-h	print this message \n");
			printf("-d	/dev/videoX       use videoX device\n");
			printf("-g	use read method for grab instead mmap \n");
			printf("-w	disable SDL hardware accel. \n");
			printf
			("-f	video format  default jpg  others options are yuv jpg \n");
			printf("-s	widthxheight      use specified input size \n");
			printf("-n	avi_file_name   if avi_file_name set enable avi capture from start \n");
			printf("-t  capture_time  used with -n option, sets the capture time in seconds\n");
			exit(0);
		}
	}
	
	/*if -n not set reset capture time*/
	if(global->Capture_time>0 && global->avifile==NULL) global->Capture_time=0;
	
	if (strncmp(global->mode, "yuv", 3) == 0) {
		global->format = V4L2_PIX_FMT_YUYV;
		global->formind = 1;
		printf("Format is yuyv\n");
	} else if (strncmp(global->mode, "jpg", 3) == 0) {
		global->format = V4L2_PIX_FMT_MJPEG;
		global->formind = 0;
		printf("Format is MJPEG\n");
	} else {
		global->format = V4L2_PIX_FMT_MJPEG;
		global->formind = 0;
		printf("Format is Default MJPEG\n");
	}
}
/*--------------------------- sound threaded loop-----------------------------*/
void *sound_capture(void *data)
{
	
	PaStreamParameters inputParameters, outputParameters;
	PaStream *stream;
	PaError err;
	//paCapData  snd_data;
	SAMPLE *recordedSamples=NULL;
	int i;
	int totalFrames;
	int numSamples;
	int numBytes;
	
	FILE  *fid;
	fid = fopen(global->sndfile, "wb+");
	if( fid == NULL )
	{
	   printf("Could not open file.");
	}
	
	err = Pa_Initialize();
	if( err != paNoError ) goto error;
	/* Record for a few seconds. */
	
	if(global->Sound_SampRateInd==0)
		global->Sound_SampRate=global->Sound_IndexDev[global->Sound_UseDev].samprate;/*using default*/
	
	if(global->Sound_NumChanInd==0) {
		/*using default if channels <3 or stereo(2) otherwise*/
		global->Sound_NumChan=(global->Sound_IndexDev[global->Sound_UseDev].chan<3)?global->Sound_IndexDev[global->Sound_UseDev].chan:2;
	}
	//printf("dev:%d SampleRate:%d Chanels:%d\n",Sound_IndexDev[Sound_UseDev].id,Sound_SampRate,Sound_NumChan);
	
	/* setting maximum buffer size*/
	totalFrames = NUM_SECONDS * global->Sound_SampRate;
	numSamples = totalFrames * global->Sound_NumChan;
	numBytes = numSamples * sizeof(SAMPLE);
	recordedSamples = (SAMPLE *) malloc( numBytes );
	
	if( recordedSamples == NULL )
	{
		printf("Could not allocate record array.\n");
		return(1);
	}
	for( i=0; i<numSamples; i++ ) recordedSamples[i] = 0;

	inputParameters.device = global->Sound_IndexDev[global->Sound_UseDev].id; /* input device */
	inputParameters.channelCount = global->Sound_NumChan;
	inputParameters.sampleFormat = PA_SAMPLE_TYPE;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL; 
	
	/*---------------------------- Record some audio. ----------------------------- */
	/* Input buffer will be twice the size of frames to read                            */
	/* This way even in slow machines it shouldn't overflow and drop frames */
	err = Pa_OpenStream(
			  &stream,
			  &inputParameters,
			  NULL,                  /* &outputParameters, */
			  global->Sound_SampRate,
			  (totalFrames*2),/* buffer as double capacity of total frames to read*/
			  paNoFlag,      /* PaNoFlag - clip and dhiter*/
			  NULL, /* sound callback - using blocking API*/
			  NULL ); /* callback userData -no callback no data */
	if( err != paNoError ) goto error;
  
	err = Pa_StartStream( stream );
	if( err != paNoError ) goto error; /*should close the stream if error ?*/
	/*----------------------------- capture loop ----------------------------------*/
	//snd_begintime=SDL_GetTicks();
	global->snd_begintime = ms_time();
	do {
	   //Pa_Sleep(SND_WAIT_TIME);
	   err = Pa_ReadStream( stream, recordedSamples, totalFrames );
	   //if( err != paNoError ) break; /*can break with input overflow*/
	   /* Write recorded data to a file. */  
	   fwrite( recordedSamples, global->Sound_NumChan * sizeof(SAMPLE), totalFrames, fid );
	} while (videoIn->capAVI);   
	
	fclose( fid );
	
	err = Pa_StopStream( stream );
	if( err != paNoError ) goto error;
	
	err = Pa_CloseStream( stream ); /*closes the stream*/
	if( err != paNoError ) goto error; 
	
	if(recordedSamples) free( recordedSamples  );
	recordedSamples=NULL;
	Pa_Terminate();
	return(0);

error:
	fclose(fid);
	if(recordedSamples) free( recordedSamples );
	recordedSamples=NULL;
	Pa_Terminate();
	fprintf( stderr, "An error occured while using the portaudio stream\n" );
	fprintf( stderr, "Error number: %d\n", err );
	fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
	return(-1);
	
}


/*------------------------------ Event handlers -------------------------------*/
/* when window is closed */
gint
delete_event (GtkWidget *widget, GdkEventConfigure *event)
{
	shutd(0);//shutDown
	
	return 0;
}

/*-------------------------------- avi close functions -----------------------*/
/* Adds audio temp file to AVI         */
/*                                     */
int AVIAudioAdd(void *data) {
	
	SAMPLE *recordedSamples=NULL;
	int i;  
	int totalFrames;
	int numSamples;
	long numBytes;
	
	totalFrames = NUM_SECONDS * global->Sound_SampRate;
	numSamples = totalFrames * global->Sound_NumChan;
 
	numBytes = numSamples * sizeof(SAMPLE);

	recordedSamples = (SAMPLE *) malloc( numBytes );

	if( recordedSamples == NULL )
	{
		printf("Could not allocate record array.\n");
		/*must enable avi capture button*/
		gtk_widget_set_sensitive (CapAVIButt, TRUE);
		return (1);
	}
	for ( i=0; i<numSamples; i++ ) recordedSamples[i] = 0;/*init to zero - silence*/
	SDL_Delay(100); /*wait to make sure main loop as stoped writing to avi*/
	AVI_set_audio(AviOut, global->Sound_NumChan, global->Sound_SampRate, sizeof(SAMPLE)*8,WAVE_FORMAT_PCM);
	printf("sample size: %i bits\n",sizeof(SAMPLE)*8);
	
	/* Audio Capture allways starts last (delay due to thread initialization)*/
	printf("sound start:%#0.8X video start:%#0.8X\n",global->snd_begintime,global->AVIstarttime);
	int synctime= global->snd_begintime - global->AVIstarttime; /*time diff for audio-video*/
	if(synctime>0 && synctime<5000) { /*only sync up to 5 seconds*/
		/*shift sound by synctime*/
		Uint32 shiftFrames=abs(synctime*global->Sound_SampRate/1000);
		Uint32 shiftSamples=shiftFrames*global->Sound_NumChan;
		printf("shift sound forward by %d ms = %d frames\n",synctime,shiftSamples);
		SAMPLE EmptySamp[shiftSamples];
		for(i=0; i<shiftSamples; i++) EmptySamp[i]=0;/*init to zero - silence*/
		AVI_write_audio(AviOut,&EmptySamp,shiftSamples*sizeof(SAMPLE)); 
	} else if (synctime<0){
		/*shift sound by synctime*/
		Uint32 shiftFrames=abs(synctime*global->Sound_SampRate/1000);
		Uint32 shiftSamples=shiftFrames*global->Sound_NumChan;
		printf("shift sound backward by %d ms - %d frames\n",synctime,shiftSamples);
		/*eat up the number of shiftframes - never seems to happen*/
	}
	FILE *fip;
	fip=fopen(global->sndfile,"rb");
	if( fip == NULL )
	{
		printf("Could not open snd data file.\n");
	} else {
		while(fread( recordedSamples, global->Sound_NumChan * sizeof(SAMPLE), totalFrames, fip )!=0){  
			AVI_write_audio(AviOut,(BYTE *) recordedSamples,numBytes);
		}
	}
	fclose(fip);
	/*remove audio file*/
	unlink(global->sndfile);
	if (recordedSamples) free( recordedSamples );
	recordedSamples=NULL;
	
	AVI_close (AviOut);
	printf ("close avi\n");
	AviOut = NULL;
	global->framecount = 0;
	global->AVIstarttime = 0;
	/*must enable avi capture button*/
	gtk_widget_set_sensitive (CapAVIButt, TRUE);
	return (0);
	
}

/* Called at avi capture stop       */
/* from avi capture button callback */
void
aviClose (void)
{
  double tottime = 0;
  int tstatus;
	
  if (AviOut)
	{
	  tottime = global->AVIstoptime - global->AVIstarttime;
	  //printf("AVI: %i frames in %d ms\n",framecount,tottime);
	  
	  if (tottime > 0) {
		/*try to find the real frame rate*/
		AviOut->fps = (double) global->framecount *1000 / tottime;
	  }
	  else {
		/*set the hardware frame rate*/   
		AviOut->fps=videoIn->fps;
	  }
	  /*---------------- write audio to avi if Sound Enable ------------------*/
	  if (global->Sound_enable > 0) {
		/* Free attribute and wait for the thread */
		pthread_attr_destroy(&sndattr);
	
		int sndrc = pthread_join(sndthread, (void **)&tstatus);
	   
		if (tstatus!=0)
		{
			printf("ERROR: status from sound thread join is %d\n", tstatus);
			/*remove audio file*/
			unlink(global->sndfile);
			/* don't add sound*/
			AVI_close (AviOut);
			printf ("close avi\n");
			AviOut = NULL;
			global->framecount = 0;
			global->AVIstarttime=0;
		} else {
			printf("Capture sound thread join with status= %d\n", tstatus);
			/*run it in a new thread to make it non-blocking*/
			/*must disable avi capture button*/
			gtk_widget_set_sensitive (CapAVIButt, FALSE);

		if (AVIAudioAdd(NULL)>0) printf("ERROR: reading Audio file\n");

		}
	  } else { /*------------------- Sound Disable ---------------------------*/
		
		AVI_close (AviOut);
		printf ("close avi\n");
		AviOut = NULL;
		global->framecount = 0;
		global->AVIstarttime=0;
	  }
	}
}

/* can't remember what it does*/
static int
num_chars (int n)
{
	int i = 0;

	if (n <= 0) {
		i++;
		n = -n;
	}

	while (n != 0) {
		n /= 10;
		i++;
	}
	return i;
}



/*----------------------------- Callbacks ------------------------------------*/
/*slider labeler*/
static void
set_slider_label (GtkRange * range)
{
	ControlInfo * ci = g_object_get_data (G_OBJECT (range), "control_info");
	if (ci->labelval) {
		char str[12];
		sprintf (str, "%*d", ci->maxchars, (int) gtk_range_get_value (range));
		gtk_label_set_text (GTK_LABEL (ci->labelval), str);
	}
}

/*slider controls callback*/
static void
slider_changed (GtkRange * range, VidState * s)
{
  
	ControlInfo * ci = g_object_get_data (G_OBJECT (range), "control_info");
	InputControl * c = s->control + ci->idx;
	int val = (int) gtk_range_get_value (range);
	
	if (input_set_control (videoIn, c, val) == 0) {
		set_slider_label (range);
		printf ("changed to %d\n", val);
	}
	else {
		printf ("changed to %d, but set failed\n", val);
	}
	if (input_get_control (videoIn, c, &val) == 0) {
		printf ("hardware value is %d\n", val);
	}
	else {
		printf ("hardware get failed\n");
	}
	
}

/*check box controls callback*/
static void
check_changed (GtkToggleButton * toggle, VidState * s)
{
	
	ControlInfo * ci = g_object_get_data (G_OBJECT (toggle), "control_info");
	InputControl * c = s->control + ci->idx;
	int val;
	
	//~ if (c->id == V4L2_CID_EXPOSURE_AUTO) {
	//~ val = gtk_toggle_button_get_active (toggle) ? AUTO_EXP : MAN_EXP;
	//~ } 
	//~ else val = gtk_toggle_button_get_active (toggle) ? 1 : 0;
	val = gtk_toggle_button_get_active (toggle) ? 1 : 0;
	
	if (input_set_control (videoIn, c, val) == 0) {
		printf ("changed to %d\n", val);
	}
	else {
		printf ("changed to %d, but set failed\n", val);
	}
	if (input_get_control (videoIn, c, &val) == 0) {
		printf ("hardware value is %d\n", val);
	}
	else {
		printf ("hardware get failed\n");
	}
	
}

/*combobox controls callback*/
static void
combo_changed (GtkComboBox * combo, VidState * s)
{
	
	ControlInfo * ci = g_object_get_data (G_OBJECT (combo), "control_info");
	InputControl * c = s->control + ci->idx;
	int index = gtk_combo_box_get_active (combo);
	int val=0;
		
	if (c->id == V4L2_CID_EXPOSURE_AUTO) {
		val=exp_vals[videoIn->available_exp[index]];	
	} else {	
		val=index;
	}

	if (input_set_control (videoIn, c, val) == 0) {
		printf ("changed to %d\n", val);
	}
	else {
		printf ("changed to %d, but set failed\n", val);
	}
	
	if (input_get_control (videoIn, c, &val) == 0) {
		printf ("hardware value is %d\n", val);
	}
	else {
		printf ("hardware get failed\n");
	}
	
}

/*resolution control callback*/
static void
resolution_changed (GtkComboBox * Resolution, void *data)
{
	/* The new resolution is writen to conf file at exit             */
	/* then is read back at start. This means that for changing */
	/* resolution we must restart the application                    */
	
	int index = gtk_combo_box_get_active(Resolution);
	global->width=videoIn->listVidCap[global->formind][index].width;
	global->height=videoIn->listVidCap[global->formind][index].height;
	
	/*check if frame rate is available at the new resolution*/
	int i=0;
	int deffps=0;
	for(i=0;i<videoIn->listVidCap[global->formind][index].numb_frates;i++) {
		if ((videoIn->listVidCap[global->formind][index].framerate_num[i]==global->fps_num) && 
			   (videoIn->listVidCap[global->formind][index].framerate_denom[i]==global->fps)) deffps=i;	
	}
	/*frame rate is not available so set to minimum*/
	if (deffps==0) {
		global->fps_num=videoIn->listVidCap[global->formind][index].framerate_num[0];
		global->fps=videoIn->listVidCap[global->formind][index].framerate_denom[0];		
	}


	
	restartdialog = gtk_dialog_new_with_buttons ("Program Restart",
												  GTK_WINDOW(mainwin),
												  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
												  "now",
												  GTK_RESPONSE_ACCEPT,
												  "Later",
												  GTK_RESPONSE_REJECT,
												  NULL);
	
	GtkWidget *message = gtk_label_new ("Changes will only take effect after guvcview restart.\n\n\nRestart now?\n");
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(restartdialog)->vbox), message);
	gtk_widget_show_all(GTK_WIDGET(GTK_CONTAINER (GTK_DIALOG(restartdialog)->vbox)));
	
	gint result = gtk_dialog_run (GTK_DIALOG (restartdialog));
	switch (result) {
		case GTK_RESPONSE_ACCEPT:
			/*restart app*/
			shutd(1);
			break;
		default:
			/* do nothing since Restart rejected*/		
			break;
	}
  
	gtk_widget_destroy (restartdialog);
		
}

/*frame rate control callback*/
static void
FrameRate_changed (GtkComboBox * FrameRate,GtkComboBox * Resolution)
{
	int resind = gtk_combo_box_get_active(Resolution);
	
	int index = gtk_combo_box_get_active (FrameRate);
		
	global->fps=videoIn->listVidCap[global->formind][resind].framerate_denom[index];
	global->fps_num=videoIn->listVidCap[global->formind][resind].framerate_num[index];
 
	input_set_framerate (videoIn, global->fps, global->fps_num);
	
	input_get_framerate(videoIn);
	global->fps=videoIn->fps;
	global->fps_num=videoIn->fps_num;
	printf("hardware fps is %d/%d ,%i/%i\n",global->fps,global->fps_num,
				videoIn->streamparm.parm.capture.timeperframe.numerator,
				videoIn->streamparm.parm.capture.timeperframe.denominator);
	
}

/*sound sample rate control callback*/
static void
SndSampleRate_changed (GtkComboBox * SampleRate, void *data)
{
	global->Sound_SampRateInd = gtk_combo_box_get_active (SampleRate);
	global->Sound_SampRate=stdSampleRates[global->Sound_SampRateInd];
	
	
}

/*image type control callback*/
static void
ImageType_changed (GtkComboBox * ImageType,GtkEntry *ImageFNameEntry) 
{
	char *filename;
	char basename[16];
	videoIn->Imgtype=gtk_combo_box_get_active (ImageType);	
	filename=gtk_entry_get_text(ImageFNameEntry);
	
	splitPath(filename, global->imgPath, global->imageName);
	
	sscanf(global->imageName,"%16[^.]",basename);
	switch(videoIn->Imgtype){
		case 0:
			sprintf(global->imageName,"%s.jpg",basename);
			break;
		case 1:
			sprintf(global->imageName,"%s.bmp",basename);
			break;
		case 2:
			sprintf(global->imageName,"%s.png",basename);
			break;
		default:
			global->imageName=DEFAULT_IMAGE_FNAME;
	}
	gtk_entry_set_text(ImageFNameEntry," ");
	gtk_entry_set_text(ImageFNameEntry,global->imageName);
	sprintf(videoIn->ImageFName,"%s/%s",global->imgPath,global->imageName);
}

/*sound device control callback*/
static void
SndDevice_changed (GtkComboBox * SoundDevice, void *data)
{
 
	global->Sound_UseDev=gtk_combo_box_get_active (SoundDevice);
	
	printf("using device id:%d\n",global->Sound_IndexDev[global->Sound_UseDev].id);
	
}

/*sound channels control callback*/
static void
SndNumChan_changed (GtkComboBox * SoundChan, void *data)
{
	/*0-device default 1-mono 2-stereo*/
	global->Sound_NumChanInd = gtk_combo_box_get_active (SoundChan);
	global->Sound_NumChan=global->Sound_NumChanInd;
}

/*avi compression control callback*/
static void
AVIComp_changed (GtkComboBox * AVIComp, void *data)
{
	int index = gtk_combo_box_get_active (AVIComp);
		
	global->AVIFormat=index;
}

/* sound enable check box callback */
static void
SndEnable_changed (GtkToggleButton * toggle, VidState * s)
{
		global->Sound_enable = gtk_toggle_button_get_active (toggle) ? 1 : 0;
		if (!global->Sound_enable) {
			gtk_widget_set_sensitive (SndSampleRate,FALSE);
			gtk_widget_set_sensitive (SndDevice,FALSE);
			gtk_widget_set_sensitive (SndNumChan,FALSE);	
		} else { 
			gtk_widget_set_sensitive (SndSampleRate,TRUE);
			gtk_widget_set_sensitive (SndDevice,TRUE);
			gtk_widget_set_sensitive (SndNumChan,TRUE);
		}
}

/* Mirror check box callback */
static void
FiltMirrorEnable_changed(GtkToggleButton * toggle, void *data)
{
	global->Frame_Flags = gtk_toggle_button_get_active (toggle) ? 
				(global->Frame_Flags | YUV_MIRROR) : 
					(global->Frame_Flags & ~YUV_MIRROR);
}

/* Upturn check box callback */
static void
FiltUpturnEnable_changed(GtkToggleButton * toggle, void *data)
{
	global->Frame_Flags = gtk_toggle_button_get_active (toggle) ? 
				(global->Frame_Flags | YUV_UPTURN) : 
					(global->Frame_Flags & ~YUV_UPTURN);
}

/* Negate check box callback */
static void
FiltNegateEnable_changed(GtkToggleButton * toggle, void *data)
{
	global->Frame_Flags = gtk_toggle_button_get_active (toggle) ? 
				(global->Frame_Flags | YUV_NEGATE) : 
					(global->Frame_Flags & ~YUV_NEGATE);
}

/* Upturn check box callback */
static void
FiltMonoEnable_changed(GtkToggleButton * toggle, void *data)
{
	global->Frame_Flags = gtk_toggle_button_get_active (toggle) ? 
				(global->Frame_Flags | YUV_MONOCR) : 
					(global->Frame_Flags & ~YUV_MONOCR);
}
/*--------------------------- file chooser dialog ----------------------------*/
static void
file_chooser (GtkButton * FileButt, const int isAVI)
{
  char str_ext[3];	
  char *basename;
  char *fullname;
	
  FileDialog = gtk_file_chooser_dialog_new ("Save File",
					  mainwin,
					  GTK_FILE_CHOOSER_ACTION_SAVE,
					  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					  GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					  NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (FileDialog), TRUE);

  if(isAVI) { /* avi File chooser*/
	
	basename =  gtk_entry_get_text(AVIFNameEntry);
	
	splitPath(basename, global->aviPath, global->aviName);
	
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (FileDialog),
															  global->aviPath);
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (FileDialog), 
															  global->aviName);
	  
	if (gtk_dialog_run (GTK_DIALOG (FileDialog)) == GTK_RESPONSE_ACCEPT)
	{
		fullname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (FileDialog));
		splitPath(fullname, global->aviPath, global->aviName);
		gtk_entry_set_text(AVIFNameEntry," ");
		gtk_entry_set_text(AVIFNameEntry,global->aviName);
	}
	  
  } else {/* Image File chooser*/
	
	basename =  gtk_entry_get_text(ImageFNameEntry);
	
	splitPath(basename, global->imgPath, global->imageName);
	
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (FileDialog), 
															  global->imgPath);
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (FileDialog), 
															global->imageName);

	  
	if (gtk_dialog_run (GTK_DIALOG (FileDialog)) == GTK_RESPONSE_ACCEPT)
	{
		fullname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (FileDialog));
		splitPath(fullname, global->imgPath, global->imageName);
		
		gtk_entry_set_text(ImageFNameEntry," ");
		gtk_entry_set_text(ImageFNameEntry,global->imageName);
		
		/*get the file extension*/
		sscanf(global->imageName,"%*[^.].%3c",str_ext);
		/* change image type */
		int somExt = str_ext[0]+str_ext[1]+str_ext[2];
		switch (somExt) {
			/* there are 8 variations we will check for 3*/
			case ('j'+'p'+'g'):
			case ('J'+'P'+'G'):
			case ('J'+'p'+'g'):
				gtk_combo_box_set_active(GTK_COMBO_BOX(ImageType),0);
				break;
			case ('b'+'m'+'p'):	
			case ('B'+'M'+'P'):
			case ('B'+'m'+'p'):
				gtk_combo_box_set_active(GTK_COMBO_BOX(ImageType),1);
				break;
			case ('p'+'n'+'g'):			
			case ('P'+'N'+'G'):		
			case ('P'+'n'+'g'):
				gtk_combo_box_set_active(GTK_COMBO_BOX(ImageType),2);
				break;
			default: /* use jpeg as default*/
				gtk_combo_box_set_active(GTK_COMBO_BOX(ImageType),0);
		}
			
	}
	  
  }
  gtk_widget_destroy (FileDialog);
	
}

/*----------------------------- Capture Image --------------------------------*/
/*image capture button callback*/
static void
capture_image (GtkButton * CapImageButt, GtkWidget * ImageFNameEntry)
{
	char *fileEntr=gtk_entry_get_text(GTK_ENTRY(ImageFNameEntry));
	splitPath(fileEntr, global->imgPath, global->imageName);
	
	int sfname=strlen(global->imgPath)+strlen(global->imageName);
	char filename[sfname+2];
	
	sprintf(filename,"%s/%s", global->imgPath,global->imageName);
	//videoIn->ImageFName=realloc(videoIn->ImageFName,(sfname+2)*sizeof(char));
	if ((sfname+2)>120) {
		printf("Error: image file name too big, unchanged.\n");
	} else {
		videoIn->ImageFName=strncpy(videoIn->ImageFName,filename,sfname+2);
	}
	//printf("imag file: %s\n",videoIn->ImageFName);
	
	videoIn->capImage = TRUE;
}

/*--------------------------- Capture AVI ------------------------------------*/
/*avi capture button callback*/
static void
capture_avi (GtkButton * CapAVIButt, GtkWidget * AVIFNameEntry)
{
	char *fileEntr = gtk_entry_get_text(GTK_ENTRY(AVIFNameEntry));
	
	char *compression="MJPG";

	switch (global->AVIFormat) {
		case 0:
			compression="MJPG";
			break;
		case 1:
			compression="YUY2";
			break;
		case 2:
			compression="DIB ";
			break;
		default:
			compression="MJPG";
	}	
	if(videoIn->capAVI) {  /************* Stop AVI ************/
		//printf("stoping AVI capture\n");
		gtk_button_set_label(CapAVIButt,"Capture");
		global->AVIstoptime = ms_time();
		printf("AVI stop time:%d\n",global->AVIstoptime);	
		videoIn->capAVI = FALSE;
		aviClose();
		/*enabling sound and avi compression controls*/
		gtk_widget_set_sensitive (AVIComp, TRUE);
		gtk_widget_set_sensitive (SndEnable,TRUE);	
		if(global->Sound_enable > 0) {	 
			gtk_widget_set_sensitive (SndSampleRate,TRUE);
			gtk_widget_set_sensitive (SndDevice,TRUE);
			gtk_widget_set_sensitive (SndNumChan,TRUE);
		}
	} 
	else {
	
		if (!(videoIn->signalquit)) { /***should not happen ***/
			/*thread exited while in AVI capture mode*/
			/* we have to close AVI                  */
			printf("close AVI since thread as exited\n");
			gtk_button_set_label(CapAVIButt,"Capture");
			global->AVIstoptime = ms_time();
			printf("AVI stop time:%d\n",global->AVIstoptime);	
			videoIn->capAVI = FALSE;
			aviClose();
			/*enabling sound and avi compression controls*/
			gtk_widget_set_sensitive (AVIComp, TRUE);
			gtk_widget_set_sensitive (SndEnable,TRUE); 
			if(global->Sound_enable > 0) {	 
				gtk_widget_set_sensitive (SndSampleRate,TRUE);
				gtk_widget_set_sensitive (SndDevice,TRUE);
				gtk_widget_set_sensitive (SndNumChan,TRUE);
			}
	 
		} 
		else {/******************** Start AVI *********************/
			/* thread is running so start AVI capture*/
			splitPath(fileEntr, global->aviPath, global->aviName);
	
			int sfname=strlen(global->aviPath)+strlen(global->aviName);
			char filename[sfname+2];
	
			sprintf(filename,"%s/%s", global->aviPath,global->aviName);
			//videoIn->ImageFName=realloc(videoIn->aviFName,(sfname+2)*sizeof(char));
			if ((sfname+2)>120) {
				printf("Error: image file name too big, unchanged.\n");
			} else {	
				videoIn->AVIFName=strncpy(videoIn->AVIFName,filename,sfname+2);
			}
			//printf("avi file: %s\n",videoIn->AVIFName);
			 
			gtk_button_set_label(CapAVIButt,"Stop");  
			AviOut = AVI_open_output_file(videoIn->AVIFName);
			/*4CC compression "YUY2" (YUV) or "DIB " (RGB24)  or  "MJPG"*/	
	   
			AVI_set_video(AviOut, videoIn->width, videoIn->height, videoIn->fps,compression);		
			/* audio will be set in aviClose - if enabled*/
			global->AVIstarttime = ms_time();
			//printf("AVI start time:%d\n",AVIstarttime);		
			videoIn->capAVI = TRUE; /* start video capture */
			/*disabling sound and avi compression controls*/
			gtk_widget_set_sensitive (AVIComp, FALSE);
			gtk_widget_set_sensitive (SndEnable,FALSE); 
			gtk_widget_set_sensitive (SndSampleRate,FALSE);
			gtk_widget_set_sensitive (SndDevice,FALSE);
			gtk_widget_set_sensitive (SndNumChan,FALSE);
			/* Creating the sound capture loop thread if Sound Enable*/ 
			if(global->Sound_enable > 0) { 
				/* Initialize and set snd thread detached attribute */
				pthread_attr_init(&sndattr);
				pthread_attr_setdetachstate(&sndattr, PTHREAD_CREATE_JOINABLE);
		   
				int rsnd = pthread_create(&sndthread, &sndattr, sound_capture, NULL); 
				if (rsnd)
				{
					printf("ERROR; return code from snd pthread_create() is %d\n", rsnd);
				}
			}
		 
		}
	}	
}

/* called by capture from start timer [-t seconds] command line option*/
int timer_callback(){
	/*stop avi capture*/
	//printf("timer alarme - stoping avi\n");
	capture_avi(CapAVIButt,AVIFNameEntry);
	global->Capture_time=0; 
	return (FALSE);/*destroys the timer*/
}

/*--------------------------- draw camera controls ---------------------------*/
static void
draw_controls (VidState *s)
{
	int i;
	
	
	if (s->control) {
		for (i = 0; i < s->num_controls; i++) {
			ControlInfo * ci = s->control_info + i;
			if (ci->widget)
				gtk_widget_destroy (ci->widget);
			if (ci->label)
				gtk_widget_destroy (ci->label);
			if (ci->labelval)
				gtk_widget_destroy (ci->labelval);
		}
		free (s->control_info);
		s->control_info = NULL;
		input_free_controls (s->control, s->num_controls);
		s->control = NULL;
	}
	
	s->control = input_enum_controls (videoIn, &s->num_controls);
	//fprintf(stderr,"V4L2_CID_BASE=0x%x\n",V4L2_CID_BASE);
	//fprintf(stderr,"V4L2_CID_PRIVATE_BASE=0x%x\n",V4L2_CID_PRIVATE_BASE);
	//fprintf(stderr,"V4L2_CID_PRIVATE_LAST=0x%x\n",V4L2_CID_PRIVATE_LAST);
	fprintf(stderr,"Controls:\n");
	for (i = 0; i < s->num_controls; i++) {
		fprintf(stderr,"control[%d]: 0x%x",i,s->control[i].id);
		fprintf (stderr,"  %s, %d:%d:%d, default %d\n", s->control[i].name,
					s->control[i].min, s->control[i].step, s->control[i].max,
					s->control[i].default_val);
	}

   if((s->control_info = malloc (s->num_controls * sizeof (ControlInfo)))==NULL){
			printf("couldn't allocate memory for: s->control_info\n");
			exit(1); 
   }

	for (i = 0; i < s->num_controls; i++) {
		ControlInfo * ci = s->control_info + i;
		InputControl * c = s->control + i;

		ci->idx = i;
		ci->widget = NULL;
		ci->label = NULL;
		ci->labelval = NULL;
		
		if (c->id == V4L2_CID_EXPOSURE_AUTO) {
			//~ int val;
			//~ ci->widget = gtk_check_button_new_with_label (c->name);
			//~ g_object_set_data (G_OBJECT (ci->widget), "control_info", ci);
			//~ gtk_widget_show (ci->widget);
			//~ gtk_table_attach (GTK_TABLE (s->table), ci->widget, 1, 3, 3+i, 4+i,
					//~ GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
			

			//~ if (input_get_control (videoIn, c, &val) == 0) {
				//~ gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ci->widget),
						//~ val==AUTO_EXP ? TRUE : FALSE);
			//~ }
			//~ else {
				//~ gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ci->widget),
						//~ c->default_val==AUTO_EXP ? TRUE : FALSE);
				//~ gtk_widget_set_sensitive (ci->widget, FALSE);
			//~ }

			//~ if (!c->enabled) {
				//~ gtk_widget_set_sensitive (ci->widget, FALSE);
			//~ }
			
			//~ g_signal_connect (G_OBJECT (ci->widget), "toggled",
					//~ G_CALLBACK (check_changed), s);
			int j=0;
			int val=0;
			/* test available modes */
			int def=0;
			input_get_control (videoIn, c, &def);/*get stored value*/

			for (j=0;j<4;j++) {
				if (input_set_control (videoIn, c, exp_vals[j]) == 0) {
					videoIn->available_exp[val]=j;/*store index to values*/
					val++;
				}
			}
			input_set_control (videoIn, c, def);/*set back to stored*/
			
			ci->widget = gtk_combo_box_new_text ();
			for (j = 0; j <val; j++) {
				gtk_combo_box_append_text (GTK_COMBO_BOX (ci->widget), 
										   exp_typ[videoIn->available_exp[j]]);
				if (def==exp_vals[videoIn->available_exp[j]]){
					gtk_combo_box_set_active (GTK_COMBO_BOX (ci->widget), j);
				}
			}

			gtk_table_attach (GTK_TABLE (s->table), ci->widget, 1, 2, 3+i, 4+i,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
			g_object_set_data (G_OBJECT (ci->widget), "control_info", ci);
			gtk_widget_show (ci->widget);

			//~ if (input_get_control (videoIn, c, &val) == 0) {
				switch(val){
					case (V4L2_EXPOSURE_MANUAL):
						j=0;
						break;
					case (V4L2_EXPOSURE_AUTO):
						j=1;
						break;
					case (V4L2_EXPOSURE_SHUTTER_PRIORITY):
						j=2;
						break;
					case (V4L2_EXPOSURE_APERTURE_PRIORITY):
						j=3;
						break;
					default :
						j=1;
				}
				//~ for (j = 0; j <4; j++) {
					//~ if (exp_vals[j]==val) break;
				//~ }
				//~ gtk_combo_box_set_active (GTK_COMBO_BOX (ci->widget), j);
			//~ }
			//~ else {
				//~ gtk_combo_box_set_active (GTK_COMBO_BOX (ci->widget), c->default_val);
				//~ gtk_widget_set_sensitive (ci->widget, FALSE);
			//~ }

			if (!c->enabled) {
				gtk_widget_set_sensitive (ci->widget, FALSE);
			}
			
			g_signal_connect (G_OBJECT (ci->widget), "changed",
					G_CALLBACK (combo_changed), s);

			ci->label = gtk_label_new ("Exposure:");	
			
		} else if (c->type == INPUT_CONTROL_TYPE_INTEGER) {
			PangoFontDescription * desc;
			int val;

			if (c->step == 0)
				c->step = 1;
			ci->widget = gtk_hscale_new_with_range (c->min, c->max, c->step);
			gtk_scale_set_draw_value (GTK_SCALE (ci->widget), FALSE);

			/* This is a hack to use always round the HScale to integer
			 * values.  Strangely, this functionality is normally only
			 * available when draw_value is TRUE. */
			GTK_RANGE (ci->widget)->round_digits = 0;

			gtk_table_attach (GTK_TABLE (s->table), ci->widget, 1, 2, 3+i, 4+i,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
			g_object_set_data (G_OBJECT (ci->widget), "control_info", ci);
			ci->maxchars = MAX (num_chars (c->min), num_chars (c->max));
			gtk_widget_show (ci->widget);
			
			ci->labelval = gtk_label_new (NULL);
			gtk_table_attach (GTK_TABLE (s->table), ci->labelval, 2, 3,
					3+i, 4+i, GTK_FILL, 0, 0, 0);
			
			desc = pango_font_description_new ();
			pango_font_description_set_family_static (desc, "monospace");
			gtk_widget_modify_font (ci->labelval, desc);
			gtk_misc_set_alignment (GTK_MISC (ci->labelval), 1, 0.5);

			if (input_get_control (videoIn, c, &val) == 0) {
				gtk_range_set_value (GTK_RANGE (ci->widget), val);
			}
			else {
				gtk_range_set_value (GTK_RANGE (ci->widget), c->default_val);
				gtk_widget_set_sensitive (ci->widget, FALSE);
				gtk_widget_set_sensitive (ci->labelval, FALSE);
			}

			if (!c->enabled) {
				gtk_widget_set_sensitive (ci->widget, FALSE);
				gtk_widget_set_sensitive (ci->labelval, FALSE);
			}
			
			set_slider_label (GTK_RANGE (ci->widget));
			g_signal_connect (G_OBJECT (ci->widget), "value-changed",
					G_CALLBACK (slider_changed), s);

			gtk_widget_show (ci->labelval);

			ci->label = gtk_label_new (g_strdup_printf ("%s:", c->name));
		}
		else if (c->type == INPUT_CONTROL_TYPE_BOOLEAN) {
			int val;
			ci->widget = gtk_check_button_new_with_label (c->name);
			g_object_set_data (G_OBJECT (ci->widget), "control_info", ci);
			gtk_widget_show (ci->widget);
			gtk_table_attach (GTK_TABLE (s->table), ci->widget, 1, 3, 3+i, 4+i,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);

			if (input_get_control (videoIn, c, &val) == 0) {
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ci->widget),
						val ? TRUE : FALSE);
			}
			else {
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ci->widget),
						c->default_val ? TRUE : FALSE);
				gtk_widget_set_sensitive (ci->widget, FALSE);
			}

			if (!c->enabled) {
				gtk_widget_set_sensitive (ci->widget, FALSE);
			}
			
			g_signal_connect (G_OBJECT (ci->widget), "toggled",
					G_CALLBACK (check_changed), s);
		}
		else if (c->type == INPUT_CONTROL_TYPE_MENU) {
			int val, j;

			ci->widget = gtk_combo_box_new_text ();
			for (j = 0; j <= c->max; j++) {
				gtk_combo_box_append_text (GTK_COMBO_BOX (ci->widget), c->entries[j]);
			}

			gtk_table_attach (GTK_TABLE (s->table), ci->widget, 1, 2, 3+i, 4+i,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
			g_object_set_data (G_OBJECT (ci->widget), "control_info", ci);
			gtk_widget_show (ci->widget);

			if (input_get_control (videoIn, c, &val) == 0) {
				gtk_combo_box_set_active (GTK_COMBO_BOX (ci->widget), val);
			}
			else {
				gtk_combo_box_set_active (GTK_COMBO_BOX (ci->widget), c->default_val);
				gtk_widget_set_sensitive (ci->widget, FALSE);
			}

			if (!c->enabled) {
				gtk_widget_set_sensitive (ci->widget, FALSE);
			}
			
			g_signal_connect (G_OBJECT (ci->widget), "changed",
					G_CALLBACK (combo_changed), s);

			ci->label = gtk_label_new (g_strdup_printf ("%s:", c->name));
		}
		else {
			fprintf (stderr, "TODO: implement menu and button\n");
			continue;
		}

		if (ci->label) {
			gtk_misc_set_alignment (GTK_MISC (ci->label), 1, 0.5);

			gtk_table_attach (GTK_TABLE (s->table), ci->label, 0, 1, 3+i, 4+i,
					GTK_FILL, 0, 0, 0);

			gtk_widget_show (ci->label);
		}
	}

}


/*-------------------------------- Main Video Loop ---------------------------*/ 
/* run in a thread (SDL overlay)*/
void *main_loop(void *data)
{
	//int ret=0;
	while (videoIn->signalquit) {
	 /*-------------------------- Grab Frame ----------------------------------*/
	 if (uvcGrab(videoIn) < 0) {
		printf("Error grabbing image \n");
		videoIn->signalquit=0;
		pthread_exit((void *) 2);
	 }
	
	 /*------------------------- Filter Frame ---------------------------------*/
	 if(global->Frame_Flags>0){
		if((global->Frame_Flags & YUV_MIRROR)==YUV_MIRROR)
			yuyv_mirror(videoIn->framebuffer,videoIn->width,videoIn->height);
		if((global->Frame_Flags & YUV_UPTURN)==YUV_UPTURN)
			yuyv_upturn(videoIn->framebuffer,videoIn->width,videoIn->height);
		if((global->Frame_Flags & YUV_NEGATE)==YUV_NEGATE)
			yuyv_negative (videoIn->framebuffer,videoIn->width,videoIn->height);
		if((global->Frame_Flags & YUV_MONOCR)==YUV_MONOCR)
			 yuyv_monochrome (videoIn->framebuffer,videoIn->width,videoIn->height);
	 }
	
	 /*-------------------------capture Image----------------------------------*/
	 //char fbasename[20];
	 if (videoIn->capImage){
		 switch(videoIn->Imgtype) {
		 case 0:/*jpg*/
			/* Save directly from MJPG frame */	 
			if((global->Frame_Flags==0) && (videoIn->formatIn==V4L2_PIX_FMT_MJPEG)) {
				if(SaveJPG(videoIn->ImageFName,videoIn->buf.bytesused,videoIn->tmpbuffer)) {
					fprintf (stderr,"Error: Couldn't capture Image to %s \n",
					videoIn->ImageFName);		
				}	
			} else { /* use built in encoder */
				if (!global->jpeg){ 
					if((global->jpeg = (BYTE*)malloc(global->jpeg_bufsize))==NULL) {
						printf("couldn't allocate memory for: jpeg buffer\n");
						exit(1);
					}				
				}
				if(!jpeg_struct) {
					if((jpeg_struct =(struct JPEG_ENCODER_STRUCTURE *) calloc(1, sizeof(struct JPEG_ENCODER_STRUCTURE)))==NULL){
						printf("couldn't allocate memory for: jpeg encoder struct\n");
						exit(1); 
					} else {
						/* Initialization of JPEG control structure */
						initialization (jpeg_struct,videoIn->width,videoIn->height);
	
						/* Initialization of Quantization Tables  */
						initialize_quantization_tables (jpeg_struct);
					}
				} 
				global->jpeg_size = encode_image(videoIn->framebuffer, global->jpeg, 
								jpeg_struct,1, videoIn->width, videoIn->height);
			 
				if(SaveBuff(videoIn->ImageFName,global->jpeg_size,global->jpeg)) { 
					fprintf (stderr,"Error: Couldn't capture Image to %s \n",
					videoIn->ImageFName);		
				}
			}
			break;
		 case 1:/*bmp*/
			if(pim==NULL) {  
				 /*24 bits -> 3bytes     32 bits ->4 bytes*/
				if((pim= malloc((pscreen->w)*(pscreen->h)*3))==NULL){
					printf("Couldn't allocate memory for: pim\n");
					videoIn->signalquit=0;
					pthread_exit((void *) 3);		
				}
			}
			yuyv2bgr(videoIn->framebuffer,pim,videoIn->width,videoIn->height);

			if(SaveBPM(videoIn->ImageFName, videoIn->width, videoIn->height, 24, pim)) {
				  fprintf (stderr,"Error: Couldn't capture Image to %s \n",
				  videoIn->ImageFName);
			} 
			else {	  
				//printf ("Capture BMP Image to %s \n",videoIn->ImageFName);
			}
			break;
		 case 2:/*png*/
			if(pim==NULL) {  
				 /*24 bits -> 3bytes     32 bits ->4 bytes*/
				if((pim= malloc((pscreen->w)*(pscreen->h)*3))==NULL){
					printf("Couldn't allocate memory for: pim\n");
					videoIn->signalquit=0;
					pthread_exit((void *) 3);		
				}
			}
			 yuyv2rgb(videoIn->framebuffer,pim,videoIn->width,videoIn->height);
			 write_png(videoIn->ImageFName, videoIn->width, videoIn->height,pim);
		 }
		 videoIn->capImage=FALSE;
		 printf("saved image to:%s\n",videoIn->ImageFName);
	  }
	  
	  /*---------------------------capture AVI---------------------------------*/
	  if (videoIn->capAVI && videoIn->signalquit){
	   long framesize;		
	   switch (global->AVIFormat) {
		   
		case 0: /*MJPG*/
			/* save MJPG frame */   
			if((global->Frame_Flags==0) && (videoIn->formatIn==V4L2_PIX_FMT_MJPEG)) {
				if (AVI_write_frame (AviOut,
								videoIn->tmpbuffer, videoIn->buf.bytesused) < 0)
					printf ("write error on avi out \n");
			} else {  /* use built in encoder */ 
				if (!global->jpeg){ 
					if((global->jpeg = (BYTE*)malloc(global->jpeg_bufsize))==NULL) {
						printf("couldn't allocate memory for: jpeg buffer\n");
						exit(1);
					}				
				}
				if(!jpeg_struct) {
					if((jpeg_struct =(struct JPEG_ENCODER_STRUCTURE *) calloc(1, sizeof(struct JPEG_ENCODER_STRUCTURE)))==NULL){
						printf("couldn't allocate memory for: jpeg encoder struct\n");
						exit(1); 
					} else {
						/* Initialization of JPEG control structure */
						initialization (jpeg_struct,videoIn->width,videoIn->height);
	
						/* Initialization of Quantization Tables  */
						initialize_quantization_tables (jpeg_struct);
					}
				} 
				global->jpeg_size = encode_image(videoIn->framebuffer, global->jpeg, 
								jpeg_struct,1, videoIn->width, videoIn->height);
			
				if (AVI_write_frame (AviOut, global->jpeg, global->jpeg_size) < 0)
					printf ("write error on avi out \n");
			}
			break;
		case 1:
		   framesize=(pscreen->w)*(pscreen->h)*2; /*YUY2 -> 2 bytes per pixel */
			   if (AVI_write_frame (AviOut,
				   p, framesize) < 0)
					printf ("write error on avi out \n");
		   break;
		case 2:
			framesize=(pscreen->w)*(pscreen->h)*3; /*DIB 24/32 -> 3/4 bytes per pixel*/ 
			if(pavi==NULL){
			  if((pavi= malloc(framesize))==NULL){
				printf("Couldn't allocate memory for: pim\n");
				videoIn->signalquit=0;
				pthread_exit((void *) 3);
			  }
			}
			yuyv2bgr(videoIn->framebuffer,pavi,videoIn->width,videoIn->height); 
			if (AVI_write_frame (AviOut,pavi, framesize) < 0)
				printf ("write error on avi out \n");
			break;

		} 
	   global->framecount++;	   
	  } 
	/*------------------------- Display Frame --------------------------------*/
	 SDL_LockYUVOverlay(overlay);
	 memcpy(p, videoIn->framebuffer,
		   videoIn->width * (videoIn->height) * 2);
	 SDL_UnlockYUVOverlay(overlay);
	 SDL_DisplayYUVOverlay(overlay, &drect);
	 //~ SDL_Delay(SDL_WAIT_TIME);
	
  }
  
   /*check if thread exited while AVI in capture mode*/
  if (videoIn->capAVI) {
	global->AVIstoptime = ms_time();
	videoIn->capAVI = FALSE;   
  }	   
  printf("Thread terminated...\n");
  
  if(pim!=NULL) free(pim);
  pim=NULL;
  if(pavi!=NULL)	free(pavi);
  pavi=NULL;
  printf("cleanig Thread allocations 100%%\n");
  fflush(NULL);//flush all output buffers
	
  pthread_exit((void *) 0);
}

/*--------------------------------- MAIN -------------------------------------*/
int main(int argc, char *argv[])
{
	int i;
	
	if((EXEC_CALL=malloc(strlen(argv[0])*sizeof(char)))==NULL) {
		printf("couldn't allocate memory for: EXEC_CALL)\n");
		exit(1);
	}
	strcpy(EXEC_CALL,argv[0]);/*stores argv[0] - program call string*/
	
	/*set global variables*/
	if((global=(struct GLOBAL *) calloc(1, sizeof(struct GLOBAL)))==NULL){
		printf("couldn't allocate memory for: global\n");
		exit(1); 
	}
	printf("initing globals\n");
	initGlobals(global);
						  
	const SDL_VideoInfo *info;
	char driver[128];
	GtkWidget * boxh;
	GtkWidget *Resolution;
	GtkWidget *FrameRate;
	GtkWidget *label_FPS;
	GtkWidget *table2;
	GtkWidget *labelResol;
	GtkWidget *CapImageButt;
	GtkWidget *ImgFileButt;
	GtkWidget *AviFileButt;
	GtkWidget *label_ImageType;
	GtkWidget *label_AVIComp;
	GtkWidget *label_SndSampRate;
	GtkWidget *label_SndDevice;
	GtkWidget *label_SndNumChan;
	GtkWidget *label_videoFilters;
	GtkWidget *table3;
	
	if ((s = malloc (sizeof (VidState)))==NULL){
		printf("couldn't allocate memory for: s\n");
		exit(1); 
	}
	
	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	   
	const char *home;
	const char *pwd;
	
	home = getenv("HOME");
	pwd = getenv("PWD");
	
	sprintf(global->confPath,"%s%s", home,"/.guvcviewrc");
	sprintf(global->aviPath,"%s", pwd);
	sprintf(global->imgPath,"%s", pwd);
	
	printf("conf Path is %s\n",global->confPath);
	readConf(global->confPath);

	printf("guvcview version %s \n", VERSION);
	
	/*------------------------ reads command line options --------------------*/
	readOpts(argc,argv);
		
	/*---------------------------- GTK init ----------------------------------*/
	
	gtk_init(&argc, &argv);
	

	/* Create a main window */
	mainwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (mainwin), "GUVCViewer Controls");
	//gtk_widget_set_usize(mainwin, winwidth, winheight);
	gtk_window_resize(GTK_WINDOW(mainwin),global->winwidth,global->winheight);
	/* Add event handlers */
	gtk_signal_connect(GTK_OBJECT(mainwin), "delete_event", GTK_SIGNAL_FUNC(delete_event), 0);
	
	
	/*----------------------------- Test SDL capabilities ---------------------*/
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) < 0) {
	fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
	exit(1);
	}
	
	/* For this version, we will use hardware acceleration as default*/
	if(global->hwaccel) {
		if ( ! getenv("SDL_VIDEO_YUV_HWACCEL") ) {
			putenv("SDL_VIDEO_YUV_HWACCEL=1");
		}
		if ( ! getenv("SDL_VIDEO_YUV_DIRECT") ) {
			putenv("SDL_VIDEO_YUV_DIRECT=1"); 
		}
	 } else {
		if ( ! getenv("SDL_VIDEO_YUV_HWACCEL") ) {
			putenv("SDL_VIDEO_YUV_HWACCEL=0");
		}
		if ( ! getenv("SDL_VIDEO_YUV_DIRECT") ) {
			putenv("SDL_VIDEO_YUV_DIRECT=0"); 
		}
	 }
	 
	if (SDL_VideoDriverName(driver, sizeof(driver))) {
	printf("Video driver: %s\n", driver);
	}
	info = SDL_GetVideoInfo();

	if (info->wm_available) {
	printf("A window manager is available\n");
	}
	if (info->hw_available) {
	printf("Hardware surfaces are available (%dK video memory)\n",
		   info->video_mem);
	SDL_VIDEO_Flags |= SDL_HWSURFACE;
	}
	if (info->blit_hw) {
	printf("Copy blits between hardware surfaces are accelerated\n");
	SDL_VIDEO_Flags |= SDL_ASYNCBLIT;
	}
	if (info->blit_hw_CC) {
	printf
		("Colorkey blits between hardware surfaces are accelerated\n");
	}
	if (info->blit_hw_A) {
	printf("Alpha blits between hardware surfaces are accelerated\n");
	}
	if (info->blit_sw) {
	printf
		("Copy blits from software surfaces to hardware surfaces are accelerated\n");
	}
	if (info->blit_sw_CC) {
	printf
		("Colorkey blits from software surfaces to hardware surfaces are accelerated\n");
	}
	if (info->blit_sw_A) {
	printf
		("Alpha blits from software surfaces to hardware surfaces are accelerated\n");
	}
	if (info->blit_fill) {
	printf("Color fills on hardware surfaces are accelerated\n");
	}

	if (!(SDL_VIDEO_Flags & SDL_HWSURFACE)){
	SDL_VIDEO_Flags |= SDL_SWSURFACE;
	}
	/*----------------------- init videoIn structure --------------------------*/	
	if((videoIn = (struct vdIn *) calloc(1, sizeof(struct vdIn)))==NULL){
		printf("couldn't allocate memory for: videoIn\n");
		exit(1); 
	}
	if (init_videoIn
		(videoIn, (char *) global->videodevice, global->width,global->height, 
	 	global->format, global->grabmethod, global->fps, global->fps_num) < 0)
			exit(1);

	/* Set jpeg encoder buffer size */
	global->jpeg_bufsize=((videoIn->width)*(videoIn->height))>>1;
	
   //~  SDL_WM_SetCaption("GUVCVideo", NULL);

	
	/*-----------------------------GTK widgets---------------------------------*/
	s->table = gtk_table_new (1, 3, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (s->table), 10);
	gtk_table_set_col_spacings (GTK_TABLE (s->table), 10);
	gtk_container_set_border_width (GTK_CONTAINER (s->table), 10);
	gtk_widget_set_size_request (s->table, 440, -1);
	
	s->control = NULL;
	draw_controls(s);
	
	boxh = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (boxh), s->table, FALSE, FALSE, 0);
	gtk_widget_show (s->table);
	
	table2 = gtk_table_new(1,3,FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table2), 10);
	gtk_table_set_col_spacings (GTK_TABLE (table2), 10);
	gtk_container_set_border_width (GTK_CONTAINER (table2), 10);
	gtk_widget_set_size_request (table2, 350, -1);
	
	/* Resolution*/
	Resolution = gtk_combo_box_new_text ();
	char temp_str[9];
	int defres=0;
	for(i=0;i<videoIn->numb_resol;i++) {
		if (videoIn->listVidCap[global->formind][i].width>0){
			sprintf(temp_str,"%ix%i",videoIn->listVidCap[global->formind][i].width,
							 videoIn->listVidCap[global->formind][i].height);
			gtk_combo_box_append_text(Resolution,temp_str);
			if ((global->width==videoIn->listVidCap[global->formind][i].width) && 
				(global->height==videoIn->listVidCap[global->formind][i].height)){
				defres=i;/*set selected*/
			}
		}
	}
	gtk_combo_box_set_active(Resolution,defres);
	//~ if (defres==0) {
		//~ global->width=videoIn->listVidCap[global->formind][0].width;
		//~ global->height=videoIn->listVidCap[global->formind][0].height;
		//~ videoIn->width=global->width;
		//~ videoIn->height=global->height;
	//~ }
	printf("Def. Res: %i    numb. fps:%i\n",defres,videoIn->listVidCap[global->formind][defres].numb_frates);
	gtk_table_attach(GTK_TABLE(table2), Resolution, 1, 3, 3, 4,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
	gtk_widget_show (Resolution);
	
	gtk_widget_set_sensitive (Resolution, TRUE);
	g_signal_connect (Resolution, "changed",
			G_CALLBACK (resolution_changed), NULL);
	
	labelResol = gtk_label_new("Resolution:");
	gtk_misc_set_alignment (GTK_MISC (labelResol), 1, 0.5);

	gtk_table_attach (GTK_TABLE(table2), labelResol, 0, 1, 3, 4,
					GTK_FILL, 0, 0, 0);

	gtk_widget_show (labelResol);
	
	/* Frame Rate */
	input_set_framerate (videoIn, global->fps, global->fps_num);
				  
	FrameRate = gtk_combo_box_new_text ();
	int deffps=0;
	for(i=0;i<videoIn->listVidCap[global->formind][defres].numb_frates;i++) {
		sprintf(temp_str,"%i/%i fps",videoIn->listVidCap[global->formind][defres].framerate_num[i],
							 videoIn->listVidCap[global->formind][defres].framerate_denom[i]);
		gtk_combo_box_append_text(FrameRate,temp_str);
		if ((videoIn->fps_num==videoIn->listVidCap[global->formind][defres].framerate_num[i]) && 
				  (videoIn->fps==videoIn->listVidCap[global->formind][defres].framerate_denom[i])){
				deffps=i;/*set selected*/
		}
	}
	
	gtk_table_attach(GTK_TABLE(table2), FrameRate, 1, 3, 2, 3,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
	gtk_widget_show (FrameRate);
	
	
	gtk_combo_box_set_active(FrameRate,deffps);
	if (deffps==0) {
		global->fps=videoIn->listVidCap[global->formind][defres].framerate_denom[0];
		global->fps_num=videoIn->listVidCap[global->formind][0].framerate_num[0];
		videoIn->fps=global->fps;
		videoIn->fps_num=global->fps_num;
	}
		
	gtk_widget_set_sensitive (FrameRate, TRUE);
	g_signal_connect (GTK_COMBO_BOX(FrameRate), "changed",
		G_CALLBACK (FrameRate_changed), Resolution);
	
	label_FPS = gtk_label_new("Frame Rate:");
	gtk_misc_set_alignment (GTK_MISC (label_FPS), 1, 0.5);

	gtk_table_attach (GTK_TABLE(table2), label_FPS, 0, 1, 2, 3,
					GTK_FILL, 0, 0, 0);

	gtk_widget_show (label_FPS);
	
	/* Image Capture*/
	CapImageButt = gtk_button_new_with_label("Capture");
	ImageFNameEntry = gtk_entry_new();
	
	gtk_entry_set_text(GTK_ENTRY(ImageFNameEntry),DEFAULT_IMAGE_FNAME);
	
	gtk_table_attach(GTK_TABLE(table2), CapImageButt, 0, 1, 4, 5,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
	
	gtk_table_attach(GTK_TABLE(table2), ImageFNameEntry, 1, 2, 4, 5,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
	ImgFileButt=gtk_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_table_attach(GTK_TABLE(table2), ImgFileButt, 2, 3, 4, 5,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
	gtk_widget_show (ImgFileButt);
	
	label_ImageType=gtk_label_new("Image Type:");
	gtk_misc_set_alignment (GTK_MISC (label_ImageType), 1, 0.5);

	gtk_table_attach (GTK_TABLE(table2), label_ImageType, 0, 1, 5, 6,
					GTK_FILL, 0, 0, 0);

	gtk_widget_show (label_ImageType);
	
	ImageType=gtk_combo_box_new_text ();
	gtk_combo_box_append_text(GTK_COMBO_BOX(ImageType),"JPG");
	gtk_combo_box_append_text(GTK_COMBO_BOX(ImageType),"BMP");
	gtk_combo_box_append_text(GTK_COMBO_BOX(ImageType),"PNG");
	gtk_combo_box_set_active(GTK_COMBO_BOX(ImageType),0);
	gtk_table_attach(GTK_TABLE(table2), ImageType, 1, 2, 5, 6,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
	gtk_widget_show (ImageType);
	
	
	gtk_widget_show (CapImageButt);
	gtk_widget_show (ImageFNameEntry);
	gtk_widget_show (ImageType);
	g_signal_connect (GTK_COMBO_BOX(ImageType), "changed",
		G_CALLBACK (ImageType_changed), ImageFNameEntry);
	g_signal_connect (GTK_BUTTON(ImgFileButt), "clicked",
		 G_CALLBACK (file_chooser), 0);
	g_signal_connect (GTK_BUTTON(CapImageButt), "clicked",
		 G_CALLBACK (capture_image), ImageFNameEntry);
	
	
	/*AVI Capture*/
	AVIFNameEntry = gtk_entry_new();
	if (global->avifile) {	/*avi capture enabled from start*/
		CapAVIButt = gtk_button_new_with_label("Stop");
		gtk_entry_set_text(GTK_ENTRY(AVIFNameEntry),global->avifile);
	} else {
		CapAVIButt = gtk_button_new_with_label("Capture");
		videoIn->capAVI = FALSE;
		gtk_entry_set_text(GTK_ENTRY(AVIFNameEntry),DEFAULT_AVI_FNAME);
	}
	
	gtk_table_attach(GTK_TABLE(table2), CapAVIButt, 0, 1, 6, 7,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table2), AVIFNameEntry, 1, 2, 6, 7,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
	
	AviFileButt=gtk_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_table_attach(GTK_TABLE(table2), AviFileButt, 2, 3, 6, 7,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
	
	gtk_widget_show (AviFileButt);
	gtk_widget_show (CapAVIButt);
	gtk_widget_show (AVIFNameEntry);
	
	g_signal_connect (GTK_BUTTON(AviFileButt), "clicked",
		 G_CALLBACK (file_chooser), 1);
	g_signal_connect (GTK_BUTTON(CapAVIButt), "clicked",
		 G_CALLBACK (capture_avi), AVIFNameEntry);
	
	gtk_box_pack_start ( GTK_BOX (boxh), table2, FALSE, FALSE, 0);
	gtk_widget_show (table2);
	
	/* AVI Compressor */
	AVIComp = gtk_combo_box_new_text ();
	
	gtk_combo_box_append_text(GTK_COMBO_BOX(AVIComp),"MJPG - compressed");
	gtk_combo_box_append_text(GTK_COMBO_BOX(AVIComp),"YUY2 - uncomp YUV");
	gtk_combo_box_append_text(GTK_COMBO_BOX(AVIComp),"RGB - uncomp BMP");
	
	gtk_table_attach(GTK_TABLE(table2), AVIComp, 1, 2, 7, 8,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
	gtk_widget_show (AVIComp);
	
	gtk_combo_box_set_active(GTK_COMBO_BOX(AVIComp),global->AVIFormat);
	
	gtk_widget_set_sensitive (AVIComp, TRUE);
	g_signal_connect (GTK_COMBO_BOX(AVIComp), "changed",
		G_CALLBACK (AVIComp_changed), NULL);
	
	label_AVIComp = gtk_label_new("AVI Format:");
	gtk_misc_set_alignment (GTK_MISC (label_AVIComp), 1, 0.5);

	gtk_table_attach (GTK_TABLE(table2), label_AVIComp, 0, 1, 7, 8,
					GTK_FILL, 0, 0, 0);

	gtk_widget_show (label_AVIComp);

	/*----------------------- sound interface --------------------------------*/
	
	/* get sound device list and info */
	
	SndDevice = gtk_combo_box_new_text ();
		
	int     it, numDevices, defaultDisplayed;
	const   PaDeviceInfo *deviceInfo;
	PaStreamParameters inputParameters, outputParameters;
	PaError err;
	
	Pa_Initialize();
	
	numDevices = Pa_GetDeviceCount();
	if( numDevices < 0 )
	{
		printf( "SOUND DISABLE: Pa_CountDevices returned 0x%x\n", numDevices );
		err = numDevices;
		Pa_Terminate();
		global->Sound_enable=0;
	} else {
	
		for( it=0; it<numDevices; it++ )
		{
			deviceInfo = Pa_GetDeviceInfo( it );
			printf( "--------------------------------------- device #%d\n", it );
				
			/* Mark global and API specific default devices */
			defaultDisplayed = 0;
			/* Default Input will save the ALSA default device index*/
			/* since ALSA lists after OSS							*/
			if( it == Pa_GetDefaultInputDevice() )
			{
				printf( "[ Default Input" );
				defaultDisplayed = 1;
				global->Sound_DefDev=global->Sound_numInputDev;/*default index in array of input devs*/
			}
			else if( it == Pa_GetHostApiInfo( deviceInfo->hostApi )->defaultInputDevice )
			{
				const PaHostApiInfo *hostInfo = Pa_GetHostApiInfo( deviceInfo->hostApi );
				printf( "[ Default %s Input", hostInfo->name );
				defaultDisplayed = 2;
				global->Sound_DefDev=global->Sound_numInputDev;/*index in array of input devs*/
			}
		/* Output device doesn't matter for capture*/
			if( it == Pa_GetDefaultOutputDevice() )
			{
				printf( (defaultDisplayed ? "," : "[") );
				printf( " Default Output" );
				defaultDisplayed = 3;
			}
			else if( it == Pa_GetHostApiInfo( deviceInfo->hostApi )->defaultOutputDevice )
			{
				const PaHostApiInfo *hostInfo = Pa_GetHostApiInfo( deviceInfo->hostApi );
				printf( (defaultDisplayed ? "," : "[") );                
				printf( " Default %s Output", hostInfo->name );/* OSS ALSA etc*/
				defaultDisplayed = 4;
			}

			if( defaultDisplayed!=0 )
				printf( " ]\n" );

			/* print device info fields */
			printf( "Name                        = %s\n", deviceInfo->name );
			printf( "Host API                    = %s\n",  Pa_GetHostApiInfo( deviceInfo->hostApi )->name );
			
			printf( "Max inputs = %d", deviceInfo->maxInputChannels  );
			/* if it as input channels it's a capture device*/
			if (deviceInfo->maxInputChannels >0) { 
				global->Sound_IndexDev[global->Sound_numInputDev].id=it; /*saves dev id*/
				global->Sound_IndexDev[global->Sound_numInputDev].chan=deviceInfo->maxInputChannels;
				global->Sound_IndexDev[global->Sound_numInputDev].samprate=deviceInfo->defaultSampleRate;
				//Sound_IndexDev[Sound_numInputDev].Hlatency=deviceInfo->defaultHighInputLatency;
				//Sound_IndexDev[Sound_numInputDev].Llatency=deviceInfo->defaultLowInputLatency;
				global->Sound_numInputDev++;
				gtk_combo_box_append_text(GTK_COMBO_BOX(SndDevice),deviceInfo->name);		
			}
			
			printf( ", Max outputs = %d\n", deviceInfo->maxOutputChannels  );

			printf( "Default low input latency   = %8.3f\n", deviceInfo->defaultLowInputLatency  );
			printf( "Default low output latency  = %8.3f\n", deviceInfo->defaultLowOutputLatency  );
			printf( "Default high input latency  = %8.3f\n", deviceInfo->defaultHighInputLatency  );
			printf( "Default high output latency = %8.3f\n", deviceInfo->defaultHighOutputLatency  );
			printf( "Default sample rate         = %8.2f\n", deviceInfo->defaultSampleRate );
			
		}
		Pa_Terminate();
		
		printf("----------------------------------------------\n");
	
	}
	
	/*--------------------- sound controls -----------------------------------*/
	gtk_table_attach(GTK_TABLE(table2), SndDevice, 1, 3, 9, 10,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
	gtk_widget_show (SndDevice);
	if(global->Sound_UseDev==0) global->Sound_UseDev=global->Sound_DefDev;/* using default device*/
	gtk_combo_box_set_active(GTK_COMBO_BOX(SndDevice),global->Sound_UseDev);
	
	if (global->Sound_enable) gtk_widget_set_sensitive (SndDevice, TRUE);
	else  gtk_widget_set_sensitive (SndDevice, FALSE);
	g_signal_connect (GTK_COMBO_BOX(SndDevice), "changed",
		G_CALLBACK (SndDevice_changed), NULL);
	
	label_SndDevice = gtk_label_new("Imput Device:");
	gtk_misc_set_alignment (GTK_MISC (label_SndDevice), 1, 0.5);

	gtk_table_attach (GTK_TABLE(table2), label_SndDevice, 0, 1, 9, 10,
					GTK_FILL, 0, 0, 0);

	gtk_widget_show (label_SndDevice);
	
	
	//~ if (Sound_numInputDev == 0) Sound_enable=0;
	//~ printf("SOUND DISABLE: no imput devices detected\n");
	
	SndEnable=gtk_check_button_new_with_label (" Sound");
	gtk_table_attach(GTK_TABLE(table2), SndEnable, 0, 1, 8, 9,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
	
	gtk_toggle_button_set_active(GTK_CHECK_BUTTON(SndEnable),(global->Sound_enable > 0));
	gtk_widget_show (SndEnable);
	g_signal_connect (GTK_CHECK_BUTTON(SndEnable), "toggled",
		G_CALLBACK (SndEnable_changed), NULL);
	
	SndSampleRate= gtk_combo_box_new_text ();
	gtk_combo_box_append_text(GTK_COMBO_BOX(SndSampleRate),"Dev. Default");
	for( i=1; stdSampleRates[i] > 0; i++ )
	{
		char dst[8];
		sprintf(dst,"%d",stdSampleRates[i]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(SndSampleRate),dst);
	}
	if (global->Sound_SampRateInd>(i-1)) global->Sound_SampRateInd=0; /*out of range*/
	
	gtk_table_attach(GTK_TABLE(table2), SndSampleRate, 1, 3, 10, 11,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
	gtk_widget_show (SndSampleRate);
	
	global->Sound_SampRate=stdSampleRates[global->Sound_SampRateInd];
	gtk_combo_box_set_active(GTK_COMBO_BOX(SndSampleRate),global->Sound_SampRateInd); /*device default*/
	
	
	if (global->Sound_enable) gtk_widget_set_sensitive (SndSampleRate, TRUE);
	else  gtk_widget_set_sensitive (SndSampleRate, FALSE);
	g_signal_connect (GTK_COMBO_BOX(SndSampleRate), "changed",
		G_CALLBACK (SndSampleRate_changed), NULL);
	
	label_SndSampRate = gtk_label_new("Sample Rate:");
	gtk_misc_set_alignment (GTK_MISC (label_SndSampRate), 1, 0.5);

	gtk_table_attach (GTK_TABLE(table2), label_SndSampRate, 0, 1, 10, 11,
					GTK_FILL, 0, 0, 0);

	gtk_widget_show (label_SndSampRate);
	
	SndNumChan= gtk_combo_box_new_text ();
	gtk_combo_box_append_text(GTK_COMBO_BOX(SndNumChan),"Dev. Default");
	gtk_combo_box_append_text(GTK_COMBO_BOX(SndNumChan),"1 - mono");
	gtk_combo_box_append_text(GTK_COMBO_BOX(SndNumChan),"2 - stereo");
	
	gtk_table_attach(GTK_TABLE(table2), SndNumChan, 1, 3, 11, 12,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
	gtk_widget_show (SndNumChan);
	switch (global->Sound_NumChanInd) {
	   case 0:/*device default*/
			gtk_combo_box_set_active(GTK_COMBO_BOX(SndNumChan),0);
		break;
	   case 1:/*mono*/	
			gtk_combo_box_set_active(GTK_COMBO_BOX(SndNumChan),1);
			global->Sound_NumChan=1;
		break;
		case 2:/*stereo*/
			gtk_combo_box_set_active(GTK_COMBO_BOX(SndNumChan),2);
			global->Sound_NumChan=2;
	   default:
		/*set Default to NUM_CHANNELS*/
			global->Sound_NumChan=NUM_CHANNELS;	
	}
	if (global->Sound_enable) gtk_widget_set_sensitive (SndNumChan, TRUE);
	else gtk_widget_set_sensitive (SndNumChan, FALSE);
	g_signal_connect (GTK_COMBO_BOX(SndNumChan), "changed",
		G_CALLBACK (SndNumChan_changed), NULL);
	
	label_SndNumChan = gtk_label_new("Chanels:");
	gtk_misc_set_alignment (GTK_MISC (label_SndNumChan), 1, 0.5);

	gtk_table_attach (GTK_TABLE(table2), label_SndNumChan, 0, 1, 11, 12,
					GTK_FILL, 0, 0, 0);

	gtk_widget_show (label_SndNumChan);
	printf("SampleRate:%d Channels:%d\n",global->Sound_SampRate,global->Sound_NumChan);
	
	/*----- Filter controls ----*/
	
	label_videoFilters = gtk_label_new("---- Video Filters ----");
	gtk_misc_set_alignment (GTK_MISC (label_videoFilters), 0.5, 0.5);

	gtk_table_attach (GTK_TABLE(table2), label_videoFilters, 0, 3, 12, 13,
					GTK_FILL, 0, 0, 0);

	gtk_widget_show (label_videoFilters);
	
	table3 = gtk_table_new(1,4,FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table3), 10);
	gtk_table_set_col_spacings (GTK_TABLE (table3), 10);
	gtk_container_set_border_width (GTK_CONTAINER (table3), 10);
	gtk_widget_set_size_request (table3, -1, -1);
	
	
	
	/* Mirror */
	FiltMirrorEnable=gtk_check_button_new_with_label (" Mirror");
	gtk_table_attach(GTK_TABLE(table3), FiltMirrorEnable, 0, 1, 0, 1,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
	
	gtk_toggle_button_set_active(GTK_CHECK_BUTTON(FiltMirrorEnable),(global->Frame_Flags & YUV_MIRROR)>0);
	gtk_widget_show (FiltMirrorEnable);
	g_signal_connect (GTK_CHECK_BUTTON(FiltMirrorEnable), "toggled",
		G_CALLBACK (FiltMirrorEnable_changed), NULL);
	/*Upturn*/
	FiltUpturnEnable=gtk_check_button_new_with_label (" Upturn");
	gtk_table_attach(GTK_TABLE(table3), FiltUpturnEnable, 1, 2, 0, 1,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
	
	gtk_toggle_button_set_active(GTK_CHECK_BUTTON(FiltUpturnEnable),(global->Frame_Flags & YUV_UPTURN)>0);
	gtk_widget_show (FiltUpturnEnable);
	g_signal_connect (GTK_CHECK_BUTTON(FiltUpturnEnable), "toggled",
		G_CALLBACK (FiltUpturnEnable_changed), NULL);
	/*Negate*/
	FiltNegateEnable=gtk_check_button_new_with_label (" Negate");
	gtk_table_attach(GTK_TABLE(table3), FiltNegateEnable, 2, 3, 0, 1,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
	
	gtk_toggle_button_set_active(GTK_CHECK_BUTTON(FiltNegateEnable),(global->Frame_Flags & YUV_NEGATE)>0);
	gtk_widget_show (FiltNegateEnable);
	g_signal_connect (GTK_CHECK_BUTTON(FiltNegateEnable), "toggled",
		G_CALLBACK (FiltNegateEnable_changed), NULL);
	/*Mono*/
	FiltMonoEnable=gtk_check_button_new_with_label (" Mono");
	gtk_table_attach(GTK_TABLE(table3), FiltMonoEnable, 3, 4, 0, 1,
					GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0, 0);
	
	gtk_toggle_button_set_active(GTK_CHECK_BUTTON(FiltMonoEnable),(global->Frame_Flags & YUV_MONOCR)>0);
	gtk_widget_show (FiltMonoEnable);
	g_signal_connect (GTK_CHECK_BUTTON(FiltMonoEnable), "toggled",
		G_CALLBACK (FiltMonoEnable_changed), NULL);
	
	gtk_table_attach (GTK_TABLE(table2), table3, 0, 3, 13, 14,
					GTK_FILL, 0, 0, 0);

	gtk_widget_show (table3);
	
/*------------------------------ SDL init video ---------------------*/
	pscreen =
	SDL_SetVideoMode(videoIn->width, videoIn->height, global->bpp,
			 SDL_VIDEO_Flags);
	overlay =
	SDL_CreateYUVOverlay(videoIn->width, videoIn->height,
				 SDL_YUY2_OVERLAY, pscreen);
	
	p = (unsigned char *) overlay->pixels[0];
	
	drect.x = 0;
	drect.y = 0;
	drect.w = pscreen->w;
	drect.h = pscreen->h;

	SDL_WM_SetCaption("GUVCVideo", NULL);
	
	/* main container */
	gtk_container_add (GTK_CONTAINER (mainwin), boxh);
	gtk_widget_show (boxh);
	
	gtk_widget_show (mainwin);
	
	/*--------------------- avi capture from start ---------------------------*/
	if(global->avifile) {
		AviOut = AVI_open_output_file(global->avifile);
		/*4CC compression "YUY2" (YUV) or "DIB " (RGB24)  or  "MJPG"*/
		char *compression="MJPG";

		switch (global->AVIFormat) {
			case 0:
				compression="MJPG";
				break;
			case 1:
				compression="YUY2";//~ /* Capture a single raw frame */
	//~ if (vd->rawFrameCapture && vd->buf.bytesused > 0) {
		//~ FILE *frame = NULL;
		//~ char filename[13];
		//~ int ret;

		//~ /* Disable frame capturing unless we're in frame stream mode */
		//~ if(vd->rawFrameCapture == 1)
			//~ vd->rawFrameCapture = 0;

		//~ /* Create a file name and open the file */
		//~ sprintf(filename, "frame%03u.raw", vd->fileCounter++ % 1000);
		//~ frame = fopen(filename, "wb");
		//~ if(frame == NULL) {
			//~ perror("Unable to open file for raw frame capturing");
			//~ goto end_capture;
		//~ }
		
		//~ /* Wri
				break;
			case 2:
				compression="DIB ";
				break;
			default:
				compression="MJPG";
		}
	   AVI_set_video(AviOut, videoIn->width, videoIn->height, videoIn->fps,compression);		
	   /* audio will be set in aviClose - if enabled*/
	   sprintf(videoIn->AVIFName,"%s/%s",global->aviPath,global->aviName);		
	   videoIn->capAVI = TRUE;
	   /*disabling sound and avi compression controls*/
	   gtk_widget_set_sensitive (AVIComp, FALSE);
	   gtk_widget_set_sensitive (SndEnable,FALSE); 
	   gtk_widget_set_sensitive (SndSampleRate,FALSE);
	   gtk_widget_set_sensitive (SndDevice,FALSE);
	   gtk_widget_set_sensitive (SndNumChan,FALSE);
	   /* Creating the sound capture loop thread if Sound Enable*/ 
	   if(global->Sound_enable > 0) { 
		  /* Initialize and set snd thread detached attribute */
		  pthread_attr_init(&sndattr);
		  pthread_attr_setdetachstate(&sndattr, PTHREAD_CREATE_JOINABLE);
		   
		  int rsnd = pthread_create(&sndthread, &sndattr, sound_capture, NULL); 
		  if (rsnd)
		  {
			 printf("ERROR; return code from snd pthread_create() is %d\n", rsnd);
		  }
		}
		if (global->Capture_time) {
			/*sets the timer function*/
			g_timeout_add(global->Capture_time*1000,timer_callback,NULL);
		}
		
		global->AVIstarttime = ms_time();
	}
	
	/*------------------ Creating the main loop (video) thread ---------------*/
	int rc = pthread_create(&mythread, &attr, main_loop, NULL); 
	if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(2);
		}
	
	/* The last thing to get called */
	gtk_main();

	return 0;
}

/*-------------------------- clean up and shut down --------------------------*/
gint 
shutd (gint restart) 
{
	int exec_status=0;
	int i;
	int tstatus;
	
	printf("Shuting Down Thread\n");
	videoIn->signalquit=0;
	printf("waiting for thread to finish\n");
	/*shuting down while in avi capture mode*/
	/*must close avi						*/
	if(videoIn->capAVI) {
		printf("stoping AVI capture\n");
		global->AVIstoptime = ms_time();
		//printf("AVI stop time:%d\n",AVIstoptime);	
		videoIn->capAVI = FALSE;
		aviClose();
	}
	
	/* Free attribute and wait for the main loop (video) thread */
	pthread_attr_destroy(&attr);
	int rc = pthread_join(mythread, (void **)&tstatus);
	if (rc)
	  {
		 printf("ERROR; return code from pthread_join() is %d\n", rc);
		 exit(-1);
	  }
	printf("Completed join with thread status= %d\n", tstatus);
	
	
	gtk_window_get_size(GTK_WINDOW(mainwin),&(global->winwidth),&(global->winheight));//mainwin or widget
	
	
	close_v4l2(videoIn);
	close(videoIn->fd);
	printf("closed strutures\n");
	free(videoIn);
	SDL_Quit();
	printf("SDL Quit\n");
	printf("cleaned allocations - 50%%\n");
	gtk_main_quit();
	
	printf("GTK quit\n");
	writeConf(global->confPath);
	input_free_controls (s->control, s->num_controls);
	printf("free controls - vidState\n");
	free(s);
	closeGlobals(global);
	if (jpeg_struct != NULL) free(jpeg_struct);
	
	if (restart==1) { /* replace running process with new one */
		 printf("restarting guvcview with command: %s\n",EXEC_CALL);
		 exec_status = execlp(EXEC_CALL,EXEC_CALL,NULL);/*No parameters passed*/
	}
	
	free(EXEC_CALL);
	printf("cleanig allocations - 100%%\n");
	return exec_status;
}

