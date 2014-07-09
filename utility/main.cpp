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

#include "mainwindow.h"
#include <QApplication>

keyboard_s keyboard_settings;

#undef C
#define C(n) { .name = #n }
kbcolors_s kb_colors[] = COLORS;
#undef C

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}

void readKeyboardValues() {
    /* Brightness */
    QFile file("/sys/devices/platform/clevo_xsm_wmi/kb_brightness");
    if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray line = file.readAll();
        int value = line.at(0) - 48;
        if(value == 1) {
            if((line.at(1) - 48) == 0)
                value = 10;
        }
        if(value >= 0 && value <= 10)
            keyboard_settings.brightness = value;
        else
            keyboard_settings.brightness = 0;
        file.close();
    } else {
        QMessageBox msgBox;
        msgBox.setText("Failed to read brightness");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }
    /* Mode */
    QFile file2("/sys/devices/platform/clevo_xsm_wmi/kb_mode");
    if(file2.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray line = file2.readAll();
        int value = line.at(0) - 48;
        if(value >= 0 && value <= 7)
            keyboard_settings.mode = value;
        else
            keyboard_settings.mode = 0;
        file2.close();
    } else {
        QMessageBox msgBox;
        msgBox.setText("Failed to read mode");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }
    /* color */
    QFile file3("/sys/devices/platform/clevo_xsm_wmi/kb_color");
    if(file3.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray line = file3.readAll();
        char left[8], right[8], center[8];
        int i = sscanf(line, "%7s %7s %7s", left, center, right);
        if(i == 3) {
            for (unsigned int j = 0;
                j < (sizeof(kb_colors) / sizeof((kb_colors)[0])); j++) {
                if (!strcmp(left, kb_colors[j].name))
                    keyboard_settings.color_l = j;
                if (!strcmp(center, kb_colors[j].name))
                    keyboard_settings.color_c = j;
                if (!strcmp(right, kb_colors[j].name))
                    keyboard_settings.color_r = j;
            }
        }
        file3.close();
    } else {
        QMessageBox msgBox;
        msgBox.setText("Failed to read colors");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }
}

void setKeyboardValues() {
    QFile file("/sys/devices/platform/clevo_xsm_wmi/kb_brightness");
    if(file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << keyboard_settings.brightness;
        file.close();
    } else {
        QMessageBox msgBox;
        msgBox.setText("Failed to set brightness");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }
    QFile file2("/sys/devices/platform/clevo_xsm_wmi/kb_mode");
    if(file2.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file2);
        out << keyboard_settings.mode;
        file2.close();
    } else {
        QMessageBox msgBox;
        msgBox.setText("Failed to set mode");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }
    QFile file3("/sys/devices/platform/clevo_xsm_wmi/kb_color");
    if(file3.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file3);
        out << kb_colors[keyboard_settings.color_l].name
            << " " << kb_colors[keyboard_settings.color_c].name
            << " " << kb_colors[keyboard_settings.color_r].name;
        file3.close();
    } else {
        QMessageBox msgBox;
        msgBox.setText("Failed to set colors");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }
}
