# Copyright 2020 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Check if there is C++ code at all
########################################################################
if(NOT dab_sources)
    MESSAGE(STATUS "No C++ sources... skipping python bindings")
    return()
endif(NOT dab_sources)

########################################################################
# Check for pygccxml
########################################################################
GR_PYTHON_CHECK_MODULE_RAW(
    "pygccxml"
    "import pygccxml"
    PYGCCXML_FOUND
    )

include(GrPybind)

########################################################################
# Python Bindings
########################################################################
list(APPEND dab_python_files
    conv_encoder_bb_python.cc
    crc16_bb_python.cc
    dab_transmission_frame_mux_bb_python.cc
    demux_cc_python.cc
    firecode_check_bb_python.cc
    insert_null_symbol_python.cc
    mp2_decode_bs_python.cc
    mp2_encode_sb_python.cc
    mp4_decode_bs_python.cc
    mp4_encode_sb_python.cc
    ofdm_insert_pilot_vcc_python.cc
    ofdm_synchronization_cvf_python.cc
    prune_python.cc
    puncture_bb_python.cc
    reed_solomon_decode_bb_python.cc
    reed_solomon_encode_bb_python.cc
    select_cus_vfvf_python.cc
    valve_ff_python.cc
    complex_to_interleaved_float_vcf_python.cc
    diff_phasor_vcc_python.cc
    fib_sink_vb_python.cc
    fib_source_b_python.cc
    frequency_interleaver_vcc_python.cc
    ofdm_coarse_frequency_correction_vcvc_python.cc
    ofdm_move_and_insert_zero_python.cc
    qpsk_mapper_vbvc_python.cc
    sum_phasor_trig_vcc_python.cc
    time_deinterleave_ff_python.cc
    time_interleave_bb_python.cc
    unpuncture_vff_python.cc python_bindings.cc)

GR_PYBIND_MAKE_OOT(dab
   ../../..
   gr::dab
   "${dab_python_files}")

# copy bindings extension for use in QA test module
add_custom_command(TARGET dab_python POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:dab_python>
    ${PROJECT_BINARY_DIR}/test_modules/gnuradio/dab/
)

install(TARGETS dab_python DESTINATION ${GR_PYTHON_DIR}/gnuradio/dab COMPONENT pythonapi)
