/* Java process monitoring, ordered by sampled cpu.
   Copyright (C) 2017-2018 mikejonesey.
   Written by Michael Jones <michael.jones@linux.com>.

   jtop is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   jtop is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the source; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <string.h>

#ifndef JTOP_BUFFERFUN_H
#define JTOP_BUFFERFUN_H

char *jtopstrncpy(char *dest, const char *src, size_t n);

#endif //JTOP_BUFFERFUN_H
