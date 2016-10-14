/*
 * This file is part of the clevo-xsm-wmi utility
 *
 * Copyright (C) 2014-2016 Arnoud Willemsen <mail@lynthium.com>
 *
 * This program is free software;  you can redistribute it and/or modify
 * it under the terms of the  GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is  distributed in the hope that it  will be useful, but
 * WITHOUT  ANY   WARRANTY;  without   even  the  implied   warranty  of
 * MERCHANTABILITY  or FITNESS FOR  A PARTICULAR  PURPOSE.  See  the GNU
 * General Public License for more details.
 *
 * You should  have received  a copy of  the GNU General  Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    for (unsigned int j = 0; j < COLORS_AMOUNT; j++) {
        ui->selectLeft->addItem(kb_colors[j].name);
        ui->selectCenter->addItem(kb_colors[j].name);
        ui->selectRight->addItem(kb_colors[j].name);
        ui->selectExtra->addItem(kb_colors[j].name);
    }

    readKeyboardValues();

    ui->notifyFailure->hide();

    if(keyboard_settings.mode == -1) {
        ui->group_mode->hide();
        ui->notifyFailure->show();
    }
    if(keyboard_settings.state == -1) {
        ui->stateCheckBox->hide();
        ui->notifyFailure->show();
    }
    if(keyboard_settings.color_left == -1) {
        ui->group_color->hide();
        ui->notifyFailure->show();
    }
    if(keyboard_settings.brightness == -1) {
        ui->group_brightness->hide();
        ui->notifyFailure->show();
    }

    if(has_extra == false) {
        ui->selectExtra->hide();
        ui->labelExtraColor->hide();
    }

    MainWindow::updateUI();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_pushButton_clicked() {
    QApplication::quit();
}

void MainWindow::on_buttonApply_clicked() {
    setKeyboardValues();
}

void MainWindow::updateUI() {
    ui->slider_brightness->setValue(keyboard_settings.brightness);

    switch(keyboard_settings.mode) {
        case 0:
            ui->radio_random_color->setChecked(true);
            break;
        case 1:
            ui->radio_custom->setChecked(true);
            break;
        case 2:
            ui->radio_breathe->setChecked(true);
            break;
        case 3:
            ui->radio_cycle->setChecked(true);
            break;
        case 4:
            ui->radio_wave->setChecked(true);
            break;
        case 5:
            ui->radio_dance->setChecked(true);
            break;
        case 6:
            ui->radio_tempo->setChecked(true);
            break;
        case 7:
            ui->radio_flash->setChecked(true);
            break;
        default:
            ui->radio_custom->setChecked(true);
            keyboard_settings.mode = 1;
            break;
    }

    ui->selectLeft->setCurrentIndex(keyboard_settings.color_left);
    ui->selectCenter->setCurrentIndex(keyboard_settings.color_center);
    ui->selectRight->setCurrentIndex(keyboard_settings.color_right);
    ui->selectExtra->setCurrentIndex(keyboard_settings.color_extra);
    ui->stateCheckBox->setChecked(keyboard_settings.state == 1);
}

void MainWindow::on_slider_brightness_valueChanged(int value) {
    keyboard_settings.brightness = value;
}

void MainWindow::on_radio_random_color_clicked(bool checked) {
    if(checked)
        keyboard_settings.mode = 0;
}
void MainWindow::on_radio_custom_clicked(bool checked) {
    if(checked)
        keyboard_settings.mode = 1;
}
void MainWindow::on_radio_breathe_clicked(bool checked) {
    if(checked)
        keyboard_settings.mode = 2;
}
void MainWindow::on_radio_cycle_clicked(bool checked) {
    if(checked)
        keyboard_settings.mode = 3;
}
void MainWindow::on_radio_wave_clicked(bool checked) {
    if(checked)
        keyboard_settings.mode = 4;
}
void MainWindow::on_radio_dance_clicked(bool checked) {
    if(checked)
        keyboard_settings.mode = 5;
}
void MainWindow::on_radio_tempo_clicked(bool checked) {
    if(checked)
        keyboard_settings.mode = 6;
}
void MainWindow::on_radio_flash_clicked(bool checked) {
    if(checked)
        keyboard_settings.mode = 7;
}

void MainWindow::on_selectLeft_currentIndexChanged(int index) {
    if(index >= 0 && index <= COLORS_AMOUNT)
        keyboard_settings.color_left = index;
}
void MainWindow::on_selectCenter_currentIndexChanged(int index) {
    if(index >= 0 && index <= COLORS_AMOUNT)
        keyboard_settings.color_center = index;
}
void MainWindow::on_selectRight_currentIndexChanged(int index) {
    if(index >= 0 && index <= COLORS_AMOUNT)
        keyboard_settings.color_right = index;
}
void MainWindow::on_selectExtra_currentIndexChanged(int index) {
    if(index >= 0 && index <= COLORS_AMOUNT)
        keyboard_settings.color_extra = index;
}

void MainWindow::on_stateCheckBox_clicked(bool checked) {
    if (checked)
        keyboard_settings.state = 1;
    else
        keyboard_settings.state = 0;
}
