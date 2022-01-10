/**
 * @file linux_alsa.c
 * @author ChineseGuuys
 * @brief linux alsa 音频驱动
 * @version 0.1
 * @date 2022-01-01
 *
 */

#define ALSA_PCM_NEW_HW_PARAMS_API

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include "wav.h"
#include <simplelogs.h>

snd_pcm_t           *gp_handle;
snd_pcm_hw_params_t *gp_params;
snd_pcm_uframes_t    g_frames;
char                *gp_buffer;
u32                  g_bufsize;
char                 filename[30];
WAV_HEADER           g_wave_header;

FILE *open_and_print_file_params(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        log_error("can't open wav file");
        return NULL;
    }

    memset(&g_wave_header, 0, sizeof(g_wave_header));
    fread(&g_wave_header, 1, sizeof(g_wave_header), fp);

    log_info("RiffID:%c%c%c%c", g_wave_header.RiffID[0], g_wave_header.RiffID[1], g_wave_header.RiffID[2], g_wave_header.RiffID[3]);
    log_info("RiffSize:%d", g_wave_header.RiffSize);
    log_info("WaveID:%c%c%c%c", g_wave_header.WaveID[0], g_wave_header.WaveID[1], g_wave_header.WaveID[2], g_wave_header.WaveID[3]);

    return fp;
}

int set_hardware_params() {
    int rc;
    rc = snd_pcm_open(&gp_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        log_error("unable to open pcm device");
        return -1;
    }

    snd_pcm_hw_params_alloca(&gp_params);

    rc = snd_pcm_hw_params_any(gp_handle, gp_params);
    if (rc < 0) {
        log_error("unable to Fill it in with default values.");
        goto err;
    }

    rc = snd_pcm_hw_params_set_access(gp_handle, gp_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (rc < 0) {
        log_error("unable to Interleaved mode.");
        goto err;
    }

    snd_pcm_format_t format;
    if (g_wave_header.FmtSize == 8) {
        format = SND_PCM_FORMAT_U8;
    } else if (g_wave_header.FmtSize == 16) {
        format = SND_PCM_FORMAT_S16_LE;
    } else if (g_wave_header.FmtSize == 24) {
        format = SND_PCM_FORMAT_U24_LE;
    } else if (g_wave_header.FmtSize == 32) {
        format = SND_PCM_FORMAT_U32_LE;
    } else {
        log_error("SND_PCM_FORMAT_UNKNOW");
        format = SND_PCM_FORMAT_U32_LE;
        goto err;
    }

    rc = snd_pcm_hw_params_set_format(gp_handle, gp_params, format);
    if (rc < 0) {
        log_error("unable to set format");
        goto err;
    }

    u32 dir, rate = g_wave_header.nSamplesPerSec;
    rc = snd_pcm_hw_params_set_rate_near(gp_handle, gp_params, &rate, &dir);
    if (rc < 0) {
        log_error("unable to set format rate");
        goto err;
    }

    rc = snd_pcm_hw_params_set_channels(gp_handle, gp_params, g_wave_header.nChannels);
    if (rc < 0) {
        log_error("unable to set channels");
        goto err;
    }

    /* 将参数写入驱动*/
    rc = snd_pcm_hw_params(gp_handle, gp_params);
    if (rc < 0) {
        log_error("unable to set hw parameters");
        goto err;
    }

    snd_pcm_hw_params_get_period_size(gp_params, &g_frames, &dir);
    g_bufsize = g_frames * 4;
    gp_buffer = (u8 *)malloc(g_bufsize);
    if (gp_buffer == NULL) {
        log_error("malloc failed");
        goto err;
    }

err:
    snd_pcm_close(gp_handle);
    return -1;
}

int play_wav(char *file) {
    FILE *fp = open_and_print_file_params(file);
    if (fp == NULL) {
        log_error("open and print file params error");
        return -1;
    }

    int ret = set_hardware_params();
    if (ret < 0) {
        log_error("set hardware params error");
        return -1;
    }

    size_t rc;
    while (1) {
        rc = fread(gp_buffer, g_bufsize, 1, fp);
        if (rc < 1) {
            break;
        }

        while ((ret = snd_pcm_writei(gp_handle, gp_buffer, g_frames)) < 0) {
            snd_pcm_prepare(gp_handle);
            log_error("buffer underrun occured");
        }
    }

    snd_pcm_drain(gp_handle);
    snd_pcm_close(gp_handle);
    free(gp_buffer);
    fclose(fp);
    return -1;
}

void *play_thread() {
    play_wav(filename);
}

int main(int argc, char *argv[]) {
    char audio[] = "../test.wav";
    strcpy(filename, audio);

    pthread_t player;
    if (pthread_create(&player, NULL, play_thread, NULL) == -1) {
        log_error("fail to create a player pthread");
    } else {
        log_info("succeed to create a player pthread");
    }

    pthread_join(player, NULL);
    log_info("the player pthread is over");
    return 0;
}