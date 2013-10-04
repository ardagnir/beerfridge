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

#ifndef TYPES_H
#define TYPES_H

#include "queue.h"

#define true 1
#define false 0

typedef enum { ICE_COLD, COLD, COOL, NORMAL, HOT, NUM_TEMPERATURES } ThawState;

typedef int bool;

typedef struct shelf
{
    LIST_HEAD(beerNode_list, beerNode) beerList;
    ThawState openTemp;
    ThawState closedTemp;
} shelf;

typedef struct processRule
{
    char* processName;
    long cpuShares[NUM_TEMPERATURES];
    long cpuCap[NUM_TEMPERATURES];
} processRule;

typedef struct beer
{
    long pid;
    long shelves;
    long realBottles;
    long speedShares;
    long speedCap;
    long temperature;
    const processRule* rule; 
    LIST_HEAD(bottle_list, bottle) bottles;
    //struct list_head beerNodes;
    LIST_ENTRY(beer) hashNode;
} beer;

typedef struct beerNode
{
    LIST_ENTRY(beerNode) otherBeers;
    beer* thisBeer;
} beerNode;

typedef struct bottle
{
    long xid;
    long shelves;
    LIST_ENTRY(bottle) bottleList;
    LIST_ENTRY(bottle) hashNode;
    bool real; //if the beer loses all real bottles it is removed
    beer* bottleBeer;
} bottle;

typedef struct Message
{
    char type[16];
    char content[128];
} Message;
#endif
