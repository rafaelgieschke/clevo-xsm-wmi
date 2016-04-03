# clevo-xsm-wmi

Kernel module for keyboard backlighting of Clevo SM series notebooks.
(And the P150EM, P750ZM, P750DM(-G), P770DM(-G) models)

Based upon tuxedo-wmi, created by TUXEDO Computers GmbH.
http://www.linux-onlineshop.de/forum/index.php?page=Thread&threadID=26

### Additions over tuxedo-wmi
* Sysfs interface to control the brightness, mode, colour,
  on/off state after the module has loaded.
  In the original code you can only set these before the module loads.
* Small QT based application to visually control the keyboard lighting using the sysfs interface.
* Cycle through colours rather than modes with the keyboard key.
* Experimental support for touchpad illumination / lower led bar on the front of the machine.

### Building

Dependencies:

* standard compile stuff (c compiler, make, etc)
* linux-headers
* qt5-base (for utility)

Fedora 22+ Dependencies:

* use qmake-qt5 instead of qmake
* install qt5-qt3d-devel with DNF

Building:
```bash
# For the module
$ cd module && make && sudo make install

# For the utility
$ cd utility && qmake && make
$ sudo install -Dm755 clevo-xsm-wmi /usr/bin/clevo-xsm-wmi
$ sudo install -Dm755 systemd/clevo-xsm-wmi.service /usr/lib/systemd/system/clevo-xsm-wmi.service
```

### Usage

Adjusting keyboard settings:
```bash
$ sudo clevo-xsm-wmi
```

Restoring state during boot:
```bash
# With the module:
$ modinfo clevo-xsm-wmi
$ sudo tee /etc/modprobe.d/clevo-xsm-wmi.conf <<< options clevo-xsm-wmi kb_color=white,white,white kb_brightness=1

# With systemd:
$ sudo systemctl enable clevo-xsm-service

# With others:
# run '/usr/bin/clevo-xsm-wmi -r' at boot to restore state
# run '/usr/bin/clevo-xsm-wmi -s' at shutdown/change to save state
```

### Distributions

Arch Linux: [Module](https://aur.archlinux.org/packages/clevo-xsm-wmi/) [DKMS](https://aur.archlinux.org/packages/clevo-xsm-wmi-dkms/) [Utility](https://aur.archlinux.org/packages/clevo-xsm-wmi-util/)

### Common issues

* 'Can't read private key' during 'make install'.

This is a common issue on Ubuntu as the module isn't signed so manual install is required.
```bash
$ sudo install -m644 clevo-xsm-wmi.ko /lib/modules/$(uname -r)/extra
$ sudo depmod
```
* Module installed but not loaded during boot.

Specify that the module should be loaded just in case.
```bash
$ sudo tee /etc/modules-load.d/clevo-xsm-wmi.conf <<< clevo-xsm-wmi
```

* I'm absolutely positive that the module is installed but still nothing happens!
```bash
$ sudo modprobe clevo-xsm-wmi
$ dmesg | grep clevo
```
If it returns no line such as 'clevo_xsm_wmi: Model Clevo P15SM found', then your model isn't supported.
Please open an issue and supply the output of 'uname -a', 'sudo dmidecode' and 'dmesg | grep clevo'.

### License
This program is free software;  you can redistribute it and/or modify
it under the terms of the  GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is  distributed in the hope that it  will be useful, but
WITHOUT  ANY   WARRANTY;  without   even  the  implied   warranty  of
MERCHANTABILITY  or FITNESS FOR  A PARTICULAR  PURPOSE.  See  the GNU
General Public License for more details.

You should  have received  a copy of  the GNU General  Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
