# Clevo-xSM-wmi

Kernel module for keyboard backlighting of Clevo SM series notebooks.

Based upon tuxedo-wmi, created by Christoph Jaeger.
http://www.linux-onlineshop.de/forum/index.php?page=Thread&threadID=26

### Additions over tuxedo-wmi
* Sysfs interface to control the brightness, mode, colour,
  on/off state after the module has loaded.
  In the original code you can only set these before the module loads.
* Small application to visually control the keyboard lighting using the sysfs
  interface.
* Cycle through colours rather than modes with the keyboard key.

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
