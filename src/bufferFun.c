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

// Buffer protection wrapper functions

#include "bufferFun.h"

// dest[10] = "bob", src = "alice", n=5 == bobal to boba\0
// dest[10] = "bob", src = "alice", n=10 == bobalice\0\0 == bobalice\0\0
// dest[2] = "bob", src = "alice", n=10 == bo == memory edited outside bob... <<<<
// dest[10] = "bob", src = "reallylongstring", n=10 == boreallylo == boreallylo\0
char *jtopstrncpy(char *dest, const char *src, size_t n){
    strncpy(dest, src, n - 1);
    if (n > 0)
        dest[n - 1]= '\0';
    return dest;
}
