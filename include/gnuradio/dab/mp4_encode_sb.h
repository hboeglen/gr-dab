/* -*- c++ -*- */
/* 
 * Copyright 2017 Moritz Luca Schmid, Communications Engineering Lab (CEL) / Karlsruhe Institute of Technology (KIT).
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */


#ifndef INCLUDED_DAB_MP4_ENCODE_SB_H
#define INCLUDED_DAB_MP4_ENCODE_SB_H

#include <gnuradio/dab/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace dab {

    /*!
     * \brief block to encode a PCM stream in HE-AAC with DAB+ specific parameters (960 granule length)
     * \ingroup dab
     *
     */
    class DAB_API mp4_encode_sb : virtual public gr::block
    {
     public:
      typedef std::shared_ptr<mp4_encode_sb> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of dab::mp4_encode_sb.
       *
       * To avoid accidental use of raw pointers, dab::mp4_encode_sb's
       * constructor is in a private implementation
       * class. dab::mp4_encode_sb::make is the public interface for
       * creating new instances.
       */
      static sptr make(int bit_rate_n, int channels, int samp_rate, int afterburner);
    };

  } // namespace dab
} // namespace gr

#endif /* INCLUDED_DAB_MP4_ENCODE_SB_H */

