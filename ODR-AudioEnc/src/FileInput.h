/* ------------------------------------------------------------------
 * Copyright (C) 2016 Matthias P. Braendli
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
/*! \section File Input
 *
 * This input reads a wav or raw file.
 *
 * The raw input needs to be signed 16-bit per sample data, with
 * the number of channels corresponding to the command line.
 *
 * The wav input must also correspond to the parameters on the command
 * line (number of channels, rate)
 */

#ifndef _FILE_INPUT_H_
#define _FILE_INPUT_H_

#include <stdint.h>
#include <stdio.h>

class FileInput
{
    public:
        FileInput(const char* filename,
                bool raw_input,
                int sample_rate) :
            m_filename(filename),
            m_raw_input(raw_input),
            m_sample_rate(sample_rate),
            m_wav(nullptr) { }

        ~FileInput();

        /*! Open the file and prepare the wav decoder.
         *
         * \return nonzero on error
         */
        int prepare(void);

        /*! Read length bytes into buf.
         *
         * \return the number of bytes read.
         */
        ssize_t read(uint8_t* buf, size_t length);
        int eof();

    protected:
        const char* m_filename;
        bool m_raw_input;
        int m_sample_rate;

        /* handle to the wav reader */
        void *m_wav;

        FILE* m_in_fh;
};

#endif

