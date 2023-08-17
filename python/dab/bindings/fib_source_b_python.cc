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
/* BINDTOOL_HEADER_FILE(fib_source_b.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(f65307e2ad25bdf3c134760bef4cef82)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/dab/fib_source_b.h>
// pydoc.h is automatically generated in the build directory
#include <fib_source_b_pydoc.h>

void bind_fib_source_b(py::module& m)
{

    using fib_source_b = ::gr::dab::fib_source_b;


    py::class_<fib_source_b,
               gr::sync_block,
               gr::block,
               gr::basic_block,
               std::shared_ptr<fib_source_b>>(m, "fib_source_b", D(fib_source_b))

        .def(py::init(&fib_source_b::make),
             py::arg("transmission_mode"),
             py::arg("coutry_ID"),
             py::arg("num_subch"),
             py::arg("ensemble_label"),
             py::arg("programme_service_labels"),
             py::arg("service_comp_label"),
             py::arg("service_comp_lang"),
             py::arg("protection_mode"),
             py::arg("data_rate_n"),
             py::arg("dabplus"),
             D(fib_source_b, make))


        ;
}