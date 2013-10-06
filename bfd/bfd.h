/*
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

#ifndef BFD_H
#define BFD_H

#include "types.h"
#include <stdlib.h>
#include <stdio.h>

#define NUM_SHELVES 64

struct bottleHolder;

struct processRule;

const char* m_freezerGroupLocation="/sys/fs/cgroup/freezer/";
const char* m_speedGroupLocation="/sys/fs/cgroup/cpu/";



Message receiveMessage(FILE* fifoin);
void handleMessage( Message message );

bottle* addBottle( long xid, long pid, bool real);
void removeBottle( bottle* theBottle, bool removeFromList );
bottle* getBottle( long pid );


void addBottleToShelf(long xid, long pid, long shelfNum, bool real);
void removeBottleFromShelf(long pid, long shelfNum);

void setShelfOpenState(bool openState, long shelfNum);

void setupShelfList();

ThawState shelfTemperature( long shelfNum );

void chillShelves();

void addBottlesToProcList();

void addBottleToProcList( bottle* theBottle );



char* getProcessNameFromPid(long pid, char* data, long size);

beer* addBeer( long pid );

void removeBeerNodeFromShelf( beer* theBeer, long shelfNum );

void removeBeerCgroup( beer* theBeer );

void shelfTemperatureChange(long shelfNum, ThawState oldTemp, ThawState newTemp);

void setShelfTemp( long shelfNum, ThawState openTemp, ThawState closeTemp);

void changeMuteBeer(beer* theBeer, bool mute);

void setDenominator(long pid);

inline ThawState maxTemp( ThawState state1, ThawState state2)
{
    if( state1 > state2 )
    {
        return state1;
    }
    else
    {
        return state2;
    }
}

beer* getBeer( long pid );

LIST_HEAD(beer_table, beer) m_beerTable[ 128 ];
LIST_HEAD(bottle_table, bottle) m_bottleTable[ 512 ]; 

shelf m_shelfList[NUM_SHELVES];

long m_shelfTemps[NUM_TEMPERATURES];
long m_openShelves;

inline ThawState shelfTemperature( long shelfNum )
{
    if(m_openShelves & 1L<<shelfNum)
    {
        return m_shelfList[shelfNum].openTemp;
    }
    else
    {
        return m_shelfList[shelfNum].closedTemp;
    }
}

#endif
