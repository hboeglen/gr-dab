/* ------------------------------------------------------------------
 * Copyright (C) 2011 Martin Storsjo
 * Copyright (C) 2017 Matthias P. Braendli
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */

/*! \mainpage Introduction
 *  The ODR-mmbTools ODR-AudioEnc Audio encoder can encode audio for
 *  ODR-DabMux, both DAB and DAB+. The DAB encoder is based on toolame. The
 *  DAB+ encoder requires a the Fraunhofer FDK AAC library, with the
 *  necessary patches for 960-transform to do DAB+ broadcast encoding.
 *
 *  This document describes some internals of the encoder, and is intended
 *  to help developers understand and improve the software package.
 *
 *  User documentation is available in the README and in the ODR-mmbTools
 *  Guide, available on the www.opendigitalradio.org website.
 *
 *  The readme for the whole package is \ref md_README
 *
 *  Interesting starting points for the encoder
 *  - \ref odr-audioenc.cpp Main encoder file
 *  - \ref VLCInput.h VLC Input
 *  - \ref AlsaInput.h Alsa Input
 *  - \ref JackInput.h JACK Input
 *  - \ref SampleQueue.h
 *  - \ref charset.h Charset conversion
 *  - \ref toolame.h libtolame API
 *  - \ref AudioLevel
 *  - \ref DataInput
 *  - \ref SilenceDetection
 *
 *  For the mot-encoder:
 *  - \ref mot-encoder.cpp
 *
 *
 *  \file odr-audioenc.cpp
 *  \brief The main file for the audio encoder
 */

#include "config.h"
#include "AlsaInput.h"
#include "FileInput.h"
#include "JackInput.h"
#include "VLCInput.h"
#include "SampleQueue.h"
#include "zmq.hpp"
#include "common.h"

extern "C" {
#include "encryption.h"
#include "utils.h"
#include "wavreader.h"
}

#include <vector>
#include <deque>
#include <chrono>
#include <thread>
#include <string>
#include <getopt.h>
#include <cstdio>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "fdk-aac/aacenc_lib.h"

extern "C" {
#include "contrib/fec/fec.h"
#include "libtoolame-dab/toolame.h"
}

using vec_u8 = std::vector<uint8_t>;

//! Enumeration of encoders we can use
enum class encoder_selection_t {
    fdk_dabplus,
    toolame_dab
};

using namespace std;

void usage(const char* name) {
    fprintf(stderr,
    "ODR-AudioEnc %s is an audio encoder for both DAB and DAB+.\n"
    "The encoder can read from JACK, ALSA or\n"
    "a file source and encode to a ZeroMQ output for ODR-DabMux.\n"
    "(Experimental!)It can also use libvlc as an input.\n"
    "\n"
    "The -D option enables experimental sound card clock drift compensation.\n"
    "A consumer sound card has a clock that is always a bit imprecise, and\n"
    "would drift off after some time. ODR-DabMux cannot handle such drift\n"
    "because it would have to throw away or insert a full DAB+ superframe,\n"
    "which would create audible artifacts. This drift compensation can\n"
    "make sure that the encoding rate is correct by inserting or deleting\n"
    "audio samples. It can be used for both ALSA and VLC inputs.\n"
    "\n"
    "When this option is enabled, you will see U and O printed in the\n"
    "console. These correspond to audio underruns and overruns caused\n"
    "by sound card clock drift. When sparse, they should not create audible\n"
    "artifacts.\n"
    "\n"
    "This encoder includes PAD (DLS and MOT Slideshow) support by\n"
    "http://rd.csp.it to be used with mot-encoder\n"
    "\nUsage:\n"
    "%s [INPUT SELECTION] [OPTION...]\n",
#if defined(GITVERSION)
    GITVERSION
#else
    PACKAGE_VERSION
#endif
    , name);
    fprintf(stderr,
    "   For the alsa input:\n"
#if HAVE_ALSA
    "     -d, --device=alsa_device             Set ALSA input device (default: \"default\").\n"
#else
    "     The Alsa input was disabled at compile time\n"
#endif
    "   For the file input:\n"
    "     -i, --input=FILENAME                 Input filename (default: stdin).\n"
    "     -f, --format={ wav, raw }            Set input file format (default: wav).\n"
    "         --fifo-silence                   Input file is fifo and encoder generates silence when fifo is empty. Ignore EOF.\n"
    "   For the JACK input:\n"
#if HAVE_JACK
    "     -j, --jack=name                      Enable JACK input, and define our name\n"
#else
    "     The JACK input was disabled at compile-time\n"
#endif
    "   For the VLC input:\n"
#if HAVE_VLC
    "     -v, --vlc-uri=uri                    Enable VLC input and use the URI given as source\n"
    "     -C, --vlc-cache=ms                   Specify VLC network cache length.\n"
    "     -g, --vlc-gain=db                    Enable VLC audio compressor, with given compressor-makeup value.\n"
    "                                          Use this as a workaround to correct the gain for streams that are\n"
    "                                          much too loud.\n"
    "     -V                                   Increase the VLC verbosity by one (can be given \n"
    "                                          multiple times)\n"
    "     -L OPTION                            Give an additional options to VLC (can be given\n"
    "                                          multiple times)\n"
    "     -w, --write-icy-text=filename        Write the ICY Text into the file, so that mot-encoder can read it.\n"
    "     -W, --write-icy-text-dl-plus         When writing the ICY Text into the file, add DL Plus information.\n"
#else
    "     The VLC input was disabled at compile-time\n"
#endif
    "   Drift compensation\n"
    "     -D, --drift-comp                     Enable ALSA/VLC sound card drift compensation.\n"
    "   Encoder parameters:\n"
    "     -b, --bitrate={ 8, 16, ..., 192 }    Output bitrate in kbps. Must be a multiple of 8.\n"
    "     -c, --channels={ 1, 2 }              Nb of input channels (default: 2).\n"
    "     -r, --rate={ 32000, 48000 }          Input sample rate (default: 48000).\n"
    "   DAB specific options\n"
    "     -a, --dab                            Encode in DAB and not in DAB+.\n"
    "         --dabmode=MODE                   Channel mode: s/d/j/m\n"
    "                                          (default: j if stereo, m if mono).\n"
    "         --dabpsy=PSY                     Psychoacoustic model 0/1/2/3\n"
    "                                          (default: 1).\n"
    "   DAB+ specific options\n"
    "     -A, --no-afterburner                 Disable AAC encoder quality increaser.\n"
    "         --aaclc                          Force the usage of AAC-LC (no SBR, no PS)\n"
    "         --sbr                            Force the usage of SBR (HE-AAC)\n"
    "         --ps                             Force the usage of SBR and PS (HE-AACv2)\n"
    "   Output and pad parameters:\n"
    "     -o, --output=URI                     Output ZMQ uri. (e.g. 'tcp://localhost:9000')\n"
    "                                     -or- Output file uri. (e.g. 'file.dabp')\n"
    "                                     -or- a single dash '-' to denote stdout\n"
    "                                          If more than one ZMQ output is given, the socket\n"
    "                                          will be connected to all listed endpoints.\n"
    "     -k, --secret-key=FILE                Enable ZMQ encryption with the given secret key.\n"
    "     -p, --pad=BYTES                      Set PAD size in bytes.\n"
    "     -P, --pad-fifo=FILENAME              Set PAD data input fifo name"
    "                                          (default: /tmp/pad.fifo).\n"
    "     -l, --level                          Show peak audio level indication.\n"
    "     -s, --silence=TIMEOUT                Abort encoding after TIMEOUT seconds of silence.\n"
    "\n"
    "Only the tcp:// zeromq transport has been tested until now,\n"
    " but epgm:// and pgm:// are also accepted\n"
    );

}

/*! Setup the FDK AAC encoder
 *
 * \return 0 on success
 */
int prepare_aac_encoder(
        HANDLE_AACENCODER *encoder,
        int subchannel_index,
        int channels,
        int sample_rate,
        int afterburner,
        int *aot)
{
    CHANNEL_MODE mode;
    switch (channels) {
        case 1: mode = MODE_1; break;
        case 2: mode = MODE_2; break;
        default:
                fprintf(stderr, "Unsupported channels number %d\n", channels);
                return 1;
    }


    if (aacEncOpen(encoder, 0x01|0x02|0x04, channels) != AACENC_OK) {
        fprintf(stderr, "Unable to open encoder\n");
        return 1;
    }

    if (*aot == AOT_NONE) {

        if(channels == 2 && subchannel_index <= 6) {
            *aot = AOT_DABPLUS_PS;
        }
        else if((channels == 1 && subchannel_index <= 8) || subchannel_index <= 10) {
            *aot = AOT_DABPLUS_SBR;
        }
        else {
            *aot = AOT_DABPLUS_AAC_LC;
        }
    }

    fprintf(stderr, "Using %d subchannels. AAC type: %s%s%s. channels=%d, sample_rate=%d\n",
            subchannel_index,
            *aot == AOT_DABPLUS_PS ? "HE-AAC v2" : "",
            *aot == AOT_DABPLUS_SBR ? "HE-AAC" : "",
            *aot == AOT_DABPLUS_AAC_LC ? "AAC-LC" : "",
            channels, sample_rate);

    if (aacEncoder_SetParam(*encoder, AACENC_AOT, *aot) != AACENC_OK) {
        fprintf(stderr, "Unable to set the AOT\n");
        return 1;
    }
    if (aacEncoder_SetParam(*encoder, AACENC_SAMPLERATE, sample_rate) != AACENC_OK) {
        fprintf(stderr, "Unable to set the sample rate\n");
        return 1;
    }
    if (aacEncoder_SetParam(*encoder, AACENC_CHANNELMODE, mode) != AACENC_OK) {
        fprintf(stderr, "Unable to set the channel mode\n");
        return 1;
    }
    if (aacEncoder_SetParam(*encoder, AACENC_CHANNELORDER, 1) != AACENC_OK) {
        fprintf(stderr, "Unable to set the wav channel order\n");
        return 1;
    }
    if (aacEncoder_SetParam(*encoder, AACENC_GRANULE_LENGTH, 960) != AACENC_OK) {
        fprintf(stderr, "Unable to set the granule length\n");
        return 1;
    }
    if (aacEncoder_SetParam(*encoder, AACENC_TRANSMUX, TT_DABPLUS) != AACENC_OK) {
        fprintf(stderr, "Unable to set the RAW transmux\n");
        return 1;
    }

    /*if (aacEncoder_SetParam(*encoder, AACENC_BITRATEMODE, AACENC_BR_MODE_SFR)
     * != AACENC_OK) {
        fprintf(stderr, "Unable to set the bitrate mode\n");
        return 1;
    }*/


    fprintf(stderr, "AAC bitrate set to: %d\n", subchannel_index*8000);
    if (aacEncoder_SetParam(*encoder, AACENC_BITRATE, subchannel_index*8000) != AACENC_OK) {
        fprintf(stderr, "Unable to set the bitrate\n");
        return 1;
    }
    if (aacEncoder_SetParam(*encoder, AACENC_AFTERBURNER, afterburner) != AACENC_OK) {
        fprintf(stderr, "Unable to set the afterburner mode\n");
        return 1;
    }
    if (!afterburner) {
        fprintf(stderr, "Warning: Afterburned disabled!\n");
    }
    if (aacEncEncode(*encoder, NULL, NULL, NULL, NULL) != AACENC_OK) {
        fprintf(stderr, "Unable to initialize the encoder\n");
        return 1;
    }
    return 0;
}

chrono::steady_clock::time_point timepoint_last_compensation;

/*! Do drift compensation by distributing the missing samples over
 *  the whole input buffer instead of having a bunch of missing samples
 *  at the end only.
 *
 *  This expands (in time) the received samples over the whole duration
 *  of the buffer.
 */
static void expand_missing_samples(vec_u8& buf, int channels, size_t valid_bytes)
{
    const size_t bytes_per_sample = BYTES_PER_SAMPLE * channels;
    assert(buf.size() % bytes_per_sample == 0);
    assert(buf.size() > valid_bytes);
    const size_t valid_samples = valid_bytes / bytes_per_sample;
    const size_t missing_samples =
        (buf.size() / bytes_per_sample) - valid_samples;

    // We only fix up to 10% missing samples
    if (missing_samples * bytes_per_sample > buf.size() / 10) {
        for (size_t i = valid_samples * bytes_per_sample; i < buf.size(); i++) {
            buf[i] = 0;
        }
    }
    else {
        const vec_u8 source_buf(buf);
        size_t source_ix = 0;

        for (size_t i = 0; i < buf.size() / bytes_per_sample; i++) {
            for (size_t j = 0; j < bytes_per_sample; j++) {
                buf.at(bytes_per_sample*i + j) = source_buf.at(source_ix + j);
            }

            // Do not advance the source index if the sample index is
            // at the spots where we want to duplicate the source sample
            if (not (i > 0 and (i % (valid_samples / missing_samples) == 0))) {
                source_ix += bytes_per_sample;
            }
        }
    }
}

/*! Wait the proper amount of time to throttle down to nominal encoding
 * rate, if drift compensation is enabled.
 */
static void drift_compensation_delay(int sample_rate, int channels, size_t bytes)
{
    const size_t bytes_per_second = sample_rate * BYTES_PER_SAMPLE * channels;

    size_t bytes_compensate = bytes;
    const auto wait_time = std::chrono::milliseconds(1000ul * bytes_compensate / bytes_per_second);
    assert(1000ul * bytes_compensate % bytes_per_second == 0);

    const auto curTime = std::chrono::steady_clock::now();

    const auto diff = curTime - timepoint_last_compensation;

    if (diff < wait_time) {
        auto waiting = wait_time - diff;
        std::this_thread::sleep_for(waiting);
    }

    timepoint_last_compensation += wait_time;
}

#define no_argument 0
#define required_argument 1
#define optional_argument 2

#define STATUS_PAD_INSERTED 0x1
#define STATUS_OVERRUN 0x2
#define STATUS_UNDERRUN 0x4

int main(int argc, char *argv[])
{
    int bitrate = 0; // 0 is default
    int ch=0;

    encoder_selection_t selected_encoder = encoder_selection_t::fdk_dabplus;

    // For the ALSA input
    const char *alsa_device = NULL;

    // For the file input
    const char *infile = NULL;
    int raw_input = 0;

    // For the VLC input
    std::string vlc_uri = "";
    std::string vlc_icytext_file = "";
    bool vlc_icytext_dlplus = false;
    std::string vlc_gain = "";
    std::string vlc_cache = "";
    std::vector<std::string> vlc_additional_opts;
    unsigned verbosity = 0;

    // For the file output
    FILE *out_fh = NULL;

    const char *jack_name = NULL;

    std::vector<std::string> output_uris;

    int sample_rate=48000, channels=2;
    void *rs_handler = NULL;
    bool afterburner = true;
    bool inFifoSilence = false;
    bool drift_compensation = false;
    AACENC_InfoStruct info = { 0 };
    int aot = AOT_NONE;

    char dab_channel_mode = '\0';
    int  dab_psy_model = 1;
    std::deque<uint8_t> toolame_output_buffer;

    /* Keep track of peaks */
    int peak_left  = 0;
    int peak_right = 0;

    /* On silence, die after the silence_timeout expires */
    bool die_on_silence = false;
    int silence_timeout = 0;
    int measured_silence_ms = 0;

    /* For MOT Slideshow and DLS insertion */
    const char* pad_fifo = "/tmp/pad.fifo";
    int pad_fd;
    int padlen = 0;

    /* Encoder status, see the above STATUS macros */
    int status = 0;

    /* Whether to show the 'sox'-like measurement */
    int show_level = 0;

    /* Data for ZMQ CURVE authentication */
    char* keyfile = NULL;
    char secretkey[CURVE_KEYLEN+1];

    const struct option longopts[] = {
        {"bitrate",                required_argument,  0, 'b'},
        {"channels",               required_argument,  0, 'c'},
        {"dabmode",                required_argument,  0,  4 },
        {"dabpsy",                 required_argument,  0,  5 },
        {"device",                 required_argument,  0, 'd'},
        {"format",                 required_argument,  0, 'f'},
        {"input",                  required_argument,  0, 'i'},
        {"jack",                   required_argument,  0, 'j'},
        {"output",                 required_argument,  0, 'o'},
        {"pad",                    required_argument,  0, 'p'},
        {"pad-fifo",               required_argument,  0, 'P'},
        {"rate",                   required_argument,  0, 'r'},
        {"secret-key",             required_argument,  0, 'k'},
        {"silence",                required_argument,  0, 's'},
        {"vlc-cache",              required_argument,  0, 'C'},
        {"vlc-gain",               required_argument,  0, 'g'},
        {"vlc-uri",                required_argument,  0, 'v'},
        {"vlc-opt",                required_argument,  0, 'L'},
        {"write-icy-text",         required_argument,  0, 'w'},
        {"write-icy-text-dl-plus", no_argument,        0, 'W'},
        {"aaclc",                  no_argument,        0,  0 },
        {"dab",                    no_argument,        0, 'a'},
        {"drift-comp",             no_argument,        0, 'D'},
        {"fifo-silence",           no_argument,        0,  3 },
        {"help",                   no_argument,        0, 'h'},
        {"level",                  no_argument,        0, 'l'},
        {"no-afterburner",         no_argument,        0, 'A'},
        {"ps",                     no_argument,        0,  2 },
        {"sbr",                    no_argument,        0,  1 },
        {"verbosity",              no_argument,        0, 'V'},
        {0, 0, 0, 0},
    };

    fprintf(stderr,
            "Welcome to %s %s, compiled at %s, %s",
            PACKAGE_NAME,
#if defined(GITVERSION)
            GITVERSION,
#else
            PACKAGE_VERSION,
#endif
            __DATE__, __TIME__);
    fprintf(stderr, "\n");
    fprintf(stderr, "  http://opendigitalradio.org\n\n");


    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    int index;
    while(ch != -1) {
        ch = getopt_long(argc, argv, "aAhDlVb:c:f:i:j:k:L:o:r:d:p:P:s:v:w:Wg:C:", longopts, &index);
        switch (ch) {
        case 0: // AAC-LC
            aot = AOT_DABPLUS_AAC_LC;
            break;
        case 1: // SBR
            aot = AOT_DABPLUS_SBR;
            break;
        case 2: // PS
            aot = AOT_DABPLUS_PS;
            break;
        case 3: // FIFO SILENCE
        case 4: // DAB channel mode
            dab_channel_mode = optarg[0];
            break;
        case 5: // DAB psy model
            dab_psy_model = atoi(optarg);
            break;
        case 'a':
            selected_encoder = encoder_selection_t::toolame_dab;
            break;
        case 'A':
            afterburner = false;
            break;
        case 'b':
            bitrate = atoi(optarg);
            break;
        case 'c':
            channels = atoi(optarg);
            break;
        case 'd':
            alsa_device = optarg;
            break;
        case 'D':
            drift_compensation = true;
            break;
        case 'f':
            if(strcmp(optarg, "raw")==0) {
                raw_input = 1;
            } else if(strcmp(optarg, "wav")!=0)
                usage(argv[0]);
            break;
        case 'i':
            infile = optarg;
            break;
        case 'j':
#if HAVE_JACK
            jack_name = optarg;
#else
            fprintf(stderr, "JACK disabled at compile time!\n");
            return 1;
#endif
            break;
        case 'k':
            keyfile = optarg;
            break;
        case 'l':
            show_level = 1;
            break;
        case 'o':
            output_uris.push_back(optarg);
            break;
        case 'p':
            padlen = atoi(optarg);
            break;
        case 'P':
            pad_fifo = optarg;
            break;
        case 'r':
            sample_rate = atoi(optarg);
            break;
        case 's':
            silence_timeout = atoi(optarg);
            if (silence_timeout > 0 && silence_timeout < 3600*24*30) {
                die_on_silence = true;
            }
            else {
                fprintf(stderr, "Invalid silence timeout (%d) given!\n", silence_timeout);
                return 1;
            }

            break;
#ifdef HAVE_VLC
        case 'v':
            vlc_uri = optarg;
            break;
        case 'w':
            vlc_icytext_file = optarg;
            break;
        case 'W':
            vlc_icytext_dlplus = true;
            break;
        case 'g':
            vlc_gain = optarg;
            break;
        case 'C':
            vlc_cache = optarg;
            break;
        case 'L':
            vlc_additional_opts.push_back(optarg);
            break;
#else
        case 'v':
        case 'w':
            fprintf(stderr, "VLC input not enabled at compile time!\n");
            return 1;
#endif
        case 'V':
            verbosity++;
            break;
        case '?':
        case 'h':
            usage(argv[0]);
            return 1;
        }
    }

    int num_inputs = 0;
    if (alsa_device) num_inputs++;
    if (infile) num_inputs++;
    if (jack_name) num_inputs++;
    if (vlc_uri != "") num_inputs++;

    if (num_inputs > 1) {
        fprintf(stderr, "You must define only one possible input, not several!\n");
        return 1;
    }

    if (selected_encoder == encoder_selection_t::fdk_dabplus) {
        if (bitrate == 0) {
            bitrate = 64;
        }

        int subchannel_index = bitrate / 8;

        if (subchannel_index < 1 || subchannel_index > 24) {
            fprintf(stderr, "Bad subchannel index: %d, must be between 1 and 24. Try other bitrate.\n",
                    subchannel_index);
            return 1;
        }

        if ( ! (sample_rate == 32000 || sample_rate == 48000)) {
            fprintf(stderr, "Invalid sample rate. Possible values are: 32000, 48000.\n");
            return 1;
        }
    }
    else if (selected_encoder == encoder_selection_t::toolame_dab) {
        if (bitrate == 0) {
            bitrate = 192;
        }

        if ( ! (sample_rate == 24000 || sample_rate == 48000)) {
            fprintf(stderr, "Invalid sample rate. Possible values are: 24000, 48000.\n");
            return 1;
        }
    }

    if (padlen < 0) {
        fprintf(stderr, "Invalid PAD length specified\n");
        return 1;
    }

    zmq::context_t zmq_ctx;
    zmq::socket_t zmq_sock(zmq_ctx, ZMQ_PUB);

    if (not output_uris.empty()) {
        for (auto uri : output_uris) {
            if (uri == "-") {
                if (out_fh != NULL) {
                    fprintf(stderr, "You can't write to more than one file!\n");
                    return 1;
                }
                out_fh = stdout;
            }
            else if ((uri.compare(0, 6, "tcp://") == 0) ||
                    (uri.compare(0, 6, "pgm://") == 0) ||
                    (uri.compare(0, 7, "epgm://") == 0)) {
                if (keyfile) {
                    fprintf(stderr, "Enabling encryption\n");

                    int rc = readkey(keyfile, secretkey);
                    if (rc) {
                        fprintf(stderr, "Error reading secret key\n");
                        return 2;
                    }

                    const int yes = 1;
                    zmq_sock.setsockopt(ZMQ_CURVE_SERVER,
                            &yes, sizeof(yes));

                    zmq_sock.setsockopt(ZMQ_CURVE_SECRETKEY,
                            secretkey, CURVE_KEYLEN);
                }
                zmq_sock.connect(uri.c_str());
            }
            else { // We assume it's a file name
                if (out_fh != NULL) {
                    fprintf(stderr, "You can't write to more than one file!\n");
                    return 1;
                }

                out_fh = fopen(uri.c_str(), "wb");

                if (!out_fh) {
                    fprintf(stderr, "Can't open output file!\n");
                    return 1;
                }
            }
        }
    }
    else {
        fprintf(stderr, "No output URI defined\n");
        return 1;
    }

    if (padlen != 0) {
        int flags;
        if (mkfifo(pad_fifo, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH) != 0) {
            if (errno != EEXIST) {
                fprintf(stderr, "Can't create pad file: %d!\n", errno);
                return 1;
            }
        }
        pad_fd = open(pad_fifo, O_RDONLY | O_NONBLOCK);
        if (pad_fd == -1) {
            fprintf(stderr, "Can't open pad file!\n");
            return 1;
        }
        flags = fcntl(pad_fd, F_GETFL, 0);
        if (fcntl(pad_fd, F_SETFL, flags | O_NONBLOCK)) {
            fprintf(stderr, "Can't set non-blocking mode in pad file!\n");
            return 1;
        }
    }


    vec_u8 input_buf;

    HANDLE_AACENCODER encoder;

    if (selected_encoder == encoder_selection_t::fdk_dabplus) {
        int subchannel_index = bitrate / 8;
        if (prepare_aac_encoder(&encoder, subchannel_index, channels,
                    sample_rate, afterburner, &aot) != 0) {
            fprintf(stderr, "Encoder preparation failed\n");
            return 1;
        }

        if (aacEncInfo(encoder, &info) != AACENC_OK) {
            fprintf(stderr, "Unable to get the encoder info\n");
            return 1;
        }

        // Each DAB+ frame will need input_size audio bytes
        const int input_size = channels * BYTES_PER_SAMPLE * info.frameLength;
        fprintf(stderr, "DAB+ Encoding: framelen=%d (%dB)\n",
                info.frameLength,
                input_size);

        input_buf.resize(input_size);
    }
    else if (selected_encoder == encoder_selection_t::toolame_dab) {
        int err = toolame_init();

        if (err == 0) {
            err = toolame_set_samplerate(sample_rate);
        }

        if (err == 0) {
            err = toolame_set_bitrate(bitrate);
        }

        if (err == 0) {
            err = toolame_set_psy_model(dab_psy_model);
        }

        if (dab_channel_mode == '\0') {
            if (channels == 2) {
                dab_channel_mode = 'j'; // Default to joint-stereo
            }
            else if (channels == 1) {
                dab_channel_mode = 'm'; // Default to mono
            }
            else {
                fprintf(stderr, "Unsupported channels number %d\n", channels);
                return 1;
            }
        }

        if (err == 0) {
            err = toolame_set_channel_mode(dab_channel_mode);
        }

        if (err == 0) {
            err = toolame_set_pad(padlen);
        }

        if (err) {
            fprintf(stderr, "libtoolame-dab init failed: %d\n", err);
            return err;
        }

        input_buf.resize(channels * 1152 * BYTES_PER_SAMPLE);
    }

    /* We assume that we need to call the encoder
     * enc_calls_per_output before it gives us one encoded audio
     * frame. This information is used when the alsa drift compensation
     * is active. This is only valid for FDK-AAC.
     */
    const int enc_calls_per_output = (aot == AOT_DABPLUS_AAC_LC) ?
        sample_rate / 8000 :
        sample_rate / 16000;

    int max_size = 32*input_buf.size() + NUM_SAMPLES_PER_CALL;

    /*! The SampleQueue \c queue is given to the inputs, so that they
     * can fill it.
     */
    SampleQueue<uint8_t> queue(BYTES_PER_SAMPLE, channels, max_size);

    /* symsize=8, gfpoly=0x11d, fcr=0, prim=1, nroots=10, pad=135 */
    rs_handler = init_rs_char(8, 0x11d, 0, 1, 10, 135);
    if (rs_handler == NULL) {
        perror("init_rs_char failed");
        return 1;
    }

    /* No input defined ? default to alsa "default" */
    if (!alsa_device) {
        alsa_device = "default";
    }

    // We'll use one of the tree possible inputs
#if HAVE_ALSA
    AlsaInputThreaded alsa_in_threaded(alsa_device, channels, sample_rate, queue);
    AlsaInputDirect   alsa_in_direct(alsa_device, channels, sample_rate);
#endif
    FileInput         file_in(infile, raw_input, sample_rate);
#if HAVE_JACK
    JackInput         jack_in(jack_name, channels, sample_rate, queue);
#endif
#if HAVE_VLC
    VLCInput          vlc_input(vlc_uri, sample_rate, channels, verbosity, vlc_gain, vlc_cache, vlc_additional_opts, queue);
#endif

    if (infile) {
        if (file_in.prepare() != 0) {
            fprintf(stderr, "File input preparation failed\n");
            return 1;
        }
    }
#if HAVE_JACK
    else if (jack_name) {
        if (jack_in.prepare() != 0) {
            fprintf(stderr, "JACK preparation failed\n");
            return 1;
        }
    }
#endif
#if HAVE_VLC
    else if (vlc_uri != "") {
        if (vlc_input.prepare() != 0) {
            fprintf(stderr, "VLC with drift compensation: preparation failed\n");
            return 1;
        }

        fprintf(stderr, "Start VLC thread\n");
        vlc_input.start();
    }
#endif
#if HAVE_ALSA
    else if (drift_compensation) {
        if (alsa_in_threaded.prepare() != 0) {
            fprintf(stderr, "Alsa with drift compensation: preparation failed\n");
            return 1;
        }

        fprintf(stderr, "Start ALSA capture thread\n");
        alsa_in_threaded.start();
    }
    else {
        if (alsa_in_direct.prepare() != 0) {
            fprintf(stderr, "Alsa preparation failed\n");
            return 1;
        }
    }
#else
    else {
        fprintf(stderr, "No input defined\n");
        return 1;
    }
#endif

    int outbuf_size;
    vec_u8 zmqframebuf;
    vec_u8 outbuf;
    if (selected_encoder == encoder_selection_t::fdk_dabplus) {
        outbuf_size = bitrate/8*120;
        outbuf.resize(24*120);
        zmqframebuf.resize(ZMQ_HEADER_SIZE + 24*120);

        if(outbuf_size % 5 != 0) {
            fprintf(stderr, "Warning: (outbuf_size mod 5) = %d\n", outbuf_size % 5);
        }
    }
    else if (selected_encoder == encoder_selection_t::toolame_dab) {
        outbuf_size = 4092;
        outbuf.resize(outbuf_size);
        fprintf(stderr, "Setting outbuf size to %zu\n", outbuf.size());

        // ODR-DabMux expects frames of length 3*bitrate
        zmqframebuf.resize(ZMQ_HEADER_SIZE + 3 * bitrate);
    }

    zmq_frame_header_t *zmq_frame_header = (zmq_frame_header_t*)&zmqframebuf[0];

    unsigned char pad_buf[padlen + 1];

    fprintf(stderr, "Starting encoding\n");

    int retval = 0;
    int send_error_count = 0;
    timepoint_last_compensation = chrono::steady_clock::now();

    int calls = 0; // for checking
    ssize_t read_bytes = 0;
    do {
        AACENC_BufDesc in_buf = { 0 }, out_buf = { 0 };

        // --------------- Read data from the PAD fifo
        int ret;
        if (padlen != 0) {
            ret = read(pad_fd, pad_buf, padlen + 1);
        }
        else {
            ret = 0;
        }


        if(ret < 0 && errno == EAGAIN) {
            // If this condition passes, there is no data to be read
            in_buf.numBufs = 1;    // Samples;
        }
        else if(ret >= 0) {
            // Otherwise, you're good to go and buffer should contain "count" bytes.
            in_buf.numBufs = 2;    // Samples + Data;
            if (ret > 0)
                status |= STATUS_PAD_INSERTED;
        }
        else {
            // Some other error occurred during read.
            fprintf(stderr, "Unable to read from PAD!\n");
                        break;
        }

        // -------------- Read Data
        memset(&outbuf[0], 0x00, outbuf_size);
        memset(&input_buf[0], 0x00, input_buf.size());

        /*! \section DataInput
         * We read data input either in a blocking way (file input, VLC or ALSA
         * without drift compensation) or in a non-blocking way (VLC or ALSA
         * with drift compensation, JACK).
         *
         * The file input doesn't need the queue at all. But the other inputs
         * do, and either use \c pop() or \c pop_wait() depending on if it's blocking or not
         *
         * In non-blocking, the \c queue makes the data available without delay, and the
         * \c drift_compensation_delay() function handles rate throttling.
         */

        if (infile) {
            read_bytes = file_in.read(&input_buf[0], input_buf.size());
            if (read_bytes < 0) {
                break;
            }
            else if (read_bytes != input_buf.size()) {
                if (inFifoSilence && file_in.eof()) {
                    memset(&input_buf[0], 0, input_buf.size());
                    read_bytes = input_buf.size();
                    usleep((long)input_buf.size() * 1000000 /
                            (BYTES_PER_SAMPLE * channels * sample_rate));
                }
                else {
                    fprintf(stderr, "Short file read !\n");
                    read_bytes = 0;
                }
            }
        }
#if HAVE_VLC
        else if (not vlc_uri.empty()) {

            if (drift_compensation && vlc_input.fault_detected()) {
                fprintf(stderr, "Detected fault in VLC input!\n");
                retval = 5;
                break;
            }

            if (drift_compensation) {
                size_t overruns;
                size_t bytes_from_queue = queue.pop(&input_buf[0], input_buf.size(), &overruns); // returns bytes
                if (bytes_from_queue != input_buf.size()) {
                    expand_missing_samples(input_buf, channels, bytes_from_queue);
                }
                read_bytes = input_buf.size();
                drift_compensation_delay(sample_rate, channels, read_bytes);

                if (bytes_from_queue != input_buf.size()) {
                    status |= STATUS_UNDERRUN;
                }

                if (overruns) {
                    status |= STATUS_OVERRUN;
                }
            }
            else {
                const int timeout_ms = 10000;
                read_bytes = input_buf.size();

                /*! pop_wait() must return after a timeout, otherwise the silence detector cannot do
                 * its job.
                 */
                size_t bytes_from_queue = queue.pop_wait(&input_buf[0], read_bytes, timeout_ms); // returns bytes

                if (bytes_from_queue < read_bytes) {
                    // queue timeout occurred
                    fprintf(stderr, "Detected fault in VLC input! No data in time.\n");
                    retval = 5;
                    break;
                }
            }

            if (not vlc_icytext_file.empty()) {
                vlc_input.write_icy_text(vlc_icytext_file, vlc_icytext_dlplus);
            }
        }
#endif
        else if (drift_compensation || jack_name) {
#if HAVE_ALSA
            if (drift_compensation && alsa_in_threaded.fault_detected()) {
                fprintf(stderr, "Detected fault in alsa input!\n");
                retval = 5;
                break;
            }
#endif

            size_t overruns;
            size_t bytes_from_queue = queue.pop(&input_buf[0], input_buf.size(), &overruns); // returns bytes
            if (bytes_from_queue != input_buf.size()) {
                expand_missing_samples(input_buf, channels, bytes_from_queue);
            }
            read_bytes = input_buf.size();
            drift_compensation_delay(sample_rate, channels, read_bytes);

            if (bytes_from_queue != input_buf.size()) {
                status |= STATUS_UNDERRUN;
            }

            if (overruns) {
                status |= STATUS_OVERRUN;
            }
        }
        else {
#if HAVE_ALSA
            read_bytes = alsa_in_direct.read(&input_buf[0], input_buf.size());
            if (read_bytes < 0) {
                break;
            }
            else if (read_bytes != input_buf.size()) {
                fprintf(stderr, "Short alsa read !\n");
            }
#endif
        }

        /*! \section AudioLevel
         * Audio level measurement is always done assuming we have two
         * channels, and is formally wrong in mono, but still gives
         * numbers one can use.
         *
         * \todo fix level measurement in mono
         */
        for (int i = 0; i < read_bytes; i+=4) {
            int16_t l = input_buf[i] | (input_buf[i+1] << 8);
            int16_t r = input_buf[i+2] | (input_buf[i+3] << 8);
            peak_left  = MAX(peak_left,  l);
            peak_right = MAX(peak_right, r);
        }

        /*! \section SilenceDetection
         * Silence detection looks at the audio level and is
         * only useful if the connection dropped, or if no data is available. It is not
         * useful if the source is nearly silent (some noise present), because the
         * threshold is 0, and not configurable. The rationale is that we want to
         * guard against connection issues, not source level issues
         */
        if (die_on_silence && MAX(peak_left, peak_right) == 0) {
            const unsigned int frame_time_msec = 1000ul *
                read_bytes / (BYTES_PER_SAMPLE * channels * sample_rate);

            measured_silence_ms += frame_time_msec;

            if (measured_silence_ms > 1000*silence_timeout) {
                fprintf(stderr, "Silence detected for %d seconds, aborting.\n",
                        silence_timeout);
                retval = 2;
                break;
            }
        }
        else {
            measured_silence_ms = 0;
        }

        int numOutBytes = 0;
        if (read_bytes and
                selected_encoder == encoder_selection_t::fdk_dabplus) {
            AACENC_InArgs in_args = { 0 };
            AACENC_OutArgs out_args = { 0 };
            // -------------- AAC Encoding
            //
            int in_identifier[] = {IN_AUDIO_DATA, IN_ANCILLRY_DATA};
            int out_identifier = OUT_BITSTREAM_DATA;
            const int calculated_padlen = ret > 0 ? pad_buf[padlen] : 0;
            const int subchannel_index = bitrate / 8;

            void *in_ptr[2], *out_ptr;
            int in_size[2], in_elem_size[2];
            int out_size, out_elem_size;

            in_ptr[0] = &input_buf[0];
            in_ptr[1] = pad_buf + (padlen - calculated_padlen); // offset due to unused PAD bytes
            in_size[0] = read_bytes;
            in_size[1] = calculated_padlen;
            in_elem_size[0] = BYTES_PER_SAMPLE;
            in_elem_size[1] = sizeof(uint8_t);
            in_args.numInSamples = input_buf.size()/BYTES_PER_SAMPLE;
            in_args.numAncBytes = calculated_padlen;

            in_buf.bufs = (void**)&in_ptr;
            in_buf.bufferIdentifiers = in_identifier;
            in_buf.bufSizes = in_size;
            in_buf.bufElSizes = in_elem_size;

            out_ptr = &outbuf[0];
            out_size = outbuf.size();
            out_elem_size = 1;
            out_buf.numBufs = 1;
            out_buf.bufs = &out_ptr;
            out_buf.bufferIdentifiers = &out_identifier;
            out_buf.bufSizes = &out_size;
            out_buf.bufElSizes = &out_elem_size;

            AACENC_ERROR err;
            if ((err = aacEncEncode(encoder, &in_buf, &out_buf, &in_args, &out_args))
                    != AACENC_OK) {
                if (err == AACENC_ENCODE_EOF) {
                    fprintf(stderr, "encoder error: EOF reached\n");
                    break;
                }
                fprintf(stderr, "Encoding failed (%d)\n", err);
                retval = 3;
                break;
            }
            calls++;

            numOutBytes = out_args.numOutBytes;
        }
        else if (selected_encoder == encoder_selection_t::toolame_dab) {
            int calculated_padlen = 0;
            if (ret == padlen + 1) {
                calculated_padlen = pad_buf[padlen];
                if (calculated_padlen <= 2) {
                    stringstream ss;
                    ss << "Invalid XPAD Length " << calculated_padlen;
                    throw runtime_error(ss.str());
                }
            }

            /*! \note toolame expects the audio to be in another shape as
             * we have in input_buf, and we need to convert first
             */
            short input_buffers[2][1152];

            if (channels == 1) {
                memcpy(input_buffers[0], &input_buf[0], 1152 * BYTES_PER_SAMPLE);
            }
            else if (channels == 2) {
                for (int i = 0; i < 1152; i++) {
                    int16_t l = input_buf[4*i]   | (input_buf[4*i+1] << 8);
                    int16_t r = input_buf[4*i+2] | (input_buf[4*i+3] << 8);

                    input_buffers[0][i] = l;
                    input_buffers[1][i] = r;
                }
            }
            else {
                fprintf(stderr, "INTERNAL ERROR! invalid number of channels\n");
            }

            if (read_bytes) {
                numOutBytes = toolame_encode_frame(input_buffers, pad_buf, calculated_padlen, &outbuf[0], outbuf.size());
            }
            else {
                numOutBytes = toolame_finish(&outbuf[0], outbuf.size());
            }
        }

        /* Check if the encoder has generated output data.
         * DAB+ requires RS encoding, which is not done in ODR-DabMux and not necessary
         * for DAB.
         */
        if (numOutBytes != 0 and
            selected_encoder == encoder_selection_t::fdk_dabplus) {

            // Our timing code depends on this
            if (calls != enc_calls_per_output) {
                fprintf(stderr, "INTERNAL ERROR! calls=%d"
                        ", expected %d\n",
                        calls, enc_calls_per_output);
            }
            calls = 0;

            int row, col;
            unsigned char buf_to_rs_enc[110];
            unsigned char rs_enc[10];
            const int subchannel_index = bitrate / 8;
            for(row=0; row < subchannel_index; row++) {
                for(col=0;col < 110; col++) {
                    buf_to_rs_enc[col] = outbuf[subchannel_index * col + row];
                }

                encode_rs_char(rs_handler, buf_to_rs_enc, rs_enc);

                for(col=110; col<120; col++) {
                    outbuf[subchannel_index * col + row] = rs_enc[col-110];
                    assert(subchannel_index * col + row < outbuf_size);
                }
            }

            numOutBytes = outbuf_size;
        }

        if (numOutBytes != 0) {
            if (out_fh) {
                fwrite(&outbuf[0], 1, numOutBytes, out_fh);
            }
            else {
                // ------------ ZeroMQ transmit
                try {
                    if (selected_encoder == encoder_selection_t::fdk_dabplus) {
                        zmq_frame_header->encoder = ZMQ_ENCODER_FDK;
                        zmq_frame_header->version = 1;
                        zmq_frame_header->datasize = numOutBytes;
                        zmq_frame_header->audiolevel_left = peak_left;
                        zmq_frame_header->audiolevel_right = peak_right;

                        assert(ZMQ_FRAME_SIZE(zmq_frame_header) <= zmqframebuf.size());

                        memcpy(ZMQ_FRAME_DATA(zmq_frame_header),
                                &outbuf[0], numOutBytes);

                        zmq_sock.send(&zmqframebuf[0], ZMQ_FRAME_SIZE(zmq_frame_header),
                                ZMQ_DONTWAIT);

                    }
                    else if (selected_encoder == encoder_selection_t::toolame_dab) {
                        toolame_output_buffer.insert(toolame_output_buffer.end(),
                                outbuf.begin(), outbuf.begin() + numOutBytes);

                        while (toolame_output_buffer.size() > 3 * bitrate) {
                            zmq_frame_header->encoder = ZMQ_ENCODER_TOOLAME;
                            zmq_frame_header->version = 1;
                            zmq_frame_header->datasize = 3 * bitrate;
                            zmq_frame_header->audiolevel_left = peak_left;
                            zmq_frame_header->audiolevel_right = peak_right;

                            uint8_t *encoded_frame = ZMQ_FRAME_DATA(zmq_frame_header);

                            // no memcpy for std::deque
                            for (size_t i = 0; i < 3*bitrate; i++) {
                                encoded_frame[i] = toolame_output_buffer[i];
                            }

                            zmq_sock.send(&zmqframebuf[0], ZMQ_FRAME_SIZE(zmq_frame_header),
                                    ZMQ_DONTWAIT);

                            toolame_output_buffer.erase(toolame_output_buffer.begin(),
                                                        toolame_output_buffer.begin() + 3 * bitrate);
                        }
                    }
                }
                catch (zmq::error_t& e) {
                    fprintf(stderr, "ZeroMQ send error !\n");
                    send_error_count ++;
                }

                if (send_error_count > 10)
                {
                    fprintf(stderr, "ZeroMQ send failed ten times, aborting!\n");
                    retval = 4;
                    break;
                }
            }
        }

        if (numOutBytes != 0)
        {
            if (show_level) {
                if (channels == 1) {
                    fprintf(stderr, "\rIn: [%-6s] %1s %1s %1s",
                            level(1, MAX(peak_right, peak_left)),
                            status & STATUS_PAD_INSERTED ? "P" : " ",
                            status & STATUS_UNDERRUN ? "U" : " ",
                            status & STATUS_OVERRUN ? "O" : " ");
                }
                else if (channels == 2) {
                    fprintf(stderr, "\rIn: [%6s|%-6s] %1s %1s %1s",
                            level(0, peak_left),
                            level(1, peak_right),
                            status & STATUS_PAD_INSERTED ? "P" : " ",
                            status & STATUS_UNDERRUN ? "U" : " ",
                            status & STATUS_OVERRUN ? "O" : " ");
                }
            }
            else {
                if (status & STATUS_OVERRUN) {
                    fprintf(stderr, "O");
                }

                if (status & STATUS_UNDERRUN) {
                    fprintf(stderr, "U");
                }

            }

            peak_right = 0;
            peak_left = 0;

            status = 0;
        }

        fflush(stdout);
    } while (read_bytes > 0);

    fprintf(stderr, "\n");

    if (out_fh) {
        fclose(out_fh);
    }

    zmq_sock.close();
    free_rs_char(rs_handler);

    if (selected_encoder == encoder_selection_t::fdk_dabplus) {
        aacEncClose(&encoder);
    }

    return retval;
}

