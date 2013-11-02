/*
 *  The beerfridge client.
 *  
 *  Copyright (C) 2013, James Kolb. All rights reserved.
 *
 *  This file is part of Beerfridge.
 *
 *  Beerfridge is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Beerfridge is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with Beerfridge.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>

int main(int argc, char **argv)
{
    if(argc<=1)
    {
      return 0;
    }
    char buffer[100];
    FILE* fifo = fopen("/tmp/beerfifo", "w");
    if(fifo)
    {
      long cursor=0;
      cursor += snprintf(buffer+cursor, 99, "%s", argv[1]);
      for( int i=2; i<argc; i++ )
      {
          cursor+=snprintf(buffer+cursor, 99-cursor, " %s", argv[i]);
      }
      fprintf(fifo, "%s\n", buffer);
      fclose(fifo);
    }
    return 0;
}
