/* -*- c++ -*- */
/*
 * Copyright 2017 by Moritz Luca Schmid, Communications Engineering Lab (CEL) / Karlsruhe Institute of Technology (KIT).
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

#ifndef INCLUDED_DAB_TIME_DEINTERLEAVE_FF_IMPL_H
#define INCLUDED_DAB_TIME_DEINTERLEAVE_FF_IMPL_H

#include <gnuradio/dab/time_deinterleave_ff.h>

namespace gr {
  namespace dab {
/*! \brief Applies time deinterleaving to a vector
 * convolutional deinterleaving -> descrambling and delay
 *
 * Applies convolutional deinterleaving to a vector with its max[vector_length] followers,
 * the scrambling_vector describes which vector element comes from which follower.
 * Delays the elements of a max delay of d_scrambling_length-1.
 * More information to the interleaving rules on ETSI EN 300 401 chapter 12.
 * This deinterleaver restores a bitstream interleaved by the block time_interleave_bb.
 *
 * @param vector_length Length of the input vectors.
 * @param scrambling_vector Vector with scrambling parameters.
 * (see ETSI EN 300 401 chapter 12)
 *
 */
    class time_deinterleave_ff_impl : public time_deinterleave_ff {

    private:
      int d_scrambling_length, d_vector_length;
      std::vector<unsigned char> d_scrambling_vector;

    public:
      time_deinterleave_ff_impl(int vector_length,
                                const std::vector<unsigned char> &scrambling_vector);

      ~time_deinterleave_ff_impl();

      // Where all the action really happens
      int work(int noutput_items,
               gr_vector_const_void_star &input_items,
               gr_vector_void_star &output_items);
    };

  } // namespace dab
} // namespace gr

#endif /* INCLUDED_DAB_TIME_DEINTERLEAVE_FF_IMPL_H */
