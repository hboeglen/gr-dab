/*
 * Copyright 2023 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/***********************************************************************************/
/* This file is automatically generated using bindtool and can be manually edited  */
/* The following lines can be configured to regenerate this file during cmake      */
/* If manual edits are made, the following tags should be modified accordingly.    */
/* BINDTOOL_GEN_AUTOMATIC(0)                                                       */
/* BINDTOOL_USE_PYGCCXML(0)                                                        */
/* BINDTOOL_HEADER_FILE(time_deinterleave_ff.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(b42f7e3be2c6749cec0ea054fdc6218a)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/dab/time_deinterleave_ff.h>
// pydoc.h is automatically generated in the build directory
#include <time_deinterleave_ff_pydoc.h>

void bind_time_deinterleave_ff(py::module& m)
{

    using time_deinterleave_ff = ::gr::dab::time_deinterleave_ff;


    py::class_<time_deinterleave_ff,
               gr::sync_block,
               gr::block,
               gr::basic_block,
               std::shared_ptr<time_deinterleave_ff>>(
        m, "time_deinterleave_ff", D(time_deinterleave_ff))

        .def(py::init(&time_deinterleave_ff::make),
             py::arg("vector_length"),
             py::arg("scrambling_vector"),
             D(time_deinterleave_ff, make))


        ;
}