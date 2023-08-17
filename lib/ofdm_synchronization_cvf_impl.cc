/* -*- c++ -*- */
/*
 * Copyright 2017, 2018, 2022 Moritz Luca Schmid, Communications Engineering Lab (CEL)
 * Karlsruhe Institute of Technology (KIT).
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ofdm_synchronization_cvf_impl.h"
#include <gnuradio/io_signature.h>
#include <boost/format.hpp>
#include <volk/volk.h>
#include <complex>
#include <sstream>
#include <string>
#include <cstdio>
#include <cmath>

using namespace boost;

namespace gr {
  namespace dab {

    ofdm_synchronization_cvf::sptr
    ofdm_synchronization_cvf::make(int fft_length, int cyclic_prefix_length,
                                   int null_symbol_length, int symbols_per_frame) {
      return gnuradio::get_initial_sptr(new ofdm_synchronization_cvf_impl(fft_length,
                                                                          cyclic_prefix_length,
                                                                          null_symbol_length,
                                                                          symbols_per_frame));
    }

    /*
     * The private constructor
     */
    ofdm_synchronization_cvf_impl::ofdm_synchronization_cvf_impl(
            int fft_length, 
            int cyclic_prefix_length,
            int null_symbol_length,
            int symbols_per_frame)
            : gr::block("ofdm_synchronization_cvf",
                        gr::io_signature::make(1, 1, sizeof(gr_complex)),
                        gr::io_signature::make(1, 1, sizeof(gr_complex)*fft_length)),
              d_fft_length(fft_length),
              d_cyclic_prefix_length(cyclic_prefix_length),
              d_symbols_per_frame(symbols_per_frame),
              d_correlation(0),
              d_energy_prefix(1),
              d_energy_repetition(1),
              d_frequency_offset_per_sample(0),
              d_NULL_detected(false),
              d_moving_average_counter(0),
              d_symbol_count(0),
              d_on_triangle(false),
              d_phase(gr_complex(1,0)),
              d_tracking(false),
              d_correlation_maximum(0){
      //allocation for repeating energy measurements/correlation
      unsigned int alignment = volk_get_alignment();
      d_mag_squared = (float *) volk_malloc(sizeof(float) * d_cyclic_prefix_length, alignment);
      this->set_output_multiple(d_fft_length+d_cyclic_prefix_length);
    }

    /*
     * Our virtual destructor.
     */
    ofdm_synchronization_cvf_impl::~ofdm_synchronization_cvf_impl() {
    }

    void
    ofdm_synchronization_cvf_impl::forecast(int noutput_items, gr_vector_int &ninput_items_required) {
      ninput_items_required[0] = noutput_items + d_fft_length + d_cyclic_prefix_length + 1;
    }

    void
    ofdm_synchronization_cvf_impl::delayed_correlation(const gr_complex *sample, bool new_calculation) {
      if (d_moving_average_counter > 100000 || d_moving_average_counter == 0 || new_calculation) {
        //calculate delayed correlation for this sample completely
        volk_32fc_x2_conjugate_dot_prod_32fc(&d_correlation, sample,
                                             &sample[d_fft_length],
                                             d_cyclic_prefix_length);
         
        // calculate energy of cyclic prefix for this sample completely
        volk_32fc_magnitude_squared_32f(d_mag_squared, sample, d_cyclic_prefix_length);
        volk_32f_accumulator_s32f(&d_energy_prefix, d_mag_squared, d_cyclic_prefix_length);

        // calculate energy of its repetition for this sample completely
        volk_32fc_magnitude_squared_32f(d_mag_squared, &sample[d_fft_length], d_cyclic_prefix_length);
        volk_32f_accumulator_s32f(&d_energy_repetition, d_mag_squared, d_cyclic_prefix_length);
      } else {
        // calculate next step for moving average
        d_correlation += sample[d_cyclic_prefix_length - 1] * conj(sample[d_fft_length + d_cyclic_prefix_length - 1]);
        d_energy_prefix += std::real(sample[d_cyclic_prefix_length - 1] * conj(sample[d_cyclic_prefix_length - 1]));
        d_energy_repetition += std::real( sample[d_fft_length + d_cyclic_prefix_length - 1] *
                                          conj(sample[d_fft_length + d_cyclic_prefix_length - 1]));
        d_correlation -= sample[0] * conj(sample[d_fft_length]);
        d_energy_prefix -= std::real(sample[0] * conj(sample[0]));
        d_energy_repetition -= std::real(sample[d_fft_length] * conj(sample[d_fft_length]));
        d_moving_average_counter++;
      }
      // normalize
      d_correlation_normalized = d_correlation / std::sqrt(d_energy_prefix * d_energy_repetition);
      // calculate magnitude
      d_correlation_normalized_magnitude = d_correlation_normalized.real() * d_correlation_normalized.real() +
                                           d_correlation_normalized.imag() * d_correlation_normalized.imag();

    }

    /*! \brief returns true at a point with a little space before the peak of a correlation triangular
     *
     */
    bool
    ofdm_synchronization_cvf_impl::detect_start_of_symbol() {
      if (d_on_triangle) {
        if (d_correlation_normalized_magnitude > d_correlation_maximum) {
          d_correlation_maximum = d_correlation_normalized_magnitude;
        } else if (d_correlation_normalized_magnitude < d_correlation_maximum - 0.05) {
            d_on_triangle = false;
            d_correlation_maximum = 0;
          return true;
        } else if (d_correlation_normalized_magnitude < 0.25) {
          d_on_triangle = false;
          d_correlation_maximum = 0;
        } else {
          // we are still on the triangle but have not reached the post-peak
        }
      } else {
        // not on a correlation triangle yet
        if (d_correlation_normalized_magnitude > 0.35) {
          // now we are on the triangle
          d_on_triangle = true;
        } else {
          // no triangle here
        }
      }
      return false;
    }

    bool
    ofdm_synchronization_cvf_impl::detect_NULL() {
        if (d_energy_prefix / d_energy_repetition < 0.4){
            return true;
        } else {
            return false;
        }
    }

    int
    ofdm_synchronization_cvf_impl::general_work(int noutput_items,
                                                gr_vector_int &ninput_items,
                                                gr_vector_const_void_star &input_items,
                                                gr_vector_void_star &output_items) {
      // Initialize this run of work
      const gr_complex *in = (const gr_complex *) input_items[0];
      gr_complex *out = (gr_complex *) output_items[0];
      unsigned int nwritten = 0;

      if(d_tracking) {
          unsigned int num_syms = std::min(d_symbols_per_frame - d_symbol_count, noutput_items / (d_fft_length + d_cyclic_prefix_length));
          for (int sym_i = 0; sym_i < num_syms; ++sym_i) {
              // Measure frequency offset for each symbol individually.
              delayed_correlation(&in[sym_i * (d_fft_length + d_cyclic_prefix_length)], true);
              d_frequency_offset_per_sample = std::arg(d_correlation) / d_fft_length; // in rad/sample
              if (d_symbol_count == 0) {
                d_phase = gr_complex(1,0);
                this->add_item_tag(0, this->nitems_written(0) + nwritten,
                                   pmt::mp("Start"),
                                   pmt::from_float(std::arg(d_correlation)));
              }
              d_phase *= std::polar(float(1.0), static_cast<float>(d_cyclic_prefix_length * d_frequency_offset_per_sample));
              volk_32fc_s32fc_x2_rotator_32fc(
                  &out[nwritten*d_fft_length], &in[(d_cyclic_prefix_length - 100) + sym_i * (d_fft_length + d_cyclic_prefix_length)],
                  std::polar(float(1.0), d_frequency_offset_per_sample),
                  &d_phase, d_fft_length);
              nwritten++;
          }
          d_symbol_count += num_syms;
          if (d_symbol_count >= d_symbols_per_frame) {
              d_tracking = false;
              d_symbol_count = 0;
              d_NULL_detected = false;
          }
          consume_each(num_syms * (d_fft_length + d_cyclic_prefix_length));
          return nwritten;

      } else { // Acquisition mode -> we search for the start of the next OFDM frame.
        for (int i = 0; i < noutput_items; ++i) {
            delayed_correlation(&in[i], true);
            if (d_NULL_detected) { // Search for peak of fixed_lag_correlation.
                if (detect_start_of_symbol()) { // Detected start of first symbol.
                    d_tracking = true;
                    d_symbol_count = 0;
                    consume_each(i); // We want to start 100 samples before the end of the CP.
                    return 0;
                }
            } else { // Search for NULL symbol.
                if (detect_NULL()){ // Found NULL symbol.
                    d_NULL_detected = true;
                    consume_each(i);
                    return 0;
                }
            }
        }

      }
      consume_each(noutput_items);
      return 0;
        
    }

  } /* namespace dab */
} /* namespace gr */

