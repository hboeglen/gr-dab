/* -*- c++ -*- */
/*
 * Copyright 2004 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#ifndef INCLUDED_DAB_DIFF_PHASOR_VCC_IMPL_H
#define INCLUDED_DAB_DIFF_PHASOR_VCC_IMPL_H

#include <gnuradio/dab/diff_phasor_vcc.h>

namespace gr {
  namespace dab {
/*! \brief vector wise working differential phasor calculation
 * this block has the same functionality as the gr-digital diff_phasor_cc block,
 * but is working on vector basis.
 */
    class diff_phasor_vcc_impl : public diff_phasor_vcc {
    private:
      unsigned int d_length; /*!< Length of each complex vector. */

    public:
      diff_phasor_vcc_impl(unsigned int length);

      int work(int noutput_items,
               gr_vector_const_void_star &input_items,
               gr_vector_void_star &output_items);
    };

  }
}

#endif /* INCLUDED_DAB_DIFF_PHASOR_VCC_H */
