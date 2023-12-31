#!/usr/bin/env python3

from PyQt5 import QtGui, QtWidgets
from PyQt5.QtCore import QTimer
from PyQt5.QtWidgets import QFileDialog, QHeaderView, QTableWidgetItem, QAbstractItemView
import sys
import time
import user_frontend
import usrp_dab_rx
import usrp_dab_tx
import gnuradio.dab.constants as constants
import math
import json
import sip

resolution_width = 0
resolution_height = 0


class DABstep(QtWidgets.QMainWindow, user_frontend.Ui_MainWindow):
    def __init__(self, parent=None):
        super(DABstep, self).__init__(parent)
        self.setupUi(self)
        self.resize(800, 600)
        # window title
        self.setWindowTitle("DABstep - A DAB/DAB+ transceiver app")
        self.screen_width = resolution_width
        self.screen_height = resolution_height
        # show logo if it exists
        self.path = constants.icon_path
        self.label_logo.setText("<img src=\"" + str(self.path) +
                                "/DAB_logo.png\">")

        # tab definitions
        self.modes = {"rec": 0, "trans": 1, "dev": 2}
        self.mode = self.modes["rec"]
        self.mode_tabs.currentChanged.connect(self.change_tab)

        # lookup table
        self.table = lookup_tables()

        ######################################################################
        # TAB RECEPTION (defining variables, signals and slots)
        ######################################################################
        # receiver variables
        self.bit_rate = 8
        self.address = 0
        self.size = 6
        self.protection = 2
        self.audio_bit_rate = 16000
        self.volume = 80
        self.subch = -1
        self.dabplus = True
        self.need_new_init = True
        self.file_path = "None"
        self.src_is_USRP = True
        self.src_is_RTL = False
        self.receiver_running = False
        self.audio_playing = False
        self.recording = False

        # table preparations
        header = self.table_mci.horizontalHeader()
        header.setSectionResizeMode(0, QtWidgets.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        header.setSectionResizeMode(2, QtWidgets.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(3, QtWidgets.QHeaderView.ResizeToContents)
        header.setStretchLastSection(False)
        self.table_mci.verticalHeader().hide()
        self.table_mci.setSelectionBehavior(QAbstractItemView.SelectRows)

        # timer for update of SNR
        self.snr_timer = QTimer()
        self.snr_timer.timeout.connect(self.snr_update)
        # change of source by radio buttons
        self.rbtn_USRP.clicked.connect(self.src2USRP)
        self.rbtn_RTL.clicked.connect(self.src2RTL)
        self.rbtn_File.clicked.connect(self.src2File)
        # set file path
        self.btn_file_path.clicked.connect(self.set_file_path)
        # init button initializes receiver with center frequency
        self.btn_init.clicked.connect(self.init_receiver)
        # update button updates services in table
        self.btn_update_info.clicked.connect(self.update_service_info)
        # a click on a cell of the service table let the text box show more info to the selected service
        self.table_mci.cellClicked.connect(self.selected_subch)
        # a click on the play button compiles receiver with new subch info and plays the audio
        self.btn_play.clicked.connect(self.play_audio)
        # adjust audio sampling rate
        self.timer_audio_sampling_rate = QTimer()
        self.timer_audio_sampling_rate.timeout.connect(
            self.adjust_audio_sampling_rate)
        # gain setter
        self.gain_spin.valueChanged.connect(self.set_rx_gain)
        # stop button click stops audio reception
        self.btn_stop.hide()
        self.btn_stop.clicked.connect(self.stop_audio)
        # volume slider moved
        self.slider_volume.valueChanged.connect(self.set_volume)
        # record button
        self.btn_record.clicked.connect(self.record_audio)
        self.btn_record.setCheckable(True)

        ######################################################################
        # DEVELOPER MODE
        ######################################################################
        self.dev_mode_active = False
        # hide scroll area
        self.dev_scroll_area.hide()
        # hide close button and just show open button
        self.btn_dev_mode_close.hide()
        # dev mode open button pressed
        self.btn_dev_mode_open.clicked.connect(self.dev_mode_open)
        # dev mode close button pressed
        self.btn_dev_mode_close.clicked.connect(self.dev_mode_close)
        # firecode display
        self.firecode_timer = QTimer()
        self.firecode_timer.timeout.connect(self.update_firecode)
        self.label_firecode.setText("")
        self.label_fic.setText("")
        font = QtGui.QFont()
        font.setFamily("CourierNew")
        self.label_firecode.setFont(font)
        self.label_firecode.hide()
        self.led_msc.hide()
        self.label_label_msc.hide()
        self.label_fic.setFont(font)
        self.label_fic.hide()
        self.led_fic.hide()
        self.label_label_fic.hide()
        self.content_count = 0

        ######################################################################
        # TAB TRANSMISSION (defining variables, signals and slots)
        ######################################################################
        # change of sink by radio buttons
        self.t_rbtn_USRP.clicked.connect(self.t_set_sink_USRP)
        self.t_rbtn_File.clicked.connect(self.t_set_sink_File)
        # create dict for service components
        self.components = [{
            "label": self.t_label_comp1,
            "data_rate_label": self.t_label_rate1,
            "data_rate": self.t_spin_rate1,
            "protection_label": self.t_label_prot1,
            "protection": self.t_combo_prot1,
            "enabled": True,
            "src_label": self.t_label_comp_src1,
            "src_path_disp": self.t_label_path_src1,
            "src_btn": self.t_btn_path_src1,
            "src_path": "None",
            "label_label": self.t_label_label1,
            "edit_label": self.t_edit_service_label1,
            "combo_dabplus": self.t_combo_dabplus1,
            "btn_record": self.t_btn_record1,
            "audio_rate_label": self.t_label_audio_rate1,
            "combo_audio_rate": self.t_combo_audio_rate1,
            "combo_audio_rate_dab": self.t_combo_audio_rate_dab1,
            "combo_stereo": self.t_combo_stereo1
        }, {
            "label": self.t_label_comp2,
            "data_rate_label": self.t_label_rate2,
            "data_rate": self.t_spin_rate2,
            "protection_label": self.t_label_prot2,
            "protection": self.t_combo_prot2,
            "enabled": False,
            "src_label": self.t_label_comp_src2,
            "src_path_disp": self.t_label_path_src2,
            "src_btn": self.t_btn_path_src2,
            "src_path": "None",
            "label_label": self.t_label_label2,
            "edit_label": self.t_edit_service_label2,
            "combo_dabplus": self.t_combo_dabplus2,
            "btn_record": self.t_btn_record2,
            "audio_rate_label": self.t_label_audio_rate2,
            "combo_audio_rate": self.t_combo_audio_rate2,
            "combo_audio_rate_dab": self.t_combo_audio_rate_dab2,
            "combo_stereo": self.t_combo_stereo2
        }, {
            "label": self.t_label_comp3,
            "data_rate_label": self.t_label_rate3,
            "data_rate": self.t_spin_rate3,
            "protection_label": self.t_label_prot3,
            "protection": self.t_combo_prot3,
            "enabled": False,
            "src_label": self.t_label_comp_src3,
            "src_path_disp": self.t_label_path_src3,
            "src_btn": self.t_btn_path_src3,
            "src_path": "None",
            "label_label": self.t_label_label3,
            "edit_label": self.t_edit_service_label3,
            "combo_dabplus": self.t_combo_dabplus3,
            "btn_record": self.t_btn_record3,
            "audio_rate_label": self.t_label_audio_rate3,
            "combo_audio_rate": self.t_combo_audio_rate3,
            "combo_audio_rate_dab": self.t_combo_audio_rate_dab3,
            "combo_stereo": self.t_combo_stereo3
        }, {
            "label": self.t_label_comp4,
            "data_rate_label": self.t_label_rate4,
            "data_rate": self.t_spin_rate4,
            "protection_label": self.t_label_prot4,
            "protection": self.t_combo_prot4,
            "enabled": False,
            "src_label": self.t_label_comp_src4,
            "src_path_disp": self.t_label_path_src4,
            "src_btn": self.t_btn_path_src4,
            "src_path": "None",
            "label_label": self.t_label_label4,
            "edit_label": self.t_edit_service_label4,
            "combo_dabplus": self.t_combo_dabplus4,
            "btn_record": self.t_btn_record4,
            "audio_rate_label": self.t_label_audio_rate4,
            "combo_audio_rate": self.t_combo_audio_rate4,
            "combo_audio_rate_dab": self.t_combo_audio_rate_dab4,
            "combo_stereo": self.t_combo_stereo4
        }, {
            "label": self.t_label_comp5,
            "data_rate_label": self.t_label_rate5,
            "data_rate": self.t_spin_rate5,
            "protection_label": self.t_label_prot5,
            "protection": self.t_combo_prot5,
            "enabled": False,
            "src_label": self.t_label_comp_src5,
            "src_path_disp": self.t_label_path_src5,
            "src_btn": self.t_btn_path_src5,
            "src_path": "None",
            "label_label": self.t_label_label5,
            "edit_label": self.t_edit_service_label5,
            "combo_dabplus": self.t_combo_dabplus5,
            "btn_record": self.t_btn_record5,
            "audio_rate_label": self.t_label_audio_rate5,
            "combo_audio_rate": self.t_combo_audio_rate5,
            "combo_audio_rate_dab": self.t_combo_audio_rate_dab5,
            "combo_stereo": self.t_combo_stereo5
        }, {
            "label": self.t_label_comp6,
            "data_rate_label": self.t_label_rate6,
            "data_rate": self.t_spin_rate6,
            "protection_label": self.t_label_prot6,
            "protection": self.t_combo_prot6,
            "enabled": False,
            "src_label": self.t_label_comp_src6,
            "src_path_disp": self.t_label_path_src6,
            "src_btn": self.t_btn_path_src6,
            "src_path": "None",
            "label_label": self.t_label_label6,
            "edit_label": self.t_edit_service_label6,
            "combo_dabplus": self.t_combo_dabplus6,
            "btn_record": self.t_btn_record6,
            "audio_rate_label": self.t_label_audio_rate6,
            "combo_audio_rate": self.t_combo_audio_rate6,
            "combo_audio_rate_dab": self.t_combo_audio_rate_dab6,
            "combo_stereo": self.t_combo_stereo6
        }, {
            "label": self.t_label_comp7,
            "data_rate_label": self.t_label_rate7,
            "data_rate": self.t_spin_rate7,
            "protection_label": self.t_label_prot7,
            "protection": self.t_combo_prot7,
            "enabled": False,
            "src_label": self.t_label_comp_src7,
            "src_path_disp": self.t_label_path_src7,
            "src_btn": self.t_btn_path_src7,
            "src_path": "None",
            "label_label": self.t_label_label7,
            "edit_label": self.t_edit_service_label7,
            "combo_dabplus": self.t_combo_dabplus7,
            "btn_record": self.t_btn_record7,
            "audio_rate_label": self.t_label_audio_rate7,
            "combo_audio_rate": self.t_combo_audio_rate7,
            "combo_audio_rate_dab": self.t_combo_audio_rate_dab7,
            "combo_stereo": self.t_combo_stereo7
        }]
        # update service components initially to hide the service components 2-7
        self.t_update_service_components()
        # provide suggestions for language combo box
        for i in range(0, len(self.table.languages)):
            self.t_combo_language.addItem(self.table.languages[i])
        # provide suggestions for country combo box
        for i in range(0, len(self.table.country_ID_ECC_E0)):
            self.t_combo_country.addItem(self.table.country_ID_ECC_E0[i])
        # update dict "components" and display of service components if spinbox "number of channels" is changed
        self.t_spin_num_subch.valueChanged.connect(self.t_change_num_subch)
        # write ensemble/service info when init is pressed
        self.t_btn_init.pressed.connect(self.t_init_transmitter)
        # start flowgraph when play btn pressed
        self.t_btn_play.pressed.connect(self.t_run_transmitter)
        # stop button pressed
        self.t_btn_stop.pressed.connect(self.t_stop_transmitter)
        # path for File sink path
        self.t_btn_file_path.pressed.connect(self.t_set_file_path)
        # gain setter
        self.tx_gain.valueChanged.connect(self.set_tx_gain)
        # path selection for all 7 (possible) sub channels
        self.t_btn_path_src1.pressed.connect(self.t_set_subch_path1)
        self.t_btn_path_src2.pressed.connect(self.t_set_subch_path2)
        self.t_btn_path_src3.pressed.connect(self.t_set_subch_path3)
        self.t_btn_path_src4.pressed.connect(self.t_set_subch_path4)
        self.t_btn_path_src5.pressed.connect(self.t_set_subch_path5)
        self.t_btn_path_src6.pressed.connect(self.t_set_subch_path6)
        self.t_btn_path_src7.pressed.connect(self.t_set_subch_path7)
        self.t_btn_record1.pressed.connect(self.t_toggle_record1)
        self.t_btn_record2.pressed.connect(self.t_toggle_record2)
        self.t_btn_record3.pressed.connect(self.t_toggle_record3)
        self.t_btn_record4.pressed.connect(self.t_toggle_record4)
        self.t_btn_record5.pressed.connect(self.t_toggle_record5)
        self.t_btn_record6.pressed.connect(self.t_toggle_record6)
        self.t_btn_record7.pressed.connect(self.t_toggle_record7)
        self.t_combo_dabplus1.currentIndexChanged.connect(
            self.change_audio_bit_rates1)
        self.t_combo_dabplus2.currentIndexChanged.connect(
            self.change_audio_bit_rates2)
        self.t_combo_dabplus3.currentIndexChanged.connect(
            self.change_audio_bit_rates3)
        self.t_combo_dabplus4.currentIndexChanged.connect(
            self.change_audio_bit_rates4)
        self.t_combo_dabplus5.currentIndexChanged.connect(
            self.change_audio_bit_rates5)
        self.t_combo_dabplus6.currentIndexChanged.connect(
            self.change_audio_bit_rates6)
        self.t_combo_dabplus7.currentIndexChanged.connect(
            self.change_audio_bit_rates7)
        # set volume if volume slider is changed
        self.t_slider_volume.valueChanged.connect(self.t_set_volume)
        self.transmitter_running = False

    ################################
    # general functions
    ################################
    def change_tab(self):
        if self.mode_tabs.currentWidget() is self.tab_transmission:
            print("changed to transmission mode")
            self.t_update_service_components()

        elif self.mode_tabs.currentWidget() is self.tab_reception:
            print("changed to reception mode")
        else:
            print("changed to unknown tab")

    ################################
    # Receiver functions
    ################################

    def src2USRP(self):
        # enable/disable buttons
        self.btn_file_path.setEnabled(False)
        self.label_path.setEnabled(False)
        self.spinbox_frequency.setEnabled(True)
        self.label_frequency.setEnabled(True)
        self.src_is_USRP = True
        self.src_is_RTL = False

    def src2RTL(self):
        # enable/disable buttons
        self.btn_file_path.setEnabled(False)
        self.label_path.setEnabled(False)
        self.spinbox_frequency.setEnabled(True)
        self.label_frequency.setEnabled(True)
        self.src_is_RTL = True
        self.src_is_USRP = False

    def src2File(self):
        # enable/disable buttons
        self.btn_file_path.setEnabled(True)
        self.label_path.setEnabled(True)
        self.spinbox_frequency.setEnabled(False)
        self.label_frequency.setEnabled(False)
        self.src_is_USRP = False
        self.src_is_RTL = False

    def set_file_path(self):
        path, _ = QFileDialog.getOpenFileName(
            self, "Pick a file with recorded IQ samples")
        if path:  # if user didn't pick a directory don't continue
            self.file_path = str(path)
            self.label_path.setText(path)

    def init_receiver(self):
        if not self.receiver_running:
            self.statusBar.showMessage("initializing receiver ...")
            # stop any processes that access to an instance of usrp_dab_rx
            self.snr_timer.stop()
            self.firecode_timer.stop()
            # check if file path is selected in case that file is the selected source
            if (not self.src_is_USRP) and (not self.src_is_RTL) and (
                    self.file_path == "None"):
                self.label_path.setStyleSheet('color: red')
            else:
                self.label_path.setStyleSheet('color: black')
                # set up and start flowgraph
                self.my_receiver = usrp_dab_rx.usrp_dab_rx(
                    self.spin_dab_mode.value(), self.spinbox_frequency.value(),
                    self.bit_rate, self.address, self.size, self.protection,
                    self.audio_bit_rate, self.dabplus, self.src_is_USRP,
                    self.src_is_RTL, self.file_path)
                self.my_receiver.set_volume(0)
                self.my_receiver.start()
                # status bar
                self.statusBar.showMessage("Reception is running.")
                # init dev mode
                self.dev_mode_init()
                # once scan ensemble automatically (after per clicking btn)
                time.sleep(1)
                self.update_service_info()
                self.btn_update_info.setEnabled(True)
                self.snr_update()
                self.slider_volume.setEnabled(True)
                self.btn_play.setEnabled(True)
                self.btn_stop.setEnabled(True)
                self.btn_update_info.setEnabled(True)
                self.btn_record.setEnabled(True)
                self.snr_timer.start(1000)
                self.firecode_timer.start(120)
                self.receiver_running = True
                self.btn_init.setText("stop receiver")
        else:
            # stop receiver was pressed
            self.dev_mode_close()
            # remove service table
            while (self.table_mci.rowCount() > 0):
                self.table_mci.removeRow(0)
            self.my_receiver.stop()
            self.receiver_running = False
            self.btn_init.setText("start receiver")
            self.slider_volume.setEnabled(False)
            self.btn_play.setEnabled(False)
            self.btn_stop.setEnabled(False)
            self.btn_update_info.setEnabled(False)
            self.btn_record.setEnabled(False)
            self.btn_play.show()
            self.btn_stop.hide()
            self.audio_playing = False
            self.statusBar.showMessage("Receiver stopped.")
            self.bar_snr.setValue(0)
            # reset variables
            self.bit_rate = 8
            self.address = 0
            self.size = 6
            self.protection = 2
            self.audio_bit_rate = 16000
            self.volume = 80
            self.subch = -1
            self.dabplus = True
            self.need_new_init = True
            self.file_path = "None"
            self.receiver_running = False
            self.audio_playing = False
            self.recording = False

    def update_service_info(self):
        # set status bar message
        self.statusBar.showMessage("Scanning ensemble...")
        # remove all old data from table at first
        while (self.table_mci.rowCount() > 0):
            self.table_mci.removeRow(0)

        # get new data from fic_sink
        service_data = self.get_service_info()
        service_labels = self.get_service_labels()

        # write new data to table
        for n, key in enumerate(
                sorted(service_data,
                       key=lambda service_data: service_data['ID'])):
            # add a new row
            self.table_mci.insertRow(n)
            # print ID in first collumn
            self.table_mci.setItem(n, 0, QTableWidgetItem(str(key['ID'])))
            # print reference (later label)
            self.table_mci.setItem(
                n, 1,
                QTableWidgetItem(
                    next((item for item in service_labels
                          if item["reference"] == key['reference']),
                         {'label': "not found"})['label']))
            # print type
            self.table_mci.setItem(
                n, 3,
                QTableWidgetItem("primary" if key['primary'] ==
                                 True else "secondary"))
            # print DAB Mode
            self.table_mci.setItem(
                n, 2,
                QTableWidgetItem(("DAB+" if key['DAB+'] == True else "DAB ")))

        # set number of sub channels
        self.num_subch = self.table_mci.rowCount()

        # display ensemble info
        ensemble_data = self.get_ensemble_info()
        self.label_ensemble.setText(ensemble_data.keys()[0].strip())
        self.label_country.setText(
            str(self.table.country_ID_ECC_E0[int(
                ensemble_data.values()[0]['country_ID'])]))
        self.lcd_number_num_subch.display(self.num_subch)
        # status bar
        self.statusBar.showMessage("Select a Service Component.")

    def selected_subch(self):
        # enable/disable buttons
        self.btn_play.setEnabled(True)
        self.btn_record.setEnabled(True)
        if self.audio_playing:
            self.btn_stop.show()
            self.btn_play.show()

        # get selected sub-channel by its ID
        ID = self.table_mci.item(self.table_mci.currentRow(), 0).text()
        # find related service reference to ID
        reference = next(
            (item
             for item in self.get_service_info() if item['ID'] == int(ID)),
            {'reference': -1})['reference']
        # get dicts to specific service and sub-channel
        service_data = next(
            (item
             for item in self.get_service_info() if item['ID'] == int(ID)), {
                 "reference": -1,
                 "ID": -1,
                 "primary": True
             })
        service_label = next((item for item in self.get_service_labels()
                              if item['reference'] == int(reference)), {
                                  "reference": -1,
                                  "label": "not found"
                              })
        subch_data = next(
            (item for item in self.get_subch_info() if item['ID'] == int(ID)),
            {
                "ID": -1,
                "address": 0,
                "protection": 0,
                "size": 0
            })
        programme_type = next((item for item in self.get_programme_type()
                               if item["reference"] == reference),
                              {"programme_type": 0})

        # update sub-channel info for receiver
        self.address = int(subch_data['address'])
        self.size = int(subch_data['size'])
        self.protection = int(subch_data['protection'])
        conv_table = [12, 8, 6, 5]
        self.bit_rate = self.size * 8 / conv_table[self.protection]
        self.dabplus = service_data['DAB+']
        if self.dabplus:
            if self.bit_rate < 100:
                self.audio_bit_rate = 48000
            else:
                self.audio_bit_rate = 32000
        else:
            self.audio_bit_rate = 48000

        # display info to selected sub-channel
        # service info
        self.label_service.setText(service_label['label'].strip())
        self.label_bit_rate.setText(str(subch_data['size'] * 8 / 6) + " kbps")
        # service component (=sub-channel) info
        self.label_primary.setText(
            ("primary" if service_data['primary'] == True else "secondary"))
        self.label_dabplus.setText(
            ("DAB+" if service_data['DAB+'] == True else "DAB"))
        # programme type
        if programme_type["programme_type"] is 0:
            self.label_programme_type.setText("\n")
        else:
            self.label_programme_type.setText(
                self.table.programme_types[programme_type["programme_type"]] +
                "\n")
        # status Bar
        self.statusBar.showMessage(
            "Play/Record the selected service component.")

    def snr_update(self):
        # display snr in progress bar if an instance of usrp_dab_rx is existing
        if hasattr(self, 'my_receiver') and self.receiver_running:
            SNR = self.my_receiver.get_snr()
            if SNR > 15.0:
                self.setStyleSheet(
                    """QProgressBar::chunk { background: "green"; }""")
            elif SNR > 10.0:
                self.setStyleSheet(
                    """QProgressBar::chunk { background: "yellow"; }""")
            elif SNR <= 10.0:
                self.setStyleSheet(
                    """QProgressBar::chunk { background: "red"; }""")
            self.lcd_snr.display(SNR)
            if SNR > 40:
                SNR = 20
            elif SNR < 0:
                SNR = 0
            self.bar_snr.setValue(int(SNR))

        else:
            self.bar_snr.setValue(0)
            #self.label_snr.setText("SNR: no reception")
        self.snr_timer.start(1000)

    def play_audio(self):
        # play button pressed
        # if selected sub-channel is not the current sub-channel we have to reconfigure the receiver
        if self.subch is not self.table_mci.currentRow():
            self.subch = self.table_mci.currentRow()
            dev_mode_opened = False
            if self.dev_mode_active:
                dev_mode_opened = True
                self.dev_mode_close()
            self.snr_timer.stop()
            self.firecode_timer.stop()
            self.my_receiver.stop()
            self.temp_src = self.my_receiver.src
            self.my_receiver = usrp_dab_rx.usrp_dab_rx(
                self.spin_dab_mode.value(),
                self.spinbox_frequency.value(),
                self.bit_rate,
                self.address,
                self.size,
                self.protection,
                self.audio_bit_rate,
                self.dabplus,
                self.src_is_USRP,
                self.src_is_RTL,
                self.file_path,
                prev_src=self.temp_src)
            self.my_receiver.set_volume(
                float(self.slider_volume.value()) / 100)
            self.my_receiver.start()
            self.statusBar.showMessage("Audio playing.")
            self.dev_mode_init()
            if dev_mode_opened:
                self.dev_mode_open()
        self.my_receiver.set_volume(float(self.slider_volume.value()) / 100)
        self.audio_playing = True
        # start timer for snr update again
        self.snr_timer.start(1000)
        self.firecode_timer.start(120)
        # start audio sampling rate timer
        self.timer_audio_sampling_rate.start(500)
        # toggle start/stop button
        self.btn_play.hide()
        self.btn_stop.show()

    def stop_audio(self):
        self.my_receiver.set_volume(0)
        self.audio_playing = False
        self.btn_play.show()
        self.btn_stop.hide()

    def adjust_audio_sampling_rate(self):
        self.timer_audio_sampling_rate.stop()
        new_sampling_rate = self.my_receiver.get_sample_rate()
        if new_sampling_rate != self.audio_bit_rate and new_sampling_rate != -1:
            self.audio_bit_rate = new_sampling_rate
            self.statusBar.showMessage("Adjusting audio sampling rate to " +
                                       str(new_sampling_rate))
            self.my_receiver.stop()
            self.my_receiver = usrp_dab_rx.usrp_dab_rx(
                self.spin_dab_mode.value(),
                self.spinbox_frequency.value(),
                self.bit_rate,
                self.address,
                self.size,
                self.protection,
                self.audio_bit_rate,
                self.dabplus,
                self.src_is_USRP,
                self.src_is_RTL,
                self.file_path,
                prev_src=self.temp_src)

            self.my_receiver.start()
        elif new_sampling_rate == -1:
            self.timer_audio_sampling_rate.start(200)

    def record_audio(self):
        if self.recording:
            self.my_receiver.set_valve_closed(True)
            self.recording = False
            self.btn_record.setChecked(False)

        else:
            self.my_receiver.set_valve_closed(False)
            self.recording = True
            self.btn_record.setChecked(True)

    def set_volume(self):
        if self.audio_playing:
            # map volume from [0:100] to [0:1]
            self.my_receiver.set_volume(
                float(self.slider_volume.value()) / 100)

    def get_ensemble_info(self):
        self.json = self.my_receiver.get_ensemble_info()
        if self.json is "":
            return {"unknown": {"country_ID": 0}}
        else:
            # load string (json) with ensemble info and convert it to dictionary
            # string structure example: "{\"SWR_BW_N\":{\"country_ID\":1}}"
            self.ensemble_info = json.loads(self.json)
            json.dumps(self.ensemble_info)
            return self.ensemble_info

    def get_service_info(self):
        self.json = self.my_receiver.get_service_info()
        if self.json is "":
            return []
        else:
            # load string (json) with MCI and convert it to array of dictionaries
            self.service_info = json.loads(self.json)
            # string structure example: "[{\"reference\":736,\"ID\":2,\"primary\":true},{\"reference\":736,\"ID\":3,\"primary\":false},{\"reference\":234,\"ID\":5,\"primary\":true}]"
            json.dumps(self.service_info)
            return self.service_info

    def get_service_labels(self):
        self.json = self.my_receiver.get_service_labels()
        if self.json is "":
            return []
        else:
            # load string (json) with service labels and convert it to array of dictionaries
            self.service_labels = json.loads(self.json)
            # string structure example: "[{\"label\":\"SWR1_BW         \",\"reference\":736},{\"label\":\"SWR2            \",\"reference\":234}]"
            json.dumps(self.service_labels)
            return self.service_labels

    def get_subch_info(self):
        self.json = self.my_receiver.get_subch_info()
        if self.json is "":
            return []
        else:
            # load string (json) with sub-channel info and convert it to array of dictionaries
            self.subch_info = json.loads(self.json)
            # string structure example: "[{\"ID\":2, \"address\":54, \"protect\":2,\"size\":84},{\"ID\":3, \"address\":54, \"protect\":2,\"size\":84}]"
            json.dumps(self.subch_info)
            return self.subch_info

    def get_programme_type(self):
        self.json = self.my_receiver.get_programme_type()
        # load string (json) with service information (programme type) and convert it to array of dictionaries
        if self.json == "":
            return []
        else:
            self.programme_type = json.loads(self.json)
            # string structure example: "[{\"reference\":736, \"programme_type\":13},{\"reference\":234, \"programme_type\":0}]"
            json.dumps(self.programme_type)
            return self.programme_type

    def get_sample_rate(self):
        # TODO: set rational resampler in flowgraoph with sample rate
        return self.my_receiver.get_sample_rate()

    ################################
    # Developer Mode
    ################################

    def dev_mode_init(self):
        available_height = self.screen_height - 200
        # constellation plot
        self.constellation = sip.wrapinstance(
            self.my_receiver.constellation_plot.pyqwidget(), QtGui.QWidget)
        self.vertical_layout_dev_mode_right.addWidget(self.constellation)
        self.constellation.setMaximumHeight(available_height / 3)
        self.constellation.setMaximumWidth(available_height / 3 * 2)
        self.constellation.hide()
        # FFT plot
        self.fft_plot = sip.wrapinstance(self.my_receiver.fft_plot.pyqwidget(),
                                         QtGui.QWidget)
        self.vertical_layout_dev_mode_right.addWidget(self.fft_plot)
        self.fft_plot.setMaximumHeight(available_height / 3)
        self.fft_plot.setMaximumWidth(available_height / 3 * 2)
        self.fft_plot.hide()
        # Waterfall plot
        self.waterfall_plot = sip.wrapinstance(
            self.my_receiver.waterfall_plot.pyqwidget(), QtGui.QWidget)
        self.vertical_layout_dev_mode_right.addWidget(self.waterfall_plot)
        self.waterfall_plot.setMaximumHeight(available_height / 3)
        self.waterfall_plot.setMaximumWidth(available_height / 3 * 2)
        self.waterfall_plot.hide()
        # if dev mode is initialized, we can enable the dev mode open button
        self.btn_dev_mode_open.setEnabled(True)

    def dev_mode_open(self):
        self.dev_mode_active = True
        self.dev_scroll_area.show()
        # hide open button and show close button
        self.btn_dev_mode_open.hide()
        self.btn_dev_mode_close.show()
        # show widgets
        self.fft_plot.show()
        self.waterfall_plot.show()
        self.constellation.show()
        self.label_firecode.show()
        self.led_msc.setText("<img src=\"" + str(self.path) +
                             "led_green.png\">")
        self.led_msc.show()
        self.label_label_msc.show()
        self.label_firecode.setText("")
        self.label_fic.show()
        self.led_fic.setText("<img src=\"" + str(self.path) +
                             "led_green.png\">")
        self.led_fic.show()
        self.label_label_fic.show()
        self.label_fic.setText("")
        self.content_count = 0
        # resize window width
        self.showMaximized()

    def dev_mode_close(self):
        self.dev_mode_active = False
        self.dev_scroll_area.hide()
        # hide close button and show open button
        self.btn_dev_mode_close.hide()
        self.btn_dev_mode_open.show()
        # hide widgets
        self.fft_plot.hide()
        self.waterfall_plot.hide()
        self.constellation.hide()
        self.label_firecode.hide()
        self.led_msc.hide()
        self.label_label_msc.hide()
        self.label_fic.hide()
        self.led_fic.hide()
        self.label_label_fic.hide()
        self.resize(800, 600)

    def update_firecode(self):
        if self.dev_mode_active:
            if self.content_count >= 80:
                self.label_firecode.setText("")
                self.label_fic.setText("")
                self.content_count = 0
            # write msc status
            if not self.my_receiver.get_firecode_passed():
                self.label_firecode.setText(self.label_firecode.text() +
                                            "<font color=\"red\">X </font>")
                self.led_msc.setText("<img src=\"" + self.path +
                                     "/led_red.png\">")
            else:
                errors = self.my_receiver.get_corrected_errors()
                if errors < 10:
                    self.label_firecode.setText(self.label_firecode.text() +
                                                "<font color=\"green\">" +
                                                str(errors) + " < /font>")
                    self.led_msc.setText("<img src=\"" + self.path +
                                         "/led_green.png\">")
                else:
                    self.label_firecode.setText(self.label_firecode.text() +
                                                "<font color=\"orange\">" +
                                                str(errors) + "</font>")
                    self.led_msc.setText("<img src=\"" + self.path +
                                         "/led_orange.png\">")
            # write fic status
            if self.my_receiver.get_crc_passed():
                self.label_fic.setText(self.label_fic.text() +
                                       "<font color=\"green\">0 </font>")
                self.led_fic.setText("<img src=\"" + self.path +
                                     "/led_green.png\">")
            else:
                self.label_fic.setText(self.label_fic.text() +
                                       "<font color=\"red\">X </font>")
                self.led_fic.setText("<img src=\"" + self.path +
                                     "/led_red.png\">")
        self.label_fic.setWordWrap(True)
        self.label_firecode.setWordWrap(True)
        self.content_count += 1

    ################################
    # Transmitter functions
    ################################

    def t_set_sink_USRP(self):
        # enable/disable buttons
        self.t_btn_file_path.setEnabled(False)
        self.t_label_sink.setEnabled(False)
        self.t_spinbox_frequency.setEnabled(True)
        self.t_label_frequency.setEnabled(True)

    def t_set_sink_File(self):
        # enable/disable buttons
        self.t_btn_file_path.setEnabled(True)
        self.t_label_sink.setEnabled(True)
        self.t_spinbox_frequency.setEnabled(False)
        self.t_label_frequency.setEnabled(False)

    def t_change_num_subch(self):
        # get number of sub-channels from spin box
        num_subch = self.t_spin_num_subch.value()
        # update info text under the component fill in forms
        if num_subch is 7:
            self.t_label_increase_num_subch_info.setText(
                "7 is the maximum number of components")
        else:
            self.t_label_increase_num_subch_info.setText(
                "increase \"Number of channels\" for more components")
        # enable num_subch fill in forms for sub-channels
        if 0 <= num_subch <= 7:
            for n in range(0, 7):
                if n < num_subch:
                    self.components[n]["enabled"] = True
                else:
                    self.components[n]["enabled"] = False
        # display changes
        self.t_update_service_components()
        self.t_spin_listen_to_component.setMaximum(num_subch)

    def t_update_service_components(self):
        # display/hide components referring to the info in components (dict)
        for component in self.components:
            if component["enabled"] is False:
                component["label"].hide()
                component["data_rate_label"].hide()
                component["data_rate"].hide()
                component["protection_label"].hide()
                component["protection"].hide()
                component["src_label"].hide()
                component["src_path_disp"].hide()
                component["src_btn"].hide()
                component["label_label"].hide()
                component["edit_label"].hide()
                component["combo_dabplus"].hide()
                component["btn_record"].hide()
                component["audio_rate_label"].hide()
                component["combo_audio_rate"].hide()
                component["combo_audio_rate_dab"].hide()
                component["combo_stereo"].hide()
            else:
                component["label"].show()
                component["data_rate_label"].show()
                component["data_rate"].show()
                component["protection_label"].show()
                component["protection"].show()
                component["src_label"].show()
                component["src_path_disp"].show()
                component["src_btn"].show()
                component["label_label"].show()
                component["edit_label"].show()
                component["combo_dabplus"].show()
                component["btn_record"].show()
                component["audio_rate_label"].show()
                component["combo_stereo"].show()
                if component["combo_dabplus"].currentIndex() is 0:
                    component["combo_audio_rate"].show()
                    component["combo_audio_rate_dab"].hide()
                else:
                    component["combo_audio_rate_dab"].show()
                    component["combo_audio_rate"].hide()

    def t_init_transmitter(self):
        if self.transmitter_running:
            self.transmitter_running = False
            self.t_btn_init.setText("start transmitter")
        else:
            self.transmitter_running = True
            self.t_btn_init.setText("stop transmitter")
            self.statusBar.showMessage("initializing transmitter...")
            # boolean is set to True if info is missing to init the transmitter
            arguments_incomplete = False
            # produce array for protection and data_rate and src_paths and stereo flags
            num_subch = self.t_spin_num_subch.value()
            protection_array = [None] * num_subch
            data_rate_n_array = [None] * num_subch
            audio_sampling_rate_array = [None] * num_subch
            audio_paths = [None] * num_subch
            stereo_flags = [None] * num_subch
            merged_service_string = ""
            dabplus_types = [True] * num_subch
            record_states = [False] * num_subch
            for i in range(0, num_subch):
                # write array with protection modes
                protection_array[i] = self.components[i][
                    "protection"].currentIndex()
                # write array with data rates
                data_rate_n_array[i] = self.components[i]["data_rate"].value(
                ) / 8
                # write stereo flags
                stereo_flags[i] = self.components[i][
                    "combo_stereo"].currentIndex()
                # write audio sampling rates in array
                if self.components[i]["combo_dabplus"].currentIndex() is 0:
                    audio_sampling_rate_array[i] = 32000 if (
                        self.components[i]["combo_audio_rate"].currentIndex()
                        is 0) else 48000
                else:
                    audio_sampling_rate_array[i] = 48000 if (
                        self.components[i]["combo_audio_rate_dab"].
                        currentIndex() is 0) else 24000
                # check audio paths
                if self.components[i]["src_path"] is "None":
                    # highlight the path which is not selected
                    self.components[i]["src_path_disp"].setStyleSheet(
                        'color: red')
                    arguments_incomplete = True
                    self.statusBar.showMessage("path " + str(i + 1) +
                                               " not selected")
                # check if length of label is <= 16 chars
                elif len(str(self.components[i]["edit_label"].text())) > 16:
                    self.components[i]["edit_label"].setStyleSheet(
                        'color: red')
                    arguments_incomplete = True
                    self.statusBar.showMessage(
                        "Warning: Label is longer than 16 characters!")
                else:
                    audio_paths[i] = self.components[i]["src_path"]
                    # write service labels appended in one string
                    merged_service_string = merged_service_string + str(
                        self.components[i]["edit_label"].text()).ljust(16)
                    self.components[i]["edit_label"].setStyleSheet(
                        'color: black')
                    # write dabplus types
                    dabplus_types[i] = (
                        1 if
                        self.components[i]["combo_dabplus"].currentIndex() is 0
                        else 0)

            # check if length of ensemble label is <= 16 chars
            if len(str(self.t_edit_ensemble_label.text())) > 16:
                self.t_edit_ensemble_label.setStyleSheet('color: red')
                arguments_incomplete = True
            else:
                self.t_edit_ensemble_label.setStyleSheet('color: black')
            # check if File path for sink is chosen if option enabled
            if self.t_rbtn_File.isChecked() and (str(self.t_label_sink.text())
                                                 == "select path"):
                self.t_label_sink.setStyleSheet('color: red')
                arguments_incomplete = True

            if arguments_incomplete is False:
                # init transmitter
                self.my_transmitter = usrp_dab_tx.usrp_dab_tx(
                    self.t_spin_dab_mode.value(),
                    self.t_spinbox_frequency.value(),
                    self.t_spin_num_subch.value(),
                    str(self.t_edit_ensemble_label.text()),
                    merged_service_string,
                    self.t_combo_language.currentIndex(),
                    self.t_combo_country.currentIndex(), protection_array,
                    data_rate_n_array, stereo_flags, audio_sampling_rate_array,
                    audio_paths, self.t_spin_listen_to_component.value(),
                    self.t_rbtn_USRP.isChecked(), dabplus_types,
                    str(self.t_label_sink.text()) + "/" +
                    str(self.t_edit_file_name.text()))

                # enable play button
                self.t_btn_play.setEnabled(True)
                self.t_label_status.setText("ready to transmit")
                self.statusBar.showMessage("ready to transmit")

    def t_run_transmitter(self):
        self.t_btn_stop.setEnabled(True)
        self.t_slider_volume.setEnabled(True)
        self.t_label_status.setText("transmitting...")
        self.statusBar.showMessage("transmitting...")
        self.my_transmitter.start()

    def t_set_volume(self):
        self.my_transmitter.set_volume(
            float(self.t_slider_volume.value()) / 100)

    def t_stop_transmitter(self):
        # stop flowgraph
        self.my_transmitter.stop()
        self.t_btn_stop.setEnabled(False)
        self.t_slider_volume.setEnabled(False)
        self.t_label_status.setText("not running")
        self.statusBar.showMessage("Transmission stopped.")

    def t_set_file_path(self):
        path = QFileDialog.getExistingDirectory(
            self, "Pick a folder for your file sink")
        if path:  # if user didn't pick a directory don't continue
            # display path in path label
            self.t_label_sink.setText(path)
            self.t_label_sink.setStyleSheet('color: black')

    def t_set_subch_path1(self):
        path,_ = QFileDialog.getOpenFileName(self,
                                           "Pick a .wav file as audio source")
        if path:  # if user didn't pick a directory don't continue
            # set new path in dict components
            self.components[0]["src_path"] = str(path)
            # display path in path label
            self.components[0]["src_path_disp"].setText(path)
            self.components[0]["src_path_disp"].setStyleSheet('color: black')
        self.t_btn_record1.setFlat(False)

    def t_set_subch_path2(self):
        path,_ = QFileDialog.getOpenFileName(self,
                                           "Pick a .wav file as audio source")
        if path:  # if user didn't pick a directory don't continue
            # set new path in dict components
            self.components[1]["src_path"] = str(path)
            # display path in path label
            self.components[1]["src_path_disp"].setText(path)
            self.components[1]["src_path_disp"].setStyleSheet('color: black')

    def t_set_subch_path3(self):
        path,_ = QFileDialog.getOpenFileName(self,
                                           "Pick a .wav file as audio source")
        if path:  # if user didn't pick a directory don't continue
            # set new path in dict components
            self.components[2]["src_path"] = str(path)
            # display path in path label
            self.components[2]["src_path_disp"].setText(path)
            self.components[2]["src_path_disp"].setStyleSheet('color: black')

    def t_set_subch_path4(self):
        path, _ = QFileDialog.getOpenFileName(self,
                                           "Pick a .wav file as audio source")
        if path:  # if user didn't pick a directory don't continue
            # set new path in dict components
            self.components[3]["src_path"] = str(path)
            # display path in path label
            self.components[3]["src_path_disp"].setText(path)
            self.components[3]["src_path_disp"].setStyleSheet('color: black')

    def t_set_subch_path5(self):
        path, _ = QFileDialog.getOpenFileName(self,
                                           "Pick a .wav file as audio source")
        if path:  # if user didn't pick a directory don't continue
            # set new path in dict components
            self.components[4]["src_path"] = str(path)
            # display path in path label
            self.components[4]["src_path_disp"].setText(path)
            self.components[4]["src_path_disp"].setStyleSheet('color: black')

    def t_set_subch_path6(self):
        path, _ = QFileDialog.getOpenFileName(self,
                                           "Pick a .wav file as audio source")
        if path:  # if user didn't pick a directory don't continue
            # set new path in dict components
            self.components[5]["src_path"] = str(path)
            # display path in path label
            self.components[5]["src_path_disp"].setText(path)
            self.components[5]["src_path_disp"].setStyleSheet('color: black')

    def t_set_subch_path7(self):
        path, _ = QFileDialog.getOpenFileName(self,
                                           "Pick a .wav file as audio source")
        if path:  # if user didn't pick a directory don't continue
            # set new path in dict components
            self.components[6]["src_path"] = str(path)
            # display path in path label
            self.components[6]["src_path_disp"].setText(path)
        self.components[7]["src_path_disp"].setStyleSheet('color: black')

    def t_toggle_record1(self):
        self.t_label_path_src1.setText("microphone")
        self.components[0]["src_path"] = "mic"

    def t_toggle_record2(self):
        self.t_label_path_src2.setText("microphone")
        self.components[1]["src_path"] = "mic"

    def t_toggle_record3(self):
        self.t_label_path_src3.setText("microphone")
        self.components[2]["src_path"] = "mic"

    def t_toggle_record4(self):
        self.t_label_path_src4.setText("microphone")
        self.components[3]["src_path"] = "mic"

    def t_toggle_record5(self):
        self.t_label_path_src5.setText("microphone")
        self.components[4]["src_path"] = "mic"

    def t_toggle_record6(self):
        self.t_label_path_src6.setText("microphone")
        self.components[5]["src_path"] = "mic"

    def t_toggle_record7(self):
        self.t_label_path_src7.setText("microphone")
        self.components[6]["src_path"] = "mic"

    def change_audio_bit_rates1(self):
        if self.t_combo_dabplus1.currentIndex() is 0:
            self.t_combo_audio_rate1.show()
            self.t_combo_audio_rate_dab1.hide()
        else:
            self.t_combo_audio_rate_dab1.show()
            self.t_combo_audio_rate1.hide()

    def change_audio_bit_rates2(self):
        if self.t_combo_dabplus2.currentIndex() is 0:
            self.t_combo_audio_rate2.show()
            self.t_combo_audio_rate_dab2.hide()
        else:
            self.t_combo_audio_rate_dab2.show()
            self.t_combo_audio_rate2.hide()

    def change_audio_bit_rates3(self):
        if self.t_combo_dabplus3.currentIndex() is 0:
            self.t_combo_audio_rate3.show()
            self.t_combo_audio_rate_dab3.hide()
        else:
            self.t_combo_audio_rate_dab3.show()
            self.t_combo_audio_rate3.hide()

    def change_audio_bit_rates4(self):
        if self.t_combo_dabplus4.currentIndex() is 0:
            self.t_combo_audio_rate4.show()
            self.t_combo_audio_rate_dab4.hide()
        else:
            self.t_combo_audio_rate_dab4.show()
            self.t_combo_audio_rate4.hide()

    def change_audio_bit_rates5(self):
        if self.t_combo_dabplus5.currentIndex() is 0:
            self.t_combo_audio_rate5.show()
            self.t_combo_audio_rate_dab5.hide()
        else:
            self.t_combo_audio_rate_dab5.show()
            self.t_combo_audio_rate5.hide()

    def change_audio_bit_rates6(self):
        if self.t_combo_dabplus6.currentIndex() is 0:
            self.t_combo_audio_rate6.show()
            self.t_combo_audio_rate_dab6.hide()
        else:
            self.t_combo_audio_rate_dab6.show()
            self.t_combo_audio_rate6.hide()

    def change_audio_bit_rates7(self):
        if self.t_combo_dabplus7.currentIndex() is 0:
            self.t_combo_audio_rate7.show()
            self.t_combo_audio_rate_dab7.hide()
        else:
            self.t_combo_audio_rate_dab7.show()
            self.t_combo_audio_rate7.hide()

    def set_rx_gain(self):
        if hasattr(self, 'my_receiver'):
            self.my_receiver.set_gain(self.gain_spin.value())

    def set_tx_gain(self):
        if hasattr(self, 'my_transmitter'):
            self.my_transmitter.set_gain(self.tx_gain.value())


class lookup_tables:
    languages = [
        "unknown language", "Albanian", "Breton", "Catalan", "Croatian",
        "Welsh", "Czech", "Danish", "German", "English", "Spanish",
        "Esperanto", "Estonian", "Basque", "Faroese", "French", "Frisian",
        "Irish", "Gaelic", "Galician", "Icelandic", "Italian", "Lappish",
        "Latin", "Latvian", "Luxembourgian", "Lithuanian", "Hungarian",
        "Maltese", "Dutch", "Norwegian", "Occitan", "Polish", "Postuguese",
        "Romanian", "Romansh", "Serbian", "Slovak", "Slovene", "Finnish",
        "Swedish", "Tuskish", "Flemish", "Walloon"
    ]
    programme_types = [
        "No programme type", "News", "Current Affairs", "Information", "Sport",
        "Education", "Drama", "Culture", "Science", "Varied", "Pop Music",
        "Rock Music", "Easy Listening Music", "Light Classical",
        "Serious Classical", "Other Music", "Weather/meteorology",
        "Finance/Business", "Children's programmes", "Social Affairs",
        "Religion", "Phone In", "Travel", "Leisure", "Jazz Music",
        "Country Music", "National Music", "Oldies Music", "Folk Music",
        "Documentary", "None", "None"
    ]
    country_ID_ECC_E0 = [
        "None", "Germany", "Algeria", "Andorra", "Israel", "Italy", "Belgium",
        "Russian Federation", "Azores (Portugal)", "Albania", "Austria",
        "Hungary", "Malta", "Germany", "Canaries (Spain)", "Egypt"
    ]
    protection_EEP_set_A = ["1-A", "2-A", "3-A", "4-A"]


def main():
    app = QtWidgets.QApplication(sys.argv)
    global resolution_width
    resolution_width = app.desktop().screenGeometry().width()
    global resolution_height
    resolution_height = app.desktop().screenGeometry().height()

    form = DABstep()
    form.show()
    app.exec_()


if __name__ == '__main__':
    main()
