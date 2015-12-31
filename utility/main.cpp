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
#include <QSettings>
#include <QFile>
#include <getopt.h>

keyboard_s keyboard_settings;
bool has_lower = true;

#undef C
#define C(n) { .name = #n }
kbcolors_s kb_colors[] = COLORS;
#undef C

int main(int argc, char *argv[]) {
    int opt = 0;

	while ((opt = getopt(argc, argv, "sr")) != -1)
		switch(opt) {
			case 's':
				saveKeyboardSettings();
				return 0;
				break;
			case 'r':
				restoreKeyboardSettings();
				return 0;
				break;
		}


    QApplication a(argc, argv);
    MainWindow w;

    w.show();

    return a.exec();
}

void readKeyboardValues() {
    /* Brightness */
    QFile fd_brightness("/sys/devices/platform/clevo_xsm_wmi/kb_brightness");
    if(fd_brightness.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray line = fd_brightness.readAll();
        int value = line.at(0) - 48;
        if(value == 1) {
            if((line.at(1) - 48) == 0)
                value = 10;
        }
        if(value >= 0 && value <= 10)
            keyboard_settings.brightness = value;
        else
            keyboard_settings.brightness = 0;
        fd_brightness.close();
    } else {
        QMessageBox msgBox;
        msgBox.setText("Failed to read brightness");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }
    /* Mode */
    QFile fd_mode("/sys/devices/platform/clevo_xsm_wmi/kb_mode");
    if(fd_mode.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray line = fd_mode.readAll();
        int value = line.at(0) - 48;
        if(value >= 0 && value <= 7)
            keyboard_settings.mode = value;
        else
            keyboard_settings.mode = 0;
        fd_mode.close();
    } else {
        QMessageBox msgBox;
        msgBox.setText("Failed to read mode");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }
    /* color */
    QFile fd_color("/sys/devices/platform/clevo_xsm_wmi/kb_color");
    if(fd_color.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray line = fd_color.readAll();
        char left[8], right[8], center[8], lower[8];
        int i = sscanf(line, "%7s %7s %7s %7s", left, center, right, lower);
        if(i == 3)
            has_lower = false;
        if(i == 3 || i == 4) {
            for (unsigned int j = 0;
                j < (sizeof(kb_colors) / sizeof((kb_colors)[0])); j++) {
                if (!strcmp(left, kb_colors[j].name))
                    keyboard_settings.color_left = j;
                if (!strcmp(center, kb_colors[j].name))
                    keyboard_settings.color_center = j;
                if (!strcmp(right, kb_colors[j].name))
                    keyboard_settings.color_right = j;
                if (!strcmp(lower, kb_colors[j].name))
                    keyboard_settings.color_lower = j;
            }
        }
        fd_color.close();
    } else {
        QMessageBox msgBox;
        msgBox.setText("Failed to read colors");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }
}

void setKeyboardValues() {
    QFile fd_brightness("/sys/devices/platform/clevo_xsm_wmi/kb_brightness");
    if(fd_brightness.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&fd_brightness);
        out << keyboard_settings.brightness;
        fd_brightness.close();
    } else {
        QMessageBox msgBox;
        msgBox.setText("Failed to set brightness");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }
    QFile fd_mode("/sys/devices/platform/clevo_xsm_wmi/kb_mode");
    if(fd_mode.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&fd_mode);
        out << keyboard_settings.mode;
        fd_mode.close();
    } else {
        QMessageBox msgBox;
        msgBox.setText("Failed to set mode");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }
    QFile fd_color("/sys/devices/platform/clevo_xsm_wmi/kb_color");
    if(fd_color.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&fd_color);
        out << kb_colors[keyboard_settings.color_left].name
            << " " << kb_colors[keyboard_settings.color_center].name
            << " " << kb_colors[keyboard_settings.color_right].name
            << " " << kb_colors[keyboard_settings.color_lower].name;
        fd_color.close();
    } else {
        QMessageBox msgBox;
        msgBox.setText("Failed to set colors");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }
}

void saveKeyboardSettings() {
    readKeyboardValues();

    QSettings settings("/etc/clevo-xsm.ini", QSettings::IniFormat);

    if(settings.isWritable()) {
        settings.beginGroup("settings");

        settings.setValue("kb_brightness", keyboard_settings.brightness);
        settings.setValue("kb_mode", keyboard_settings.mode);
        settings.setValue("kb_color_left", kb_colors[keyboard_settings.color_left].name);
        settings.setValue("kb_color_center", kb_colors[keyboard_settings.color_center].name);
        settings.setValue("kb_color_right", kb_colors[keyboard_settings.color_right].name);
        settings.setValue("kb_color_lower", kb_colors[keyboard_settings.color_lower].name);

        settings.endGroup();
    }
}

void restoreKeyboardSettings() {
    QFile fd_settings("/etc/clevo-xsm.ini");
    if(fd_settings.exists()) {
        QSettings settings("/etc/clevo-xsm.ini", QSettings::IniFormat);
        settings.beginGroup("settings");
        const QStringList childKeys = settings.childKeys();

        keyboard_settings.brightness = settings.value("kb_brightness").toInt();
        keyboard_settings.mode       = settings.value("kb_mode").toInt();

        QByteArray bcolor_left      = settings.value("kb_color_left").toString().toUtf8();
        const char* color_left      = bcolor_left.constData();
        QByteArray bcolor_center    = settings.value("kb_color_center").toString().toUtf8();
        const char* color_center    = bcolor_center.constData();
        QByteArray bcolor_right     = settings.value("kb_color_right").toString().toUtf8();
        const char* color_right     = bcolor_right.constData();
        QByteArray bcolor_lower     = settings.value("kb_color_lower").toString().toUtf8();
        const char* color_lower     = bcolor_lower.constData();

        for (unsigned int j = 0;
            j < (sizeof(kb_colors) / sizeof((kb_colors)[0])); j++) {
            if (!strcmp(color_left, kb_colors[j].name))
                keyboard_settings.color_left = j;
            if (!strcmp(color_center, kb_colors[j].name))
                keyboard_settings.color_center = j;
            if (!strcmp(color_right, kb_colors[j].name))
                keyboard_settings.color_right = j;
            if (!strcmp(color_lower, kb_colors[j].name))
                keyboard_settings.color_lower = j;
        }

        settings.endGroup();

        setKeyboardValues();
    }
}
