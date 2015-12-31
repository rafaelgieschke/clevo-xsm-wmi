/*
 * This file is part of the clevo-xsm-wmi utility
 *
 * Copyright (C) 2014 Arnoud Willemsen <mail@lynthium.com>
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define COLORS { C(black), C(blue), C(red), C(magenta), C(green), C(cyan), C(yellow), C(white) }
#define COLORS_AMOUNT 8

#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    void updateUI();
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

    void on_buttonApply_clicked();

    void on_slider_brightness_valueChanged(int value);

    void on_radio_tempo_clicked(bool checked);

    void on_radio_breathe_clicked(bool checked);

    void on_radio_custom_clicked(bool checked);

    void on_radio_cycle_clicked(bool checked);

    void on_radio_dance_clicked(bool checked);

    void on_radio_flash_clicked(bool checked);

    void on_radio_random_color_clicked(bool checked);

    void on_radio_wave_clicked(bool checked);

    void on_selectLeft_currentIndexChanged(int index);

    void on_selectCenter_currentIndexChanged(int index);

    void on_selectRight_currentIndexChanged(int index);

    void on_selectLower_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
};

extern bool has_lower;

struct keyboard_s {
    int brightness;
    int mode;
    int color_left;
    int color_center;
    int color_right;
    int color_lower;
};
extern keyboard_s keyboard_settings;

struct kbcolors_s {
    const char *const name;
};
extern kbcolors_s kb_colors[];

void readKeyboardValues();
void setKeyboardValues();
void saveKeyboardSettings();
void restoreKeyboardSettings();

#endif // MAINWINDOW_H
