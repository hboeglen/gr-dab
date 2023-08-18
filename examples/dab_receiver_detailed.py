#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: dab_receiver_detailed
# Author: Hervé Boeglen
# GNU Radio version: 3.10.4.0

from packaging.version import Version as StrictVersion

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print("Warning: failed to XInitThreads()")

from PyQt5 import Qt
from gnuradio import eng_notation
from gnuradio import qtgui
from gnuradio.filter import firdes
import sip
from gnuradio import audio
from gnuradio import blocks
from gnuradio import dab
from gnuradio import fft
from gnuradio.fft import window
from gnuradio import gr
import sys
import signal
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio.qtgui import Range, GrRangeWidget
from PyQt5 import QtCore
import gnuradio.dab.parameters as dp
import numpy as np
import osmosdr
import time



from gnuradio import qtgui

class dab_receiver_detailed(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "dab_receiver_detailed", catch_exceptions=True)
        Qt.QWidget.__init__(self)
        self.setWindowTitle("dab_receiver_detailed")
        qtgui.util.check_set_qss()
        try:
            self.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))
        except:
            pass
        self.top_scroll_layout = Qt.QVBoxLayout()
        self.setLayout(self.top_scroll_layout)
        self.top_scroll = Qt.QScrollArea()
        self.top_scroll.setFrameStyle(Qt.QFrame.NoFrame)
        self.top_scroll_layout.addWidget(self.top_scroll)
        self.top_scroll.setWidgetResizable(True)
        self.top_widget = Qt.QWidget()
        self.top_scroll.setWidget(self.top_widget)
        self.top_layout = Qt.QVBoxLayout(self.top_widget)
        self.top_grid_layout = Qt.QGridLayout()
        self.top_layout.addLayout(self.top_grid_layout)

        self.settings = Qt.QSettings("GNU Radio", "dab_receiver_detailed")

        try:
            if StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
                self.restoreGeometry(self.settings.value("geometry").toByteArray())
            else:
                self.restoreGeometry(self.settings.value("geometry"))
        except:
            pass

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 2048000
        self.gain_slider = gain_slider = 30
        self.freq = freq = 206352000
        self.dab_params = dab_params = dp.dab_parameters(1,samp_rate,0)

        ##################################################
        # Blocks
        ##################################################
        self._gain_slider_range = Range(0, 40, 1, 30, 200)
        self._gain_slider_win = GrRangeWidget(self._gain_slider_range, self.set_gain_slider, "Gain", "counter_slider", float, QtCore.Qt.Horizontal, "value")

        self.top_layout.addWidget(self._gain_slider_win)
        self._freq_tool_bar = Qt.QToolBar(self)
        self._freq_tool_bar.addWidget(Qt.QLabel("Frequency" + ": "))
        self._freq_line_edit = Qt.QLineEdit(str(self.freq))
        self._freq_tool_bar.addWidget(self._freq_line_edit)
        self._freq_line_edit.returnPressed.connect(
            lambda: self.set_freq(int(str(self._freq_line_edit.text()))))
        self.top_layout.addWidget(self._freq_tool_bar)
        self.rtlsdr_source_0 = osmosdr.source(
            args="numchan=" + str(1) + " " + ''
        )
        self.rtlsdr_source_0.set_time_unknown_pps(osmosdr.time_spec_t())
        self.rtlsdr_source_0.set_sample_rate(samp_rate)
        self.rtlsdr_source_0.set_center_freq(freq, 0)
        self.rtlsdr_source_0.set_freq_corr(0, 0)
        self.rtlsdr_source_0.set_dc_offset_mode(2, 0)
        self.rtlsdr_source_0.set_iq_balance_mode(0, 0)
        self.rtlsdr_source_0.set_gain_mode(False, 0)
        self.rtlsdr_source_0.set_gain(gain_slider, 0)
        self.rtlsdr_source_0.set_if_gain(20, 0)
        self.rtlsdr_source_0.set_bb_gain(20, 0)
        self.rtlsdr_source_0.set_antenna('', 0)
        self.rtlsdr_source_0.set_bandwidth(0, 0)
        self.reed_solomon_decode_bb_0 = dab.reed_solomon_decode_bb((int(88/8)))
        self.qtgui_freq_sink_x_0 = qtgui.freq_sink_c(
            dab_params.fft_length, #size
            window.WIN_BLACKMAN_hARRIS, #wintype
            0, #fc
            samp_rate, #bw
            "", #name
            1,
            None # parent
        )
        self.qtgui_freq_sink_x_0.set_update_time(0.10)
        self.qtgui_freq_sink_x_0.set_y_axis((-140), 10)
        self.qtgui_freq_sink_x_0.set_y_label('Relative Gain', 'dB')
        self.qtgui_freq_sink_x_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, 0.0, 0, "")
        self.qtgui_freq_sink_x_0.enable_autoscale(False)
        self.qtgui_freq_sink_x_0.enable_grid(False)
        self.qtgui_freq_sink_x_0.set_fft_average(1.0)
        self.qtgui_freq_sink_x_0.enable_axis_labels(True)
        self.qtgui_freq_sink_x_0.enable_control_panel(False)
        self.qtgui_freq_sink_x_0.set_fft_window_normalized(False)



        labels = ['', '', '', '', '',
            '', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
            1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "black", "cyan",
            "magenta", "yellow", "dark red", "dark green", "dark blue"]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
            1.0, 1.0, 1.0, 1.0, 1.0]

        for i in range(1):
            if len(labels[i]) == 0:
                self.qtgui_freq_sink_x_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_freq_sink_x_0.set_line_label(i, labels[i])
            self.qtgui_freq_sink_x_0.set_line_width(i, widths[i])
            self.qtgui_freq_sink_x_0.set_line_color(i, colors[i])
            self.qtgui_freq_sink_x_0.set_line_alpha(i, alphas[i])

        self._qtgui_freq_sink_x_0_win = sip.wrapinstance(self.qtgui_freq_sink_x_0.qwidget(), Qt.QWidget)
        self.top_layout.addWidget(self._qtgui_freq_sink_x_0_win)
        self.qtgui_const_sink_x_1 = qtgui.const_sink_c(
            1024, #size
            "", #name
            1, #number of inputs
            None # parent
        )
        self.qtgui_const_sink_x_1.set_update_time(0.10)
        self.qtgui_const_sink_x_1.set_y_axis((-2), 2)
        self.qtgui_const_sink_x_1.set_x_axis((-2), 2)
        self.qtgui_const_sink_x_1.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, "")
        self.qtgui_const_sink_x_1.enable_autoscale(True)
        self.qtgui_const_sink_x_1.enable_grid(False)
        self.qtgui_const_sink_x_1.enable_axis_labels(True)


        labels = ['', '', '', '', '',
            '', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
            1, 1, 1, 1, 1]
        colors = ["blue", "red", "red", "red", "red",
            "red", "red", "red", "red", "red"]
        styles = [0, 0, 0, 0, 0,
            0, 0, 0, 0, 0]
        markers = [0, 0, 0, 0, 0,
            0, 0, 0, 0, 0]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
            1.0, 1.0, 1.0, 1.0, 1.0]

        for i in range(1):
            if len(labels[i]) == 0:
                self.qtgui_const_sink_x_1.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_const_sink_x_1.set_line_label(i, labels[i])
            self.qtgui_const_sink_x_1.set_line_width(i, widths[i])
            self.qtgui_const_sink_x_1.set_line_color(i, colors[i])
            self.qtgui_const_sink_x_1.set_line_style(i, styles[i])
            self.qtgui_const_sink_x_1.set_line_marker(i, markers[i])
            self.qtgui_const_sink_x_1.set_line_alpha(i, alphas[i])

        self._qtgui_const_sink_x_1_win = sip.wrapinstance(self.qtgui_const_sink_x_1.qwidget(), Qt.QWidget)
        self.top_layout.addWidget(self._qtgui_const_sink_x_1_win)
        self.fft_vxx_0 = fft.fft_vcc(dab_params.fft_length, True, [], True, 1)
        self.dab_ofdm_synchronization_cvc_0 = dab.ofdm_synchronization_cvf(dab_params.fft_length, dab_params.cp_length, dab_params.fft_length, dab_params.symbols_per_frame)
        self.dab_ofdm_coarse_frequency_correction_vcvc_0 = dab.ofdm_coarse_frequency_correction_vcvc(dab_params.fft_length, dab_params.num_carriers, dab_params.cp_length)
        self.dab_msc_decode_0 = dab.msc_decode(dab.parameters.dab_parameters(mode=1, sample_rate=samp_rate, verbose=False), 528, 66, 2, False, False)
        self.dab_mp4_decode_bs_0 = dab.mp4_decode_bs((int(88/8)))
        self.dab_frequency_interleaver_vcc_0 = dab.frequency_interleaver_vcc(dab_params.frequency_deinterleaving_sequence_array)
        self.dab_firecode_check_bb_0 = dab.firecode_check_bb((int(88/8)))
        self.dab_diff_phasor_vcc_0 = dab.diff_phasor_vcc(1536)
        self.dab_demux_cc_0 = dab.demux_cc(dab_params.num_carriers, dab_params.num_fic_syms, dab_params.num_msc_syms, complex(0,0))
        self.blocks_vector_to_stream_0 = blocks.vector_to_stream(gr.sizeof_gr_complex*1, 1536)
        self.blocks_short_to_float_1 = blocks.short_to_float(1, 32767)
        self.blocks_short_to_float_0 = blocks.short_to_float(1, 32767)
        self.blocks_multiply_const_vxx_1 = blocks.multiply_const_vcc([1.0/np.sqrt(2048)]*dab_params.fft_length)
        self.blocks_multiply_const_vxx_0 = blocks.multiply_const_cc(2048/6)
        self.audio_sink_0 = audio.sink(48000, '', True)


        ##################################################
        # Connections
        ##################################################
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.qtgui_const_sink_x_1, 0))
        self.connect((self.blocks_multiply_const_vxx_1, 0), (self.dab_ofdm_coarse_frequency_correction_vcvc_0, 0))
        self.connect((self.blocks_short_to_float_0, 0), (self.audio_sink_0, 0))
        self.connect((self.blocks_short_to_float_1, 0), (self.audio_sink_0, 1))
        self.connect((self.blocks_vector_to_stream_0, 0), (self.blocks_multiply_const_vxx_0, 0))
        self.connect((self.dab_demux_cc_0, 0), (self.blocks_vector_to_stream_0, 0))
        self.connect((self.dab_demux_cc_0, 1), (self.dab_msc_decode_0, 0))
        self.connect((self.dab_diff_phasor_vcc_0, 0), (self.dab_frequency_interleaver_vcc_0, 0))
        self.connect((self.dab_firecode_check_bb_0, 0), (self.reed_solomon_decode_bb_0, 0))
        self.connect((self.dab_frequency_interleaver_vcc_0, 0), (self.dab_demux_cc_0, 0))
        self.connect((self.dab_mp4_decode_bs_0, 0), (self.blocks_short_to_float_0, 0))
        self.connect((self.dab_mp4_decode_bs_0, 1), (self.blocks_short_to_float_1, 0))
        self.connect((self.dab_msc_decode_0, 0), (self.dab_firecode_check_bb_0, 0))
        self.connect((self.dab_ofdm_coarse_frequency_correction_vcvc_0, 0), (self.dab_diff_phasor_vcc_0, 0))
        self.connect((self.dab_ofdm_synchronization_cvc_0, 0), (self.fft_vxx_0, 0))
        self.connect((self.fft_vxx_0, 0), (self.blocks_multiply_const_vxx_1, 0))
        self.connect((self.reed_solomon_decode_bb_0, 0), (self.dab_mp4_decode_bs_0, 0))
        self.connect((self.rtlsdr_source_0, 0), (self.dab_ofdm_synchronization_cvc_0, 0))
        self.connect((self.rtlsdr_source_0, 0), (self.qtgui_freq_sink_x_0, 0))


    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "dab_receiver_detailed")
        self.settings.setValue("geometry", self.saveGeometry())
        self.stop()
        self.wait()

        event.accept()

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.set_dab_params(dp.dab_parameters(1,self.samp_rate,0))
        self.qtgui_freq_sink_x_0.set_frequency_range(0, self.samp_rate)
        self.rtlsdr_source_0.set_sample_rate(self.samp_rate)

    def get_gain_slider(self):
        return self.gain_slider

    def set_gain_slider(self, gain_slider):
        self.gain_slider = gain_slider
        self.rtlsdr_source_0.set_gain(self.gain_slider, 0)

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq
        Qt.QMetaObject.invokeMethod(self._freq_line_edit, "setText", Qt.Q_ARG("QString", str(self.freq)))
        self.rtlsdr_source_0.set_center_freq(self.freq, 0)

    def get_dab_params(self):
        return self.dab_params

    def set_dab_params(self, dab_params):
        self.dab_params = dab_params




def main(top_block_cls=dab_receiver_detailed, options=None):
    if gr.enable_realtime_scheduling() != gr.RT_OK:
        print("Error: failed to enable real-time scheduling.")

    if StrictVersion("4.5.0") <= StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
        style = gr.prefs().get_string('qtgui', 'style', 'raster')
        Qt.QApplication.setGraphicsSystem(style)
    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls()

    tb.start()

    tb.show()

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()

        Qt.QApplication.quit()

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    timer = Qt.QTimer()
    timer.start(500)
    timer.timeout.connect(lambda: None)

    qapp.exec_()

if __name__ == '__main__':
    main()
