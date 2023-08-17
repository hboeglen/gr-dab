/*
 * Copyright 2020 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <pybind11/pybind11.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

namespace py = pybind11;

// Headers for binding functions
/**************************************/
// The following comment block is used for
// gr_modtool to insert function prototypes
// Please do not delete
/**************************************/
// BINDING_FUNCTION_PROTOTYPES(
    void bind_conv_encoder_bb(py::module& m);
    void bind_crc16_bb(py::module& m);
    void bind_dab_transmission_frame_mux_bb(py::module& m);
    void bind_demux_cc(py::module& m);
    void bind_firecode_check_bb(py::module& m);
    void bind_insert_null_symbol(py::module& m);
    void bind_mp2_decode_bs(py::module& m);
    void bind_mp2_encode_sb(py::module& m);
    void bind_mp4_decode_bs(py::module& m);
    void bind_mp4_encode_sb(py::module& m);
    void bind_ofdm_insert_pilot_vcc(py::module& m);
    void bind_ofdm_synchronization_cvf(py::module& m);
    void bind_prune(py::module& m);
    void bind_puncture_bb(py::module& m);
    void bind_reed_solomon_decode_bb(py::module& m);
    void bind_reed_solomon_encode_bb(py::module& m);
    void bind_select_cus_vfvf(py::module& m);
    void bind_valve_ff(py::module& m);
    void bind_complex_to_interleaved_float_vcf(py::module& m);
    void bind_diff_phasor_vcc(py::module& m);
    void bind_fib_sink_vb(py::module& m);
    void bind_fib_source_b(py::module& m);
    void bind_frequency_interleaver_vcc(py::module& m);
    void bind_ofdm_coarse_frequency_correction_vcvc(py::module& m);
    void bind_ofdm_move_and_insert_zero(py::module& m);
    void bind_qpsk_mapper_vbvc(py::module& m);
    void bind_sum_phasor_trig_vcc(py::module& m);
    void bind_time_deinterleave_ff(py::module& m);
    void bind_time_interleave_bb(py::module& m);
    void bind_unpuncture_vff(py::module& m);
// ) END BINDING_FUNCTION_PROTOTYPES


// We need this hack because import_array() returns NULL
// for newer Python versions.
// This function is also necessary because it ensures access to the C API
// and removes a warning.
void* init_numpy()
{
    import_array();
    return NULL;
}

PYBIND11_MODULE(dab_python, m)
{
    // Initialize the numpy C API
    // (otherwise we will see segmentation faults)
    init_numpy();

    // Allow access to base block methods
    py::module::import("gnuradio.gr");

    /**************************************/
    // The following comment block is used for
    // gr_modtool to insert binding function calls
    // Please do not delete
    /**************************************/
    // BINDING_FUNCTION_CALLS(
    bind_conv_encoder_bb(m);
    bind_crc16_bb(m);
    bind_dab_transmission_frame_mux_bb(m);
    bind_demux_cc(m);
    bind_firecode_check_bb(m);
    bind_insert_null_symbol(m);
    bind_mp2_decode_bs(m);
    bind_mp2_encode_sb(m);
    bind_mp4_decode_bs(m);
    bind_mp4_encode_sb(m);
    bind_ofdm_insert_pilot_vcc(m);
    bind_ofdm_synchronization_cvf(m);
    bind_prune(m);
    bind_puncture_bb(m);
    bind_reed_solomon_decode_bb(m);
    bind_reed_solomon_encode_bb(m);
    bind_select_cus_vfvf(m);
    bind_valve_ff(m);
    bind_complex_to_interleaved_float_vcf(m);
    bind_diff_phasor_vcc(m);
    bind_fib_sink_vb(m);
    bind_fib_source_b(m);
    bind_frequency_interleaver_vcc(m);
    bind_ofdm_coarse_frequency_correction_vcvc(m);
    bind_ofdm_move_and_insert_zero(m);
    bind_qpsk_mapper_vbvc(m);
    bind_sum_phasor_trig_vcc(m);
    bind_time_deinterleave_ff(m);
    bind_time_interleave_bb(m);
    bind_unpuncture_vff(m);
    // ) END BINDING_FUNCTION_CALLS
}