/*******************************************************************************#
#           guvcview              http://guvcview.sourceforge.net               #
#                                                                               #
#           Paulo Assis <pj.assis@gmail.com>                                    #
#           Nobuhiro Iwamatsu <iwamatsu@nigauri.org>                            #
#                             Add UYVY color support(Macbook iSight)            #
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

/* support for internationalization - i18n */
#include <glib/gi18n.h>
#include <glib.h>
#include <glib/gprintf.h>

#include <libusb.h>

#include "uvc_h264.h"
#include "v4l2_dyna_ctrls.h"
#include "v4l2_formats.h"
#include "string_utils.h"
#include "callbacks.h"
#include "guvcview.h"


#define USB_VIDEO_CONTROL		    0x01
#define USB_VIDEO_CONTROL_INTERFACE	0x24
#define USB_VIDEO_CONTROL_XU_TYPE	0x06

static void print_probe_commit_data(uvcx_video_config_probe_commit_t *data)
{
	printf("uvcx_video_config_probe_commit:\n");
	printf("\tFrameInterval: %i\n", data->dwFrameInterval);
	printf("\tBitRate: %i\n", data->dwBitRate);
	printf("\tHints: 0x%X\n", data->bmHints);
	printf("\tConfigurationIndex: %i\n", data->wConfigurationIndex);
	printf("\tWidth: %i\n", data->wWidth);
	printf("\tHeight: %i\n", data->wHeight);
	printf("\tSliceUnits: %i\n", data->wSliceUnits);
	printf("\tSliceMode: %i\n", data->wSliceMode);
	printf("\tProfile: %i\n", data->wProfile);
	printf("\tIFramePeriod: %i\n", data->wIFramePeriod);
	printf("\tEstimatedVideoDelay: %i\n",data->wEstimatedVideoDelay);
	printf("\tEstimatedMaxConfigDelay: %i\n",data->wEstimatedMaxConfigDelay);
	printf("\tUsageType: %i\n",data->bUsageType);
	printf("\tRateControlMode: %i\n",data->bRateControlMode);
	printf("\tTemporalScaleMode: %i\n",data->bTemporalScaleMode);
	printf("\tSpatialScaleMode: %i\n",data->bSpatialScaleMode);
	printf("\tSNRScaleMode: %i\n",data->bSNRScaleMode);
	printf("\tStreamMuxOption: %i\n",data->bStreamMuxOption);
	printf("\tStreamFormat: %i\n",data->bStreamFormat);
	printf("\tEntropyCABAC: %i\n",data->bEntropyCABAC);
	printf("\tTimestamp: %i\n",data->bTimestamp);
	printf("\tNumOfReorderFrames: %i\n",data->bNumOfReorderFrames);
	printf("\tPreviewFlipped: %i\n",data->bPreviewFlipped);
	printf("\tView: %i\n",data->bView);
	printf("\tReserved1: %i\n",data->bReserved1);
	printf("\tReserved2: %i\n",data->bReserved2);
	printf("\tStreamID: %i\n",data->bStreamID);
	printf("\tSpatialLayerRatio: %i\n",data->bSpatialLayerRatio);
	printf("\tLeakyBucketSize: %i\n",data->wLeakyBucketSize);
}

static void update_h264_controls(
	uvcx_video_config_probe_commit_t *config_probe_req,
	struct ALL_DATA* data)
{
	//struct GLOBAL *global = data->global;
	struct vdIn *videoIn  = data->videoIn;
	struct uvc_h264_gtkcontrols  *h264_controls = data->h264_controls;

	//dwFrameInterval
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(h264_controls->FrameInterval), config_probe_req->dwFrameInterval);
	//dwBitRate
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(h264_controls->BitRate), config_probe_req->dwBitRate);
	//bmHints
	uint16_t hints = config_probe_req->bmHints;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_res), ((hints & 0x0001) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_prof), ((hints & 0x0002) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_ratecontrol), ((hints & 0x0004) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_usage), ((hints & 0x0008) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_slicemode), ((hints & 0x0010) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_sliceunit), ((hints & 0x0020) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_view), ((hints & 0x0040) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_temporal), ((hints & 0x0080) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_snr), ((hints & 0x0100) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_spatial), ((hints & 0x0200) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_spatiallayer), ((hints & 0x0400) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_frameinterval), ((hints & 0x0800) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_leakybucket), ((hints & 0x1000) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_bitrate), ((hints & 0x2000) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_cabac), ((hints & 0x4000) != 0));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_iframe), ((hints & 0x8000) != 0));
	//wWidth x wHeight
	int h264_format_ind = get_FormatIndex(videoIn->listFormats,V4L2_PIX_FMT_H264);
	VidFormats *listVidFormats = &videoIn->listFormats->listVidFormats[h264_format_ind];
	int i=0;
	int defres = 0;
	for(i=0; i < listVidFormats->numb_res ; i++)
	{
		if ((config_probe_req->wWidth == listVidFormats->listVidCap[i].width) &&
			(config_probe_req->wHeight == listVidFormats->listVidCap[i].height))
		{
			defres = i;
			break;
		}
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(h264_controls->Resolution), defres);//set selected resolution index
	//wSliceMode
	gtk_combo_box_set_active(GTK_COMBO_BOX(h264_controls->SliceMode), config_probe_req->wSliceMode);
	//wSliceUnits
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(h264_controls->SliceUnits), config_probe_req->wSliceUnits);
	//wProfile
	uint16_t profile = config_probe_req->wProfile & 0xFF00;
	int prof_index = 0;
	switch(profile)
	{
		case 0x4200:
			prof_index = 0;
			break;
		case 0x4D00:
			prof_index = 1;
			break;
		case 0x6400:
			prof_index = 2;
			break;
		case 0x5300:
			prof_index = 3;
			break;
		case 0x5600:
			prof_index = 4;
			break;
		case 0x7600:
			prof_index = 5;
			break;
		case 0x8000:
			prof_index = 6;
			break;
		default:
			fprintf(stderr, "H264 probe: unknown profile mode 0x%X\n", profile);
			break;

	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(h264_controls->Profile), prof_index);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(h264_controls->Profile_flags), config_probe_req->wProfile & 0x00FF);
	//wIFramePeriod
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(h264_controls->IFramePeriod), config_probe_req->wIFramePeriod);
	//wEstimatedVideoDelay
	 gtk_spin_button_set_value(GTK_SPIN_BUTTON(h264_controls->EstimatedVideoDelay), config_probe_req->wEstimatedVideoDelay);
	//wEstimatedMaxConfigDelay
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(h264_controls->EstimatedMaxConfigDelay), config_probe_req->wEstimatedMaxConfigDelay);
	//bUsageType
	int usage_type_ind = (config_probe_req->bUsageType & 0x0F) - 1;
	if(usage_type_ind < 0)
		usage_type_ind = 0;
	gtk_combo_box_set_active(GTK_COMBO_BOX(h264_controls->UsageType), usage_type_ind);
	//bRateControlMode
	gtk_combo_box_set_active(GTK_COMBO_BOX(h264_controls->RateControlMode), (config_probe_req->bRateControlMode & 0x03) - 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(h264_controls->RateControlMode_cbr_flag), config_probe_req->bRateControlMode & 0x0000001C);
	//bTemporalScaleMode
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(h264_controls->TemporalScaleMode), config_probe_req->bTemporalScaleMode);
	//bSpatialScaleMode
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(h264_controls->SpatialScaleMode), config_probe_req->bSpatialScaleMode);
	//bSNRScaleMode
	uint8_t snrscalemode = config_probe_req->bSNRScaleMode & 0x0F;
	int snrscalemode_index = 0;
	switch(snrscalemode)
	{
		case 0:
			snrscalemode_index = 0;
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			snrscalemode_index = snrscalemode - 1;
			break;
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(h264_controls->SNRScaleMode), snrscalemode_index);
	//bStreamMuxOption
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->StreamMuxOption),((config_probe_req->bStreamMuxOption & 0x01) != 0));
	uint8_t streammux = config_probe_req->bStreamMuxOption & 0x0E;
	int streammux_index = 0;
	switch(streammux)
	{
		case 2:
			streammux_index = 0;
			break;
		case 4:
			streammux_index = 1;
			break;
		case 8:
			streammux_index = 2;
			break;
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX(h264_controls->StreamMuxOption_aux), streammux_index);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->StreamMuxOption_mjpgcontainer),((config_probe_req->bStreamMuxOption & 0x40) != 0));
	//bStreamFormat
	gtk_combo_box_set_active (GTK_COMBO_BOX(h264_controls->StreamFormat), config_probe_req->bStreamMuxOption & 0x01);
	//bEntropyCABAC
	gtk_combo_box_set_active (GTK_COMBO_BOX(h264_controls->EntropyCABAC), config_probe_req->bEntropyCABAC & 0x01);
	//bTimestamp
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->Timestamp), (config_probe_req->bTimestamp & 0x01) != 0);
	//bNumOfReorderFrames
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(h264_controls->NumOfReorderFrames), config_probe_req->bNumOfReorderFrames);
	//bPreviewFlipped
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->PreviewFlipped), (config_probe_req->bPreviewFlipped & 0x01) != 0);
	//bView
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(h264_controls->View), config_probe_req->bView);
	//bStreamID
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(h264_controls->StreamID), config_probe_req->bStreamID);
	//bSpatialLayerRatio
	int SLRatio = config_probe_req->bSpatialLayerRatio & 0x000000FF;
	gdouble val = (gdouble) ((SLRatio & 0x000000F0)>>4) + (gdouble)((SLRatio & 0x0000000F)/16);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(h264_controls->SpatialLayerRatio), val);
	//wLeakyBucketSize
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(h264_controls->LeakyBucketSize), config_probe_req->wLeakyBucketSize);
}

static void fill_video_config_probe(
	uvcx_video_config_probe_commit_t *config_probe_req,
	struct ALL_DATA* data)
{
	//struct GLOBAL *global = data->global;
	struct vdIn *videoIn  = data->videoIn;
	struct uvc_h264_gtkcontrols  *h264_controls = data->h264_controls;

	//dwFrameInterval
	config_probe_req->dwFrameInterval = (uint32_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(h264_controls->FrameInterval));
	//dwBitRate
	config_probe_req->dwBitRate = (uint32_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(h264_controls->BitRate));
	//bmHints
	uint16_t hints = 0x0000;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_res)) ?  0x0001 :  0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_prof)) ? 0x0002 : 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_ratecontrol)) ? 0x0004: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_usage)) ? 0x0008: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_slicemode)) ? 0x0010: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_sliceunit)) ? 0x0020: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_view)) ? 0x0040: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_temporal)) ? 0x0080: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_snr)) ? 0x0100: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_spatial)) ? 0x0200: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_spatiallayer)) ? 0x0400: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_frameinterval)) ? 0x0800: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_leakybucket)) ? 0x1000: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_bitrate)) ? 0x2000: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_cabac)) ? 0x4000: 0;
	hints |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(h264_controls->Hints_iframe)) ? 0x8000: 0;
	config_probe_req->bmHints = hints;
	//wWidth x wHeight
	int h264_format_ind = get_FormatIndex(videoIn->listFormats,V4L2_PIX_FMT_H264);
	int def_res = gtk_combo_box_get_active(GTK_COMBO_BOX(h264_controls->Resolution));
	config_probe_req->wWidth = (uint16_t) videoIn->listFormats->listVidFormats[h264_format_ind].listVidCap[def_res].width;
	config_probe_req->wHeight = (uint16_t)  videoIn->listFormats->listVidFormats[h264_format_ind].listVidCap[def_res].height;
	//wSliceMode
	config_probe_req->wSliceMode = (uint16_t) gtk_combo_box_get_active(GTK_COMBO_BOX(h264_controls->SliceMode));
	//wSliceUnits
	config_probe_req->wSliceUnits = (uint16_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(h264_controls->SliceUnits));
	//wProfile
	int profile_index = gtk_combo_box_get_active(GTK_COMBO_BOX(h264_controls->Profile));
	uint16_t profile = 0x4200;
	switch(profile_index)
	{
		case 0:
			profile = 0x4200;
			break;
		case 1:
			profile = 0x4D00;
			break;
		case 2:
			profile = 0x6400;
			break;
		case 3:
			profile = 0x5300;
			break;
		case 4:
			profile = 0x5600;
			break;
		case 5:
			profile = 0x7600;
			break;
		case 6:
			profile = 0x8000;
			break;
		default:
			fprintf(stderr, "H264 probe: unknown profile\n");
			break;
	}

	profile |= (uint16_t) (gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(h264_controls->Profile_flags)) & 0x000000FF);
	config_probe_req->wProfile = profile;
	//wIFramePeriod
	config_probe_req->wIFramePeriod = (uint16_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(h264_controls->IFramePeriod));
	//wEstimatedVideoDelay
	config_probe_req->wEstimatedVideoDelay = (uint16_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(h264_controls->EstimatedVideoDelay));
	//wEstimatedMaxConfigDelay
	config_probe_req->wEstimatedMaxConfigDelay = (uint16_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(h264_controls->EstimatedMaxConfigDelay));
	//bUsageType
	config_probe_req->bUsageType = (uint8_t) (gtk_combo_box_get_active(GTK_COMBO_BOX(h264_controls->UsageType)) + 1);
	//bRateControlMode
	config_probe_req->bRateControlMode = (uint8_t) (gtk_combo_box_get_active(GTK_COMBO_BOX(h264_controls->RateControlMode)) + 1);
	config_probe_req->bRateControlMode |= (uint8_t) (gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(h264_controls->RateControlMode_cbr_flag)) & 0x0000001C);
	//bTemporalScaleMode
	config_probe_req->bTemporalScaleMode = (uint8_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(h264_controls->TemporalScaleMode));
	//bSpatialScaleMode
	config_probe_req->bSpatialScaleMode = (uint8_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(h264_controls->SpatialScaleMode));
	//bSNRScaleMode
	int snrscalemode_index = gtk_combo_box_get_active(GTK_COMBO_BOX(h264_controls->SNRScaleMode));
	config_probe_req->bSNRScaleMode = 0;
	switch(snrscalemode_index)
	{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			config_probe_req->bSNRScaleMode = snrscalemode_index + 1;
			break;
	}
	//bStreamMuxOption
	uint8_t streammux = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(h264_controls->StreamMuxOption)) ? 0x01: 0;
	
	int streammux_index = gtk_combo_box_get_active(GTK_COMBO_BOX(h264_controls->StreamMuxOption_aux));
	switch(streammux_index)
	{
		case 0:
			streammux |= 0x02;
			break;
		case 1:
			streammux |= 0x04;
			break;
		case 2:
			streammux |= 0x08;
			break;
	}
	streammux |= gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(h264_controls->StreamMuxOption_mjpgcontainer)) ? 0x40 : 0x00;
	
	config_probe_req->bStreamMuxOption = streammux;
	//bStreamFormat
	config_probe_req->bStreamMuxOption = (uint8_t) gtk_combo_box_get_active(GTK_COMBO_BOX(h264_controls->StreamFormat)) & 0x01;
	//bEntropyCABAC
	config_probe_req->bEntropyCABAC = (uint8_t) gtk_combo_box_get_active(GTK_COMBO_BOX(h264_controls->EntropyCABAC)) & 0x01;
	//bTimestamp
	config_probe_req->bTimestamp = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(h264_controls->Timestamp)) ? 0x01 : 0x00;
	//bNumOfReorderFrames
	config_probe_req->bNumOfReorderFrames = (uint8_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(h264_controls->NumOfReorderFrames));
	//bPreviewFlipped
	config_probe_req->bPreviewFlipped = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(h264_controls->PreviewFlipped)) ? 0x01 : 0x00;
	//bView
	config_probe_req->bView = (uint8_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(h264_controls->View));
	//bStreamID
	config_probe_req->bStreamID = (uint8_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(h264_controls->StreamID));
	//bSpatialLayerRatio
	gdouble spatialratio = gtk_spin_button_get_value(GTK_SPIN_BUTTON(h264_controls->SpatialLayerRatio));
	uint8_t high_nibble = floor(spatialratio);
	uint8_t lower_nibble = floor((spatialratio - high_nibble) * 16);
	config_probe_req->bSpatialLayerRatio = (high_nibble << 4) + lower_nibble;
	//wLeakyBucketSize
	config_probe_req->wLeakyBucketSize = (uint16_t) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(h264_controls->LeakyBucketSize));
}

void h264_probe_button_clicked(GtkButton * Button, struct ALL_DATA* data)
{
	struct GLOBAL *global = data->global;
	struct vdIn *videoIn  = data->videoIn;

	uvcx_video_config_probe_commit_t config_probe_req;
	uvcx_video_probe(videoIn->fd, global->uvc_h264_unit, UVC_GET_CUR, &config_probe_req);

	//get the control data and fill req (need fps and resolution)
	fill_video_config_probe(&config_probe_req, data);
	printf("Probing:\n");
	print_probe_commit_data(&config_probe_req);

	//Probe the request
	uvcx_video_probe(videoIn->fd, global->uvc_h264_unit, UVC_SET_CUR, &config_probe_req);
	uvcx_video_probe(videoIn->fd, global->uvc_h264_unit, UVC_GET_CUR, &config_probe_req);
	printf("Probe Request:\n");
	print_probe_commit_data(&config_probe_req);

	//update the control widgets
	update_h264_controls(&config_probe_req, data);
}

void h264_commit_button_clicked(GtkButton * Button, struct ALL_DATA* data)
{
	struct GLOBAL *global = data->global;
	struct vdIn *videoIn  = data->videoIn;

	uvcx_video_config_probe_commit_t config_probe_req;
	uvcx_video_probe(videoIn->fd, global->uvc_h264_unit, UVC_GET_CUR, &config_probe_req);

	//get the control data and fill req (need fps and resolution)
	fill_video_config_probe(&config_probe_req, data);
	printf("Commiting:\n");
	print_probe_commit_data(&config_probe_req);
	
	//Commit the request
	uvcx_video_commit(videoIn->fd, global->uvc_h264_unit, &config_probe_req);
	
	uvcx_video_probe(videoIn->fd, global->uvc_h264_unit, UVC_GET_CUR, &config_probe_req);
	printf("Probe Current:\n");
	print_probe_commit_data(&config_probe_req);
}

/*
 * creates the control widgets for uvc H264
 */
void add_uvc_h264_controls_tab (struct ALL_DATA* all_data)
{
	struct GLOBAL *global = all_data->global;
	struct vdIn *videoIn  = all_data->videoIn;
	struct uvc_h264_gtkcontrols *h264_controls = g_new0(struct uvc_h264_gtkcontrols, 1);
	all_data->h264_controls =  h264_controls;
	struct GWIDGET *gwidget = all_data->gwidget;

	//get current values
	uvcx_video_config_probe_commit_t config_probe_cur;
	uvcx_video_probe(videoIn->fd, global->uvc_h264_unit, UVC_GET_CUR, &config_probe_cur);
	//print_probe_commit_data(&config_probe_cur);

	//get Max values
	uvcx_video_config_probe_commit_t config_probe_max;
	uvcx_video_probe(videoIn->fd, global->uvc_h264_unit, UVC_GET_MAX, &config_probe_max);

	//get Min values
	uvcx_video_config_probe_commit_t config_probe_min;
	uvcx_video_probe(videoIn->fd, global->uvc_h264_unit, UVC_GET_MIN, &config_probe_min);

	int line = 0;
	//add the controls and associate the callbacks
	GtkWidget *table;
	GtkWidget *scroll;
	GtkWidget *Tab;
	GtkWidget *TabLabel;
	GtkWidget *TabIcon;

	//TABLE
	table = gtk_grid_new();
	gtk_grid_set_column_homogeneous (GTK_GRID(table), FALSE);
	gtk_widget_set_hexpand (table, TRUE);
	gtk_widget_set_halign (table, GTK_ALIGN_FILL);

	gtk_grid_set_row_spacing (GTK_GRID(table), 4);
	gtk_grid_set_column_spacing (GTK_GRID (table), 4);
	gtk_container_set_border_width (GTK_CONTAINER (table), 2);
	gtk_widget_show (table);
	//SCROLL
	scroll = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(scroll), GTK_CORNER_TOP_LEFT);

    //ADD TABLE TO SCROLL

    //viewport is only needed for gtk < 3.8
    //for 3.8 and above s->table can be directly added to scroll1
    GtkWidget* viewport = gtk_viewport_new(NULL,NULL);
    gtk_container_add(GTK_CONTAINER(viewport), table);
    gtk_widget_show(viewport);

    gtk_container_add(GTK_CONTAINER(scroll), viewport);
	gtk_widget_show(scroll);

	//new hbox for tab label and icon
	Tab = gtk_grid_new();
	TabLabel = gtk_label_new(_("H264 Controls"));
	gtk_widget_show (TabLabel);

	gchar* TabIconPath = g_strconcat (PACKAGE_DATA_DIR,"/pixmaps/guvcview/image_controls.png",NULL);
	//don't test for file - use default empty image if load fails
	//get icon image
	TabIcon = gtk_image_new_from_file(TabIconPath);
	g_free(TabIconPath);
	gtk_widget_show (TabIcon);
	gtk_grid_attach (GTK_GRID(Tab), TabIcon, 0, 0, 1, 1);
	gtk_grid_attach (GTK_GRID(Tab), TabLabel, 1, 0, 1, 1);

	gtk_widget_show (Tab);
	gtk_notebook_append_page(GTK_NOTEBOOK(gwidget->boxh),scroll,Tab);


	//probe_commitcontrols

	//dwFrameInterval (get it from current fps)
	GtkWidget* label_FrameInterval = gtk_label_new(_("Frame Interval (100ns units):"));
	gtk_misc_set_alignment (GTK_MISC (label_FrameInterval), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_FrameInterval, 0, line, 1, 1);
	gtk_widget_show (label_FrameInterval);

	//uint32_t frame_interval = (global->fps_num * 1000000000LL / global->fps)/100;

	GtkAdjustment *adjustment0 = gtk_adjustment_new (
                                	config_probe_cur.dwFrameInterval,
                                	config_probe_min.dwFrameInterval,
                                    config_probe_max.dwFrameInterval,
                                    1,
                                    10,
                                    0);

    h264_controls->FrameInterval = gtk_spin_button_new(adjustment0, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(h264_controls->FrameInterval), TRUE);

    gtk_grid_attach (GTK_GRID(table), h264_controls->FrameInterval, 1, line, 1 ,1);
    gtk_widget_show (h264_controls->FrameInterval);

	//dwBitRate
	line++;
	GtkWidget* label_BitRate = gtk_label_new(_("Bit Rate:"));
	gtk_misc_set_alignment (GTK_MISC (label_BitRate), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_BitRate, 0, line, 1, 1);
	gtk_widget_show (label_BitRate);

	GtkAdjustment *adjustment1 = gtk_adjustment_new (
                                	config_probe_cur.dwBitRate,
                                	config_probe_min.dwBitRate,
                                    config_probe_max.dwBitRate,
                                    1,
                                    10,
                                    0);

    h264_controls->BitRate = gtk_spin_button_new(adjustment1, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(h264_controls->BitRate), TRUE);

    gtk_grid_attach (GTK_GRID(table), h264_controls->BitRate, 1, line, 1 ,1);
    gtk_widget_show (h264_controls->BitRate);

	//bmHints
	line++;
	GtkWidget* hints_table = gtk_grid_new();

	GtkWidget* label_Hints = gtk_label_new(_("Hints:"));
	gtk_misc_set_alignment (GTK_MISC (label_Hints), 1, 0.5);
	gtk_grid_attach (GTK_GRID(hints_table), label_Hints, 0, 1, 2, 1);
	gtk_widget_show (label_Hints);

	h264_controls->Hints_res = gtk_check_button_new_with_label (_("Resolution"));
	gtk_grid_attach (GTK_GRID(hints_table), h264_controls->Hints_res, 0, 2, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->Hints_res),((config_probe_cur.bmHints & 0x0001) > 0));
	gtk_widget_show (h264_controls->Hints_res);
	h264_controls->Hints_prof = gtk_check_button_new_with_label (_("Profile"));
	gtk_grid_attach (GTK_GRID(hints_table),h264_controls->Hints_prof, 1, 2, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->Hints_prof),((config_probe_cur.bmHints & 0x0002) > 0));
	gtk_widget_show (h264_controls->Hints_prof);
	h264_controls->Hints_ratecontrol = gtk_check_button_new_with_label (_("Rate Control"));
	gtk_grid_attach (GTK_GRID(hints_table),h264_controls->Hints_ratecontrol, 2, 2, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->Hints_ratecontrol),((config_probe_cur.bmHints & 0x0004) > 0));
	gtk_widget_show (h264_controls->Hints_ratecontrol);
	h264_controls->Hints_usage = gtk_check_button_new_with_label (_("Usage Type"));
	gtk_grid_attach (GTK_GRID(hints_table),h264_controls->Hints_usage, 3, 2, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->Hints_usage),((config_probe_cur.bmHints & 0x0008) > 0));
	gtk_widget_show (h264_controls->Hints_usage);
	h264_controls->Hints_slicemode = gtk_check_button_new_with_label (_("Slice Mode"));
	gtk_grid_attach (GTK_GRID(hints_table),h264_controls->Hints_slicemode, 0, 3, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->Hints_slicemode),((config_probe_cur.bmHints & 0x0010) > 0));
	gtk_widget_show (h264_controls->Hints_slicemode);
	h264_controls->Hints_sliceunit = gtk_check_button_new_with_label (_("Slice Unit"));
	gtk_grid_attach (GTK_GRID(hints_table),h264_controls->Hints_sliceunit, 1, 3, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->Hints_sliceunit),((config_probe_cur.bmHints & 0x0020) > 0));
	gtk_widget_show (h264_controls->Hints_sliceunit);
	h264_controls->Hints_view = gtk_check_button_new_with_label (_("MVC View"));
	gtk_grid_attach (GTK_GRID(hints_table),h264_controls->Hints_view, 2, 3, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->Hints_view),((config_probe_cur.bmHints & 0x0040) > 0));
	gtk_widget_show (h264_controls->Hints_view);
	h264_controls->Hints_temporal = gtk_check_button_new_with_label (_("Temporal Scale"));
	gtk_grid_attach (GTK_GRID(hints_table),h264_controls->Hints_temporal, 3, 3, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->Hints_temporal),((config_probe_cur.bmHints & 0x0080) > 0));
	gtk_widget_show (h264_controls->Hints_temporal);
	h264_controls->Hints_snr = gtk_check_button_new_with_label (_("SNR Scale"));
	gtk_grid_attach (GTK_GRID(hints_table),h264_controls->Hints_snr, 0, 4, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->Hints_snr),((config_probe_cur.bmHints & 0x0100) > 0));
	gtk_widget_show (h264_controls->Hints_snr);
	h264_controls->Hints_spatial = gtk_check_button_new_with_label (_("Spatial Scale"));
	gtk_grid_attach (GTK_GRID(hints_table),h264_controls->Hints_spatial, 1, 4, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->Hints_spatial),((config_probe_cur.bmHints & 0x0200) > 0));
	gtk_widget_show (h264_controls->Hints_spatial);
	h264_controls->Hints_spatiallayer = gtk_check_button_new_with_label (_("Spatial Layer Ratio"));
	gtk_grid_attach (GTK_GRID(hints_table),h264_controls->Hints_spatiallayer, 2, 4, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->Hints_spatiallayer),((config_probe_cur.bmHints & 0x0400) > 0));
	gtk_widget_show (h264_controls->Hints_spatiallayer);
	h264_controls->Hints_frameinterval = gtk_check_button_new_with_label (_("Frame Interval"));
	gtk_grid_attach (GTK_GRID(hints_table),h264_controls->Hints_frameinterval, 3, 4, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->Hints_frameinterval),((config_probe_cur.bmHints & 0x0800) > 0));
	gtk_widget_show (h264_controls->Hints_frameinterval);
	h264_controls->Hints_leakybucket = gtk_check_button_new_with_label (_("Leaky Bucket Size"));
	gtk_grid_attach (GTK_GRID(hints_table),h264_controls->Hints_leakybucket, 0, 5, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->Hints_leakybucket),((config_probe_cur.bmHints & 0x1000) > 0));
	gtk_widget_show (h264_controls->Hints_leakybucket);
	h264_controls->Hints_bitrate = gtk_check_button_new_with_label (_("Bit Rate"));
	gtk_grid_attach (GTK_GRID(hints_table),h264_controls->Hints_bitrate, 1, 5, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->Hints_bitrate),((config_probe_cur.bmHints & 0x2000) > 0));
	gtk_widget_show (h264_controls->Hints_bitrate);
	h264_controls->Hints_cabac = gtk_check_button_new_with_label (_("CABAC"));
	gtk_grid_attach (GTK_GRID(hints_table),h264_controls->Hints_cabac, 2, 5, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->Hints_cabac),((config_probe_cur.bmHints & 0x4000) > 0));
	gtk_widget_show (h264_controls->Hints_cabac);
	h264_controls->Hints_iframe = gtk_check_button_new_with_label (_("(I) Frame Period"));
	gtk_grid_attach (GTK_GRID(hints_table),h264_controls->Hints_iframe, 3, 5, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->Hints_iframe),((config_probe_cur.bmHints & 0x8000) > 0));
	gtk_widget_show (h264_controls->Hints_iframe);

	gtk_grid_attach (GTK_GRID(table), hints_table, 0, line, 2, 1);
	gtk_widget_show(hints_table);

	//wWidth x wHeight
	line++;
	GtkWidget* label_Resolution = gtk_label_new(_("Resolution:"));
	gtk_misc_set_alignment (GTK_MISC (label_Resolution), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_Resolution, 0, line, 1, 1);
	gtk_widget_show (label_Resolution);

	h264_controls->Resolution = gtk_combo_box_text_new ();
	gtk_widget_set_halign (h264_controls->Resolution, GTK_ALIGN_FILL);
	gtk_widget_set_hexpand (h264_controls->Resolution, TRUE);
	char temp_str[20];
	int defres=0;

	int h264_format_ind = get_FormatIndex(videoIn->listFormats,V4L2_PIX_FMT_H264);
	VidFormats *listVidFormats = &videoIn->listFormats->listVidFormats[h264_format_ind];

	int i = 0;
	for(i = 0 ; i < listVidFormats->numb_res ; i++)
	{
		if (listVidFormats->listVidCap[i].width>0)
		{
			g_snprintf(temp_str,18,"%ix%i", listVidFormats->listVidCap[i].width,
							 listVidFormats->listVidCap[i].height);
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(h264_controls->Resolution),temp_str);

			if ((config_probe_cur.wWidth == listVidFormats->listVidCap[i].width) &&
				(config_probe_cur.wHeight == listVidFormats->listVidCap[i].height))
					defres=i;//set selected resolution index
		}
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(h264_controls->Resolution),defres);
	gtk_grid_attach (GTK_GRID(table), h264_controls->Resolution, 1, line, 1 ,1);
	gtk_widget_show (h264_controls->Resolution);
	
	//wSliceMode
	line++;
	GtkWidget* label_SliceMode = gtk_label_new(_("Slice Mode:"));
	gtk_misc_set_alignment (GTK_MISC (label_SliceMode), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_SliceMode, 0, line, 1, 1);
	gtk_widget_show (label_SliceMode);

	h264_controls->SliceMode = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->SliceMode),
								_("no multiple slices"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->SliceMode),
								_("bits/slice"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->SliceMode),
								_("Mbits/slice"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->SliceMode),
								_("slices/frame"));

	gtk_combo_box_set_active(GTK_COMBO_BOX(h264_controls->SliceMode), config_probe_cur.wSliceMode); //0 indexed

	gtk_grid_attach (GTK_GRID(table), h264_controls->SliceMode, 1, line, 1 ,1);
	gtk_widget_show (h264_controls->SliceMode);

	//wSliceUnits
	line++;
	GtkWidget* label_SliceUnits = gtk_label_new(_("Slice Units:"));
	gtk_misc_set_alignment (GTK_MISC (label_SliceUnits), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_SliceUnits, 0, line, 1, 1);
	gtk_widget_show (label_SliceUnits);

	GtkAdjustment *adjustment2 = gtk_adjustment_new (
                                	config_probe_cur.wSliceUnits,
                                	config_probe_min.wSliceUnits,
                                    config_probe_max.wSliceUnits,
                                    1,
                                    10,
                                    0);

    h264_controls->SliceUnits = gtk_spin_button_new(adjustment2, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(h264_controls->SliceUnits), TRUE);

    gtk_grid_attach (GTK_GRID(table), h264_controls->SliceUnits, 1, line, 1 ,1);
    gtk_widget_show (h264_controls->SliceUnits);

	//wProfile
	line++;
	GtkWidget* label_Profile = gtk_label_new(_("Profile:"));
	gtk_misc_set_alignment (GTK_MISC (label_Profile), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_Profile, 0, line, 1, 1);
	gtk_widget_show (label_Profile);

	h264_controls->Profile = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->Profile),
								_("Baseline Profile"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->Profile),
								_("Main Profile"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->Profile),
								_("High Profile"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->Profile),
								_("Scalable Baseline Profile"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->Profile),
								_("Scalable High Profile"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->Profile),
								_("Multiview High Profile"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->Profile),
								_("Stereo High Profile"));

	uint16_t profile = config_probe_cur.wProfile & 0xFF00;
	int prof_index = 0;
	switch(profile)
	{
		case 0x4200:
			prof_index = 0;
			break;
		case 0x4D00:
			prof_index = 1;
			break;
		case 0x6400:
			prof_index = 2;
			break;
		case 0x5300:
			prof_index = 3;
			break;
		case 0x5600:
			prof_index = 4;
			break;
		case 0x7600:
			prof_index = 5;
			break;
		case 0x8000:
			prof_index = 6;
			break;
		default:
			fprintf(stderr, "H264 probe: unknown profile mode 0x%X\n", profile);
			break;

	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(h264_controls->Profile), prof_index);

	gtk_grid_attach (GTK_GRID(table), h264_controls->Profile, 1, line, 1 ,1);
	gtk_widget_show (h264_controls->Profile);

	//wProfile (Bits 0-7)
	line++;
	GtkWidget* label_Profile_flags = gtk_label_new(_("Profile flags:"));
	gtk_misc_set_alignment (GTK_MISC (label_Profile_flags), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_Profile_flags, 0, line, 1, 1);
	gtk_widget_show (label_Profile_flags);

	int cur_flags = config_probe_cur.wProfile & 0x000000FF;
	int max_flags = config_probe_max.wProfile & 0x000000FF;
	int min_flags = config_probe_min.wProfile & 0x000000FF;

	GtkAdjustment *adjustment3 = gtk_adjustment_new (
                                	cur_flags,
                                	min_flags,
                                    max_flags,
                                    1,
                                    10,
                                    0);

    h264_controls->Profile_flags = gtk_spin_button_new(adjustment3, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(h264_controls->Profile_flags), TRUE);

    gtk_grid_attach (GTK_GRID(table), h264_controls->Profile_flags, 1, line, 1 ,1);
    gtk_widget_show (h264_controls->Profile_flags);

	//wIFramePeriod
	line++;
	GtkWidget* label_IFramePeriod = gtk_label_new(_("(I) Frame Period (ms):"));
	gtk_misc_set_alignment (GTK_MISC (label_IFramePeriod), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_IFramePeriod, 0, line, 1, 1);
	gtk_widget_show (label_IFramePeriod);

	GtkAdjustment *adjustment4 = gtk_adjustment_new (
                                	config_probe_cur.wIFramePeriod,
                                	config_probe_min.wIFramePeriod,
                                    config_probe_max.wIFramePeriod,
                                    1,
                                    10,
                                    0);

    h264_controls->IFramePeriod = gtk_spin_button_new(adjustment4, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(h264_controls->IFramePeriod), TRUE);

    gtk_grid_attach (GTK_GRID(table), h264_controls->IFramePeriod, 1, line, 1 ,1);
    gtk_widget_show (h264_controls->IFramePeriod);

	//wEstimatedVideoDelay
	line++;
	GtkWidget* label_EstimatedVideoDelay = gtk_label_new(_("Estimated Video Delay (ms):"));
	gtk_misc_set_alignment (GTK_MISC (label_EstimatedVideoDelay), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_EstimatedVideoDelay, 0, line, 1, 1);
	gtk_widget_show (label_EstimatedVideoDelay);

	GtkAdjustment *adjustment5 = gtk_adjustment_new (
                                	config_probe_cur.wEstimatedVideoDelay,
                                	config_probe_min.wEstimatedVideoDelay,
                                    config_probe_max.wEstimatedVideoDelay,
                                    1,
                                    10,
                                    0);

    h264_controls->EstimatedVideoDelay = gtk_spin_button_new(adjustment5, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(h264_controls->EstimatedVideoDelay), TRUE);

    gtk_grid_attach (GTK_GRID(table), h264_controls->EstimatedVideoDelay, 1, line, 1 ,1);
    gtk_widget_show (h264_controls->EstimatedVideoDelay);

    //wEstimatedMaxConfigDelay
	line++;
	GtkWidget* label_EstimatedMaxConfigDelay = gtk_label_new(_("Estimated Max Config Delay (ms):"));
	gtk_misc_set_alignment (GTK_MISC (label_EstimatedMaxConfigDelay), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_EstimatedMaxConfigDelay, 0, line, 1, 1);
	gtk_widget_show (label_EstimatedMaxConfigDelay);

	GtkAdjustment *adjustment6 = gtk_adjustment_new (
                                	config_probe_cur.wEstimatedMaxConfigDelay,
                                	config_probe_min.wEstimatedMaxConfigDelay,
                                    config_probe_max.wEstimatedMaxConfigDelay,
                                    1,
                                    10,
                                    0);

    h264_controls->EstimatedMaxConfigDelay = gtk_spin_button_new(adjustment6, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(h264_controls->EstimatedMaxConfigDelay), TRUE);

    gtk_grid_attach (GTK_GRID(table), h264_controls->EstimatedMaxConfigDelay, 1, line, 1 ,1);
    gtk_widget_show (h264_controls->EstimatedMaxConfigDelay);

	//bUsageType
	line++;
	GtkWidget* label_UsageType = gtk_label_new(_("Usage Type:"));
	gtk_misc_set_alignment (GTK_MISC (label_UsageType), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_UsageType, 0, line, 1, 1);
	gtk_widget_show (label_UsageType);

	h264_controls->UsageType = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->UsageType),
								_("Real-time"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->UsageType),
								_("Broadcast"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->UsageType),
								_("Storage"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->UsageType),
								_("(0) Non-scalable single layer AVC"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->UsageType),
								_("(1) SVC temporal scalability with hierarchical P"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->UsageType),
								_("(2q) SVC temporal scalability + Quality/SNR scalability"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->UsageType),
								_("(2s) SVC temporal scalability + spatial scalability"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->UsageType),
								_("(3) Full SVC scalability"));

	uint8_t usage = config_probe_cur.bUsageType & 0x0F;
	int usage_index = usage - 1; // from 0x01 to 0x0F
	if(usage_index < 0)
		usage_index = 0;

	gtk_combo_box_set_active(GTK_COMBO_BOX(h264_controls->UsageType), usage_index);

	gtk_grid_attach (GTK_GRID(table), h264_controls->UsageType, 1, line, 1 ,1);
	gtk_widget_show (h264_controls->UsageType);
	
	//bRateControlMode
	line++;
	GtkWidget* label_RateControlMode = gtk_label_new(_("Rate Control Mode:"));
	gtk_misc_set_alignment (GTK_MISC (label_RateControlMode), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_RateControlMode, 0, line, 1, 1);
	gtk_widget_show (label_RateControlMode);

	h264_controls->RateControlMode = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->RateControlMode),
								_("CBR"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->RateControlMode),
								_("VBR"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->RateControlMode),
								_("Constant QP"));

	uint8_t ratecontrolmode = config_probe_cur.bRateControlMode & 0x03;
	int ratevontrolmode_index = ratecontrolmode - 1; // from 0x01 to 0x03
	if(ratevontrolmode_index < 0)
		ratevontrolmode_index = 0;

	gtk_combo_box_set_active(GTK_COMBO_BOX(h264_controls->RateControlMode), ratevontrolmode_index);

	gtk_grid_attach (GTK_GRID(table), h264_controls->RateControlMode, 1, line, 1 ,1);
	gtk_widget_show (h264_controls->RateControlMode);

	//bRateControlMode flags (Bits 4-8)
	line++;
	GtkWidget* label_RateControlMode_cbr_flag = gtk_label_new(_("Rate Control Mode flags:"));
	gtk_misc_set_alignment (GTK_MISC (label_RateControlMode_cbr_flag), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_RateControlMode_cbr_flag, 0, line, 1, 1);
	gtk_widget_show (label_RateControlMode_cbr_flag);

	cur_flags = config_probe_cur.bRateControlMode & 0x0000001C;
	max_flags = config_probe_max.bRateControlMode & 0x0000001C;
	min_flags = config_probe_min.bRateControlMode & 0x0000001C;

	GtkAdjustment *adjustment7 = gtk_adjustment_new (
                                	cur_flags,
                                	min_flags,
                                    max_flags,
                                    1,
                                    10,
                                    0);

    h264_controls->RateControlMode_cbr_flag = gtk_spin_button_new(adjustment7, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(h264_controls->RateControlMode_cbr_flag), TRUE);

    gtk_grid_attach (GTK_GRID(table), h264_controls->RateControlMode_cbr_flag, 1, line, 1 ,1);
    gtk_widget_show (h264_controls->RateControlMode_cbr_flag);

	//bTemporalScaleMode
	line++;
	GtkWidget* label_TemporalScaleMode = gtk_label_new(_("Temporal Scale Mode:"));
	gtk_misc_set_alignment (GTK_MISC (label_TemporalScaleMode), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_TemporalScaleMode, 0, line, 1, 1);
	gtk_widget_show (label_TemporalScaleMode);

	cur_flags = config_probe_cur.bTemporalScaleMode & 0x0000000F;
	max_flags = config_probe_max.bTemporalScaleMode & 0x0000000F;
	min_flags = config_probe_min.bTemporalScaleMode & 0x0000000F;

	GtkAdjustment *adjustment8 = gtk_adjustment_new (
                                	cur_flags,
                                	min_flags,
                                    max_flags,
                                    1,
                                    10,
                                    0);

    h264_controls->TemporalScaleMode = gtk_spin_button_new(adjustment8, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(h264_controls->TemporalScaleMode), TRUE);

    gtk_grid_attach (GTK_GRID(table), h264_controls->TemporalScaleMode, 1, line, 1 ,1);
    gtk_widget_show (h264_controls->TemporalScaleMode);

	//bSpatialScaleMode
	line++;
	GtkWidget* label_SpatialScaleMode = gtk_label_new(_("Spatial Scale Mode:"));
	gtk_misc_set_alignment (GTK_MISC (label_SpatialScaleMode), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_SpatialScaleMode, 0, line, 1, 1);
	gtk_widget_show (label_SpatialScaleMode);

	cur_flags = config_probe_cur.bSpatialScaleMode & 0x0000000F;
	max_flags = config_probe_max.bSpatialScaleMode & 0x0000000F;
	min_flags = config_probe_min.bSpatialScaleMode & 0x0000000F;

	GtkAdjustment *adjustment9 = gtk_adjustment_new (
                                	cur_flags,
                                	min_flags,
                                    max_flags,
                                    1,
                                    10,
                                    0);

    h264_controls->SpatialScaleMode = gtk_spin_button_new(adjustment9, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(h264_controls->SpatialScaleMode), TRUE);

    gtk_grid_attach (GTK_GRID(table), h264_controls->SpatialScaleMode, 1, line, 1 ,1);
    gtk_widget_show (h264_controls->SpatialScaleMode);

	//bSNRScaleMode
	line++;
	GtkWidget* label_SNRScaleMode = gtk_label_new(_("SNR Control Mode:"));
	gtk_misc_set_alignment (GTK_MISC (label_SNRScaleMode), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_SNRScaleMode, 0, line, 1, 1);
	gtk_widget_show (label_SNRScaleMode);

	h264_controls->SNRScaleMode = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->SNRScaleMode),
								_("No SNR Enhancement Layer"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->SNRScaleMode),
								_("CGS NonRewrite (2 Layer)"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->SNRScaleMode),
								_("CGS NonRewrite (3 Layer)"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->SNRScaleMode),
								_("CGS Rewrite (2 Layer)"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->SNRScaleMode),
								_("CGS Rewrite (3 Layer)"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->SNRScaleMode),
								_("MGS (2 Layer)"));

	uint8_t snrscalemode = config_probe_cur.bSNRScaleMode & 0x0F;
	int snrscalemode_index = 0;
	switch(snrscalemode)
	{
		case 0:
			snrscalemode_index = 0;
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			snrscalemode_index = snrscalemode - 1;
			break;
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(h264_controls->SNRScaleMode), snrscalemode_index);

	gtk_grid_attach (GTK_GRID(table), h264_controls->SNRScaleMode, 1, line, 1 ,1);
	gtk_widget_show (h264_controls->SNRScaleMode);

	//bStreamMuxOption
	line++;
	GtkWidget* StreamMuxOption_table = gtk_grid_new();
	h264_controls->StreamMuxOption = gtk_check_button_new_with_label (_("Stream Mux Enable"));
	gtk_grid_attach (GTK_GRID(StreamMuxOption_table), h264_controls->StreamMuxOption, 0, 1, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->StreamMuxOption),((config_probe_cur.bStreamMuxOption & 0x01) != 0));
	gtk_widget_show (h264_controls->StreamMuxOption);

	h264_controls->StreamMuxOption_aux = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->StreamMuxOption_aux),
								_("Embed H.264 aux stream"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->StreamMuxOption_aux),
								_("Embed YUY2 aux stream"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->StreamMuxOption_aux),
								_("Embed NV12 aux stream"));

	uint8_t streammux = config_probe_cur.bStreamMuxOption & 0x0E;
	int streammux_index = 0;
	switch(streammux)
	{
		case 2:
			streammux_index = 0;
			break;
		case 4:
			streammux_index = 1;
			break;
		case 8:
			streammux_index = 2;
			break;
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(h264_controls->StreamMuxOption_aux), streammux_index);
	gtk_grid_attach (GTK_GRID(StreamMuxOption_table), h264_controls->StreamMuxOption_aux, 1, 1, 1, 1);

	h264_controls->StreamMuxOption_mjpgcontainer = gtk_check_button_new_with_label (_("MJPG payload container"));
	gtk_grid_attach (GTK_GRID(StreamMuxOption_table), h264_controls->StreamMuxOption_mjpgcontainer, 2, 1, 1, 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->StreamMuxOption_mjpgcontainer),((config_probe_cur.bStreamMuxOption & 0x40) != 0));
	gtk_widget_show (h264_controls->StreamMuxOption_mjpgcontainer);

	gtk_grid_attach (GTK_GRID(table), StreamMuxOption_table, 0, line, 2, 1);
	gtk_widget_show(StreamMuxOption_table);

	//bStreamFormat
	line++;
	GtkWidget* label_StreamFormat = gtk_label_new(_("Stream Format:"));
	gtk_misc_set_alignment (GTK_MISC (label_StreamFormat), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_StreamFormat, 0, line, 1, 1);
	gtk_widget_show (label_StreamFormat);

	h264_controls->StreamFormat = gtk_combo_box_text_new();
	//TODO: check for min and max values
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->StreamFormat),
								_("Byte stream format (H.264 Annex-B)"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->StreamFormat),
								_("NAL stream format"));

	uint8_t streamformat = config_probe_cur.bStreamFormat & 0x01;
	int streamformat_index = streamformat;
	gtk_combo_box_set_active(GTK_COMBO_BOX(h264_controls->StreamFormat), streamformat_index);

	gtk_grid_attach (GTK_GRID(table), h264_controls->StreamFormat, 1, line, 1 ,1);
	gtk_widget_show (h264_controls->StreamFormat);

	//bEntropyCABAC
	line++;
	GtkWidget* label_EntropyCABAC = gtk_label_new(_("Entropy CABAC:"));
	gtk_misc_set_alignment (GTK_MISC (label_EntropyCABAC), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_EntropyCABAC, 0, line, 1, 1);
	gtk_widget_show (label_EntropyCABAC);

	h264_controls->EntropyCABAC = gtk_combo_box_text_new();
	//TODO: check for min and max values
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->EntropyCABAC),
								_("CAVLC"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(h264_controls->EntropyCABAC),
								_("CABAC"));

	uint8_t entropycabac = config_probe_cur.bEntropyCABAC & 0x01;
	int entropycabac_index = entropycabac;
	gtk_combo_box_set_active(GTK_COMBO_BOX(h264_controls->EntropyCABAC), entropycabac_index);

	gtk_grid_attach (GTK_GRID(table), h264_controls->EntropyCABAC, 1, line, 1 ,1);
	gtk_widget_show (h264_controls->EntropyCABAC);

	//bTimestamp
	line++;
	h264_controls->Timestamp = gtk_check_button_new_with_label (_("Picture timing SEI"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->Timestamp),((config_probe_cur.bTimestamp & 0x01) > 0));
	gtk_grid_attach (GTK_GRID(table), h264_controls->Timestamp, 1, line, 1 ,1);
	gtk_widget_show (h264_controls->Timestamp);

	//bNumOfReorderFrames
	line++;
	GtkWidget* label_NumOfReorderFrames = gtk_label_new(_("B Frames:"));
	gtk_misc_set_alignment (GTK_MISC (label_NumOfReorderFrames), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_NumOfReorderFrames, 0, line, 1, 1);
	gtk_widget_show (label_NumOfReorderFrames);

	cur_flags = config_probe_cur.bNumOfReorderFrames & 0x000000FF;
	max_flags = config_probe_max.bNumOfReorderFrames & 0x000000FF;
	min_flags = config_probe_min.bNumOfReorderFrames & 0x000000FF;

	GtkAdjustment *adjustment10 = gtk_adjustment_new (
                                	cur_flags,
                                	min_flags,
                                    max_flags,
                                    1,
                                    10,
                                    0);

    h264_controls->NumOfReorderFrames = gtk_spin_button_new(adjustment10, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(h264_controls->NumOfReorderFrames), TRUE);

    gtk_grid_attach (GTK_GRID(table), h264_controls->NumOfReorderFrames, 1, line, 1 ,1);
    gtk_widget_show (h264_controls->NumOfReorderFrames);

	//bPreviewFlipped
	line++;
	h264_controls->PreviewFlipped = gtk_check_button_new_with_label (_("Preview Flipped"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(h264_controls->PreviewFlipped),((config_probe_cur.bPreviewFlipped & 0x01) > 0));
	gtk_grid_attach (GTK_GRID(table), h264_controls->PreviewFlipped, 1, line, 1 ,1);
	gtk_widget_show (h264_controls->PreviewFlipped);

	//bView
	line++;
	GtkWidget* label_View = gtk_label_new(_("Additional MVC Views:"));
	gtk_misc_set_alignment (GTK_MISC (label_View), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_View, 0, line, 1, 1);
	gtk_widget_show (label_View);

	cur_flags = config_probe_cur.bView & 0x000000FF;
	max_flags = config_probe_max.bView & 0x000000FF;
	min_flags = config_probe_min.bView & 0x000000FF;

	GtkAdjustment *adjustment11 = gtk_adjustment_new (
                                	cur_flags,
                                	min_flags,
                                    max_flags,
                                    1,
                                    10,
                                    0);

    h264_controls->View = gtk_spin_button_new(adjustment11, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(h264_controls->View), TRUE);

    gtk_grid_attach (GTK_GRID(table), h264_controls->View, 1, line, 1 ,1);
    gtk_widget_show (h264_controls->View);

    //bStreamID
	line++;
	GtkWidget* label_StreamID = gtk_label_new(_("Simulcast stream index:"));
	gtk_misc_set_alignment (GTK_MISC (label_StreamID), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_StreamID, 0, line, 1, 1);
	gtk_widget_show (label_StreamID);

	cur_flags = config_probe_cur.bStreamID & 0x000000FF;
	max_flags = config_probe_max.bStreamID & 0x000000FF;
	min_flags = config_probe_min.bStreamID & 0x000000FF;

	GtkAdjustment *adjustment12 = gtk_adjustment_new (
                                	cur_flags,
                                	min_flags,
                                    max_flags,
                                    1,
                                    10,
                                    0);

    h264_controls->StreamID = gtk_spin_button_new(adjustment12, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(h264_controls->StreamID), TRUE);

    gtk_grid_attach (GTK_GRID(table), h264_controls->StreamID, 1, line, 1 ,1);
    gtk_widget_show (h264_controls->StreamID);

    //bSpatialLayerRatio
	line++;
	GtkWidget* label_SpatialLayerRatio = gtk_label_new(_("Spatial Layer Ratio:"));
	gtk_misc_set_alignment (GTK_MISC (label_SpatialLayerRatio), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_SpatialLayerRatio, 0, line, 1, 1);
	gtk_widget_show (label_SpatialLayerRatio);

	cur_flags = config_probe_cur.bSpatialLayerRatio & 0x000000FF;
	max_flags = config_probe_max.bSpatialLayerRatio & 0x000000FF;
	min_flags = config_probe_min.bSpatialLayerRatio & 0x000000FF;

	gdouble cur = (gdouble) ((cur_flags & 0x000000F0)>>4) + (gdouble)((cur_flags & 0x0000000F)/16);
	gdouble min = (gdouble) ((min_flags & 0x000000F0)>>4) + (gdouble)((min_flags & 0x0000000F)/16);
	gdouble max = (gdouble) ((max_flags & 0x000000F0)>>4) + (gdouble)((max_flags & 0x0000000F)/16);

	GtkAdjustment *adjustment13 = gtk_adjustment_new (
                                	cur,
                                	min,
                                    max,
                                    0.1,
                                    1,
                                    1);

    h264_controls->SpatialLayerRatio = gtk_spin_button_new(adjustment13, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(h264_controls->SpatialLayerRatio), TRUE);

    gtk_grid_attach (GTK_GRID(table), h264_controls->SpatialLayerRatio, 1, line, 1 ,1);
    gtk_widget_show (h264_controls->SpatialLayerRatio);

	//wLeakyBucketSize
	line++;
	GtkWidget* label_LeakyBucketSize = gtk_label_new(_("Leaky Bucket Size (ms):"));
	gtk_misc_set_alignment (GTK_MISC (label_LeakyBucketSize), 1, 0.5);
	gtk_grid_attach (GTK_GRID(table), label_LeakyBucketSize, 0, line, 1, 1);
	gtk_widget_show (label_LeakyBucketSize);

	cur_flags = config_probe_cur.wLeakyBucketSize;
	max_flags = config_probe_max.wLeakyBucketSize;
	min_flags = config_probe_min.wLeakyBucketSize;

	GtkAdjustment *adjustment14 = gtk_adjustment_new (
                                	cur_flags,
                                	min_flags,
                                    max_flags,
                                    1,
                                    10,
                                    0);

    h264_controls->LeakyBucketSize = gtk_spin_button_new(adjustment14, 1, 0);
    gtk_editable_set_editable(GTK_EDITABLE(h264_controls->LeakyBucketSize), TRUE);

    gtk_grid_attach (GTK_GRID(table), h264_controls->LeakyBucketSize, 1, line, 1 ,1);
    gtk_widget_show (h264_controls->LeakyBucketSize);

	//PROBE COMMIT buttons
	line++;

	h264_controls->probe_button = gtk_button_new_with_label(_("PROBE"));
	g_signal_connect (GTK_BUTTON(h264_controls->probe_button), "clicked",
                                G_CALLBACK (h264_probe_button_clicked), all_data);

    gtk_grid_attach (GTK_GRID(table), h264_controls->probe_button, 0, line, 1 ,1);
	gtk_widget_show(h264_controls->probe_button);

	h264_controls->commit_button = gtk_button_new_with_label(_("COMMIT"));
	g_signal_connect (GTK_BUTTON(h264_controls->commit_button), "clicked",
                                G_CALLBACK (h264_commit_button_clicked), all_data);

    gtk_grid_attach (GTK_GRID(table), h264_controls->commit_button, 1, line, 1 ,1);
	gtk_widget_show(h264_controls->commit_button);

	gtk_widget_show(table);

}

/* get the unit id for GUID_UVCX_H264_XU by using libusb */
uint8_t xu_get_unit_id (uint64_t busnum, uint64_t devnum)
{
    /* use libusb */
	libusb_context *usb_ctx = NULL;
    libusb_device **device_list = NULL;
    libusb_device *device = NULL;
    ssize_t cnt;
    int i, j, k;

	static const uint8_t guid[16] = GUID_UVCX_H264_XU;
	uint8_t unit = 0;

    if (usb_ctx == NULL)
      libusb_init (&usb_ctx);

    cnt = libusb_get_device_list (usb_ctx, &device_list);
    for (i = 0; i < cnt; i++)
    {
		uint64_t dev_busnum = libusb_get_bus_number (device_list[i]);
		uint64_t dev_devnum = libusb_get_device_address (device_list[i]);
		if (busnum == dev_busnum &&	devnum == dev_devnum)
		{
			device = libusb_ref_device (device_list[i]);
			break;
		}
	}

	libusb_free_device_list (device_list, 1);

	if (device)
	{
		struct libusb_device_descriptor desc;

		 if (libusb_get_device_descriptor (device, &desc) == 0)
		 {
			for (i = 0; i < desc.bNumConfigurations; ++i)
			{
				struct libusb_config_descriptor *config = NULL;

				if (libusb_get_config_descriptor (device, i, &config) == 0)
				{
					for (j = 0; j < config->bNumInterfaces; j++)
					{
						for (k = 0; k < config->interface[j].num_altsetting; k++)
						{
							const struct libusb_interface_descriptor *interface;
							const guint8 *ptr = NULL;

							interface = &config->interface[j].altsetting[k];
							if (interface->bInterfaceClass != LIBUSB_CLASS_VIDEO ||
								interface->bInterfaceSubClass != USB_VIDEO_CONTROL)
								continue;
							ptr = interface->extra;
							while (ptr - interface->extra +
								sizeof (xu_descriptor) < interface->extra_length)
							{
								xu_descriptor *desc = (xu_descriptor *) ptr;

								if (desc->bDescriptorType == USB_VIDEO_CONTROL_INTERFACE &&
									desc->bDescriptorSubType == USB_VIDEO_CONTROL_XU_TYPE &&
									memcmp (desc->guidExtensionCode, guid, 16) == 0)
								{
									uint8_t unit_id = desc->bUnitID;

									libusb_unref_device (device);
									return unit_id;
								}
								ptr += desc->bLength;
							}
						}
					}
				}
				else
					fprintf(stderr,"libusb: couldn't get config descriptor for configuration %i\n", i);
			}
		}
		else
			fprintf(stderr,"libusb: couldn't get device descriptor");
		libusb_unref_device (device);
	}

	return unit;
}

/*
 * check for uvc h264 support by querying UVCX_VERSION
 * although geting a unit id > 0 from xu_get_unit_id
 * should be enought
 */
int has_h264_support(int hdevice, uint8_t unit_id)
{
	if(unit_id <= 0)
	{
		g_printerr("device doesn't seem to support uvc H264 (%i)\n", unit_id);
		return 0;
	}

	uvcx_version_t uvcx_version;

	if(query_xu_control(hdevice, unit_id, UVCX_VERSION, UVC_GET_CUR, &uvcx_version) < 0)
	{
		g_printerr("device doesn't seem to support uvc H264 in unit_id %d\n", unit_id);
		return 0;
	}
	else
	{
		g_printerr("device seems to support uvc H264 (version: %d) in unit_id %d\n", uvcx_version.wVersion, unit_id);
		return 1;
	}
}

/*
 * called if uvc h264 is supported
 * adds h264 to the format list
 */
void check_uvc_h264_format(struct vdIn *vd, struct GLOBAL *global)
{
	if(global->uvc_h264_unit < 0)
		return;

	if(get_FormatIndex(vd->listFormats, V4L2_PIX_FMT_H264) >= 0)
		return; //H264 is already in the list

	//add format to the list
	int mjpg_index = get_FormatIndex(vd->listFormats, V4L2_PIX_FMT_MJPEG);
	if(mjpg_index < 0) //MJPG must be available for uvc H264 streams
		return;

	set_SupPixFormatUvcH264();

	vd->listFormats->numb_formats++; //increment number of formats
	int fmtind = vd->listFormats->numb_formats;

	vd->listFormats->listVidFormats = g_renew(VidFormats, vd->listFormats->listVidFormats, fmtind);
	vd->listFormats->listVidFormats[fmtind-1].format = V4L2_PIX_FMT_H264;
	g_snprintf(vd->listFormats->listVidFormats[fmtind-1].fourcc ,5,"H264");
	vd->listFormats->listVidFormats[fmtind-1].listVidCap = NULL;
	vd->listFormats->listVidFormats[fmtind-1].numb_res = 0;
	//enumerate frame sizes with UVCX_VIDEO_CONFIG_PROBE
	//get PROBE info (def, max, min, cur)
	uvcx_video_config_probe_commit_t config_probe_def;
	uvcx_video_config_probe_commit_t config_probe_max;
	uvcx_video_config_probe_commit_t config_probe_min;
	uvcx_video_config_probe_commit_t config_probe_cur;
	uvcx_video_config_probe_commit_t config_probe_test;

	uvcx_video_probe(vd->fd, global->uvc_h264_unit, UVC_GET_DEF, &config_probe_def);
	uvcx_video_probe(vd->fd, global->uvc_h264_unit, UVC_GET_MAX, &config_probe_max);
	uvcx_video_probe(vd->fd, global->uvc_h264_unit, UVC_GET_MIN, &config_probe_min);
	uvcx_video_probe(vd->fd, global->uvc_h264_unit, UVC_GET_CUR, &config_probe_cur);

	//check MJPG resolutions and frame rates for H264
	int numb_res = vd->listFormats->listVidFormats[mjpg_index].numb_res;

	int i=0, j=0;
	int res_index = 0;
	for(i=0; i < numb_res; i++)
	{
		config_probe_test = config_probe_def;

		int width = vd->listFormats->listVidFormats[mjpg_index].listVidCap[i].width;
		int height = vd->listFormats->listVidFormats[mjpg_index].listVidCap[i].height;

		config_probe_test.wWidth = width;
		config_probe_test.wHeight = height;

		if(!uvcx_video_probe(vd->fd, global->uvc_h264_unit, UVC_SET_CUR, &config_probe_test))
			continue;

		if(!uvcx_video_probe(vd->fd, global->uvc_h264_unit, UVC_GET_CUR, &config_probe_test))
			continue;

		if(config_probe_test.wWidth != width || config_probe_test.wHeight != height)
		{
			fprintf(stderr, "H264 resolution %ix%i not supported\n", width, height);
			continue;
		}

		res_index++;
		vd->listFormats->listVidFormats[fmtind-1].listVidCap = g_renew(VidCap,
			vd->listFormats->listVidFormats[fmtind-1].listVidCap, res_index);
		vd->listFormats->listVidFormats[fmtind-1].numb_res = res_index;
		vd->listFormats->listVidFormats[fmtind-1].listVidCap[res_index-1].width = width;
		vd->listFormats->listVidFormats[fmtind-1].listVidCap[res_index-1].height = height;
		vd->listFormats->listVidFormats[fmtind-1].listVidCap[res_index-1].framerate_num = NULL;
		vd->listFormats->listVidFormats[fmtind-1].listVidCap[res_index-1].framerate_denom = NULL;
		vd->listFormats->listVidFormats[fmtind-1].listVidCap[res_index-1].numb_frates = 0;

		//check frates
		int numb_frates = vd->listFormats->listVidFormats[mjpg_index].listVidCap[i].numb_frates;
		int frate_index = 0;
		for(j=0; j < numb_frates; j++)
		{
			int framerate_num = vd->listFormats->listVidFormats[mjpg_index].listVidCap[i].framerate_num[j];
			int framerate_denom = vd->listFormats->listVidFormats[mjpg_index].listVidCap[i].framerate_denom[j];
			//in 100ns units
			uint32_t frame_interval = (framerate_num * 1000000000LL / framerate_denom)/100;
			config_probe_test.dwFrameInterval = frame_interval;

			if(!uvcx_video_probe(vd->fd, global->uvc_h264_unit, UVC_SET_CUR, &config_probe_test))
				continue;

			if(!uvcx_video_probe(vd->fd, global->uvc_h264_unit, UVC_GET_CUR, &config_probe_test))
				continue;

			if(config_probe_test.dwFrameInterval != frame_interval)
			{
				fprintf(stderr, "H264 resolution %ix%i with frame_rate %i not supported\n", width, height, frame_interval);
				continue;
			}

			frate_index++;
			vd->listFormats->listVidFormats[fmtind-1].listVidCap[res_index-1].numb_frates = frate_index;
			vd->listFormats->listVidFormats[fmtind-1].listVidCap[res_index-1].framerate_num = g_renew(int,
				vd->listFormats->listVidFormats[fmtind-1].listVidCap[res_index-1].framerate_num, frate_index);
			vd->listFormats->listVidFormats[fmtind-1].listVidCap[res_index-1].framerate_num[frate_index-1] = framerate_num;
			vd->listFormats->listVidFormats[fmtind-1].listVidCap[res_index-1].framerate_denom = g_renew(int,
				vd->listFormats->listVidFormats[fmtind-1].listVidCap[res_index-1].framerate_denom, frate_index);
			vd->listFormats->listVidFormats[fmtind-1].listVidCap[res_index-1].framerate_denom[frate_index-1] = framerate_denom;
		}
	}

	//return config video probe to current state
	uvcx_video_probe(vd->fd, global->uvc_h264_unit, UVC_SET_CUR, &config_probe_cur);
}

void commit_uvc_h264_format(struct vdIn *vd, struct GLOBAL *global)
{
	uvcx_video_config_probe_commit_t config_probe_cur;
	uvcx_video_config_probe_commit_t config_probe_req;

	uvcx_video_probe(vd->fd, global->uvc_h264_unit, UVC_GET_CUR, &config_probe_cur);

	config_probe_req = config_probe_cur;

	config_probe_req.wWidth = global->width;
	config_probe_req.wHeight = global->height;
	//in 100ns units
	uint32_t frame_interval = (global->fps_num * 1000000000LL / global->fps)/100;
	config_probe_req.dwFrameInterval = frame_interval;

	//probe the format
	uvcx_video_probe(vd->fd, global->uvc_h264_unit, UVC_SET_CUR, &config_probe_req);
	uvcx_video_probe(vd->fd, global->uvc_h264_unit, UVC_GET_CUR, &config_probe_req);

	if(config_probe_req.wWidth != global->width)
	{
		fprintf(stderr, "H264 config probe: requested width %i but got %i\n",
			global->width, config_probe_req.wWidth);

		global->width = config_probe_req.wWidth;
	}
	if(config_probe_req.wHeight != global->height)
	{
		fprintf(stderr, "H264 config probe: requested height %i but got %i\n",
			global->height, config_probe_req.wHeight);

		global->height = config_probe_req.wHeight;
	}
	if(config_probe_req.dwFrameInterval != frame_interval)
	{
		fprintf(stderr, "H264 config probe: requested frame interval %i but got %i\n",
			frame_interval, config_probe_req.dwFrameInterval);
	}
	//commit the format
	print_probe_commit_data(&config_probe_req);
	uvcx_video_commit(vd->fd, global->uvc_h264_unit, &config_probe_req);
}

int uvcx_video_probe(int hdevice, uint8_t unit_id, uint8_t query, uvcx_video_config_probe_commit_t *uvcx_video_config)
{
	int err = 0;


	if((err = query_xu_control(hdevice, unit_id, UVCX_VIDEO_CONFIG_PROBE, query, uvcx_video_config)) < 0)
	{
		perror("UVCX_VIDEO_CONFIG_PROBE error");
		return err;
	}

	return err;
}

int uvcx_video_commit(int hdevice, uint8_t unit_id, uvcx_video_config_probe_commit_t *uvcx_video_config)
{
	int err = 0;

	if((err = query_xu_control(hdevice, unit_id, UVCX_VIDEO_CONFIG_COMMIT, UVC_SET_CUR, uvcx_video_config)) < 0)
	{
		perror("UVCX_VIDEO_CONFIG_COMMIT error");
		return err;
	}

	return err;
}


