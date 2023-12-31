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


#ifndef INCLUDED_DAB_DAB_TRANSMISSION_FRAME_MUX_BB_H
#define INCLUDED_DAB_DAB_TRANSMISSION_FRAME_MUX_BB_H

#include <gnuradio/dab/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace dab {

    /*! \brief multiplex to DAB transmission frames
     *
     */
    class DAB_API dab_transmission_frame_mux_bb : virtual public gr::block
    {
     public:
      typedef std::shared_ptr<dab_transmission_frame_mux_bb> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of dab::dab_transmission_frame_mux_bb.
       *
       * To avoid accidental use of raw pointers, dab::dab_transmission_frame_mux_bb's
       * constructor is in a private implementation
       * class. dab::dab_transmission_frame_mux_bb::make is the public interface for
       * creating new instances.
       */
      static sptr make(int transmission_mode, int num_subch, const std::vector<unsigned int> &subch_size);
    };

  } // namespace dab
} // namespace gr

#endif /* INCLUDED_DAB_DAB_TRANSMISSION_FRAME_MUX_BB_H */

