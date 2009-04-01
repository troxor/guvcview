/*******************************************************************************#
#           guvcview              http://guvcview.berlios.de                    #
#                                                                               #
#           Paulo Assis <pj.assis@gmail.com>                                    #
#                                                                               #
# This program is free software; you can redistribute it and/or modify          #
# it under the terms of the GNU General Public License as published by          #
# the Free Software Foundation; either version 2 of the License, or             #
# (at your option) any later version.                                           #
#                                                                               #
# This program is distributed in the hope that it will be useful,               #
# but WITHOUT ANY WARRANTY; without even the implied warranty of                #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 #
# GNU General Public License for more details.                                  #
#                                                                               #
# You should have received a copy of the GNU General Public License             #
# along with this program; if not, write to the Free Software                   #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA     #
#                                                                               #
********************************************************************************/

#ifndef LAVC_COMMON_H
#define LAVC_COMMON_H

#include "../config.h"
#include "defs.h"
#ifdef HAS_LIBAVCODEC_AVCODEC_H
  #include <libavcodec/avcodec.h>
#else
  #ifdef HAS_FFMPEG_AVCODEC_H
    #include <ffmpeg/avcodec.h>
  #else
    #ifdef HAS_FFMPEG_LIBAVCODEC_AVCODEC_H
      #include <ffmpeg/libavcodec/avcodec.h>
    #else
      #include <avcodec.h>
    #endif
  #endif
#endif



struct lavcData
{
	AVCodec *codec;
	AVCodecContext *codec_context;
	AVFrame *picture;

	BYTE* tmpbuf;
	int outbuf_size;
	BYTE* outbuf;

};

int encode_lavc_frame (BYTE *picture_buf, struct lavcData* data);

// arg = pointer to lavcData struct =>
// *arg = struct lavcData**
void clean_lavc (void *arg);

#endif
