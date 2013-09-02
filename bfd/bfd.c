/*
 *  The beerfridge demon
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

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>


#include "bfd.h"
#include "parseRc.h"

#define CONT_SIGNAL 18
#define STOP_SIGNAL 19

void assert( bool assertion )
{
  if(!assertion)
  {
    fprintf(stderr, "failed assertion");
  }
}

void system2(char* debug)
{
  printf(debug);
  printf("\n");
  system(debug);
}


int main()
{
    Message message;
    for (int i=0; i<NUM_TEMPERATURES; i++)
    {
        m_shelfTemps[i]=0;
    }
    m_openShelves=0;
    for(long i=0; i<sizeof(m_shelfList)/sizeof(shelf); i++)
    {
        LIST_INIT(&(m_shelfList[i].beerList));
        //Eventually these should be set with messages, but even then, these are probably good defaults:
        m_shelfList[i].closedTemp=COLD;
        m_shelfList[i].openTemp=NORMAL;
        m_shelfTemps[m_shelfList[i].closedTemp]|=i;
    }

    parseRcFile();

    FILE *fifoin = fopen("/tmp/beerfifo", "r");
    while( true )
    {
        message=receiveMessage(fifoin);
        if(!strcmp(message.type,"exit"))
        {
            break;
        }
        if(strcmp(message.type,"NONE")!=0)
        {
            handleMessage( message );
        }
    }
    fclose(fifoin);

    return 0;
}



long hashify( long num, long size )
{
    return (num*37)%size;
}

void stripBeerFridgeCgroup( char* currentCgroup )
{
    char* foundGroup;
    char* nextSlash;
    if( foundGroup = strstr( currentCgroup, "/beerfridge_" ) )
    {
        if( nextSlash = strstr( foundGroup, "/" ) )
        {
            memmove( foundGroup, nextSlash, strlen(nextSlash)+1 );
        }
        else
        {
            foundGroup[0]='\0';
        }
    }
}

void removeBeerCgroup( beer* theBeer )
{
 //Eventually, I should probably do this 
}

void addToCGroup( const char* typeSignifier, char* locationSignifier, char* currentCgroup, long pid, long createDirectory)
{
    char* directoryBuffer;
    char* echoBuffer;
    int errorCheck;
    char* newCheckLoc;
    /*
    while ( currentCgroup!=0 && (newCheckLoc=strstr(currentCgroup,"beerfridge_")))
    {
      currentCgroup=strstr(newCheckLoc,"/");
    }
    */
    stripBeerFridgeCgroup(currentCgroup);

    if( currentCgroup[0]=='\0'|| strcmp(currentCgroup, "/")==0 ) 
    {
      errorCheck = asprintf( &directoryBuffer, "/sys/fs/cgroup/%s/%s", typeSignifier, locationSignifier );
    }
    else
    {
      errorCheck = asprintf( &directoryBuffer, "/sys/fs/cgroup/%s%s/%s", typeSignifier, currentCgroup, locationSignifier );
    }

    assert(errorCheck != -1);

    //Create cgroup
    if(createDirectory)
    {
        char* mkdirBuffer;
        errorCheck = asprintf( &mkdirBuffer, "mkdir -p %s", directoryBuffer);
        assert( errorCheck !=-1 );
        system2( mkdirBuffer );
        free(mkdirBuffer);
    }
      
    //Add process to cgroup
    errorCheck = asprintf( &echoBuffer, "echo %d > %s/tasks", pid, directoryBuffer );
    assert( errorCheck !=-1 );
    system2( echoBuffer );

    free(directoryBuffer);
    free(echoBuffer);
}

void cgroupProcessAndChildrenRecursively( const char* typeSignifier, long pid, long parentPid )
{
    char fileName[128];
    char cgroup[256];
    FILE* cgroupCheck;
    char locationSignifier[64];
    sprintf(locationSignifier,"beerfridge_%d",parentPid);
    //Don't overide the process of a child beer
    if( parentPid != pid && getBeer(pid) )
    {
        return;
    }
    sprintf( fileName, "/proc/%d/cgroup", pid );
    cgroupCheck = fopen( fileName, "r" );
    if(cgroupCheck)
    {
        while( fgets( cgroup, 100, cgroupCheck ) )
        {
           if( strstr( cgroup, typeSignifier) )
           {
               if( strstr( cgroup, locationSignifier ) )
               {
                   //Process is already part of the beer. It got added to the beer by the cgroup and it's children are now trapped.
                   return;
               }
               else
               {
                  break;
               }
           }
        }
    }
    strtok(cgroup,"\n");
    addToCGroup( typeSignifier, locationSignifier, strstr(cgroup,"/"), pid, parentPid==pid);

    char* command;
    asprintf(&command, "ps --ppid %d -o pid=", pid);
    FILE* children = popen(command, "r" );
    free(command);
    if(children)
    {
        char child[64];
        while(fgets(child, 63, children))
        {
           cgroupProcessAndChildrenRecursively( typeSignifier, strtol(child, 0, 10), parentPid);
        }
        
    }
    pclose(children);
}

bottle* getBottle( long xid )
{
    bottle* theBottle;
    long hashVal=hashify( xid, sizeof(m_bottleTable)/sizeof(bottle*) );
    LIST_FOREACH(theBottle, &(m_bottleTable[hashVal]), hashNode)
    {
      if( theBottle->xid == xid ) 
      {
          return theBottle;
      }
    }
    return 0;
}

beer* getBeer( long pid )
{
    beer* theBeer;
    long hashVal=hashify( pid, sizeof(m_beerTable)/sizeof(beer*) );
    LIST_FOREACH(theBeer, &(m_beerTable[hashVal]), hashNode)
    {
      if( theBeer->pid == pid ) 
      {
          return theBeer;
      }
    }
    return 0;
}

void loadTemperatureReactions( beer* theBeer )
{
    char processName[80];
    getProcessNameFromPid(theBeer->pid, processName, 80);
    theBeer->rule=lookupRule(processName);
}


beer* addBeer( long pid )
{
    long hashVal = hashify( pid, sizeof(m_beerTable)/sizeof(beer*) );
    beer* theBeer = (beer*)malloc(sizeof(beer));
    theBeer->pid = pid;
    theBeer->shelves = 0;
    theBeer->realBottles = 0;
    theBeer->speed = 1000;
    //This should probably be an uninitialized value instead.
    theBeer->temperature = HOT;
    loadTemperatureReactions( theBeer );
    LIST_INIT( &(theBeer->bottles) );
    cgroupProcessAndChildrenRecursively( "cpu", pid, pid );
    cgroupProcessAndChildrenRecursively( "freezer", pid, pid );
    LIST_INSERT_HEAD( &(m_beerTable[hashVal]), theBeer, hashNode) ;
    return theBeer;
}

void removeBeer( beer* theBeer )
{
    //Eventually this should store it's beernodes and delete them that way
    for(int shelf=0; shelf<NUM_SHELVES; shelf++)
    {
        if( theBeer->shelves & 1L<<shelf )
        {
            removeBeerNodeFromShelf(theBeer, shelf);
        }
    }

    bottle* theBottle;
    LIST_REMOVE(theBeer, hashNode);
    //delete all bottles
    LIST_FOREACH( theBottle, &(theBeer->bottles), bottleList)
    {
        removeBottle( theBottle, false );
    }
    removeBeerCgroup(theBeer);
    free(theBeer);
}

bottle* addBottle( long xid, long pid, bool real )
{
    long hashVal = hashify( xid, sizeof(m_bottleTable)/sizeof(bottle*) );
    bottle* theBottle = malloc(sizeof(bottle));
    beer* theBeer = getBeer( pid );
    if( !theBeer )
    {
        if(!real)
        {
          //Beers are cleared when they have no real bottles, so this beer would clear right away
          return 0;
        }
        theBeer = addBeer( pid );
    }
    theBottle->xid = xid;
    theBottle->bottleBeer = theBeer;
    theBottle->shelves = 0;
    theBottle->real = real;
    theBottle->bottleBeer->realBottles+=real;
    LIST_INSERT_HEAD( &(theBeer->bottles), theBottle, bottleList);
    LIST_INSERT_HEAD( &(m_bottleTable[hashVal]), theBottle, hashNode );
    return theBottle;
}

void removeBottle( bottle* theBottle, bool removeFromList )
{
    LIST_REMOVE(theBottle, hashNode) ;
    theBottle->bottleBeer->realBottles-=theBottle->real;
    if(removeFromList)
    {
        LIST_REMOVE(theBottle, bottleList);
        if( !theBottle->bottleBeer->realBottles)
        {
            removeBeer( theBottle->bottleBeer );
        }
    }
    free(theBottle);
}

bool updateShelfMembership( beer* theBeer)
{
    bool ret;
    long newShelfMembership=0;
    bottle* theBottle;
    LIST_FOREACH(theBottle, &(theBeer->bottles), bottleList)
    {
        newShelfMembership |= theBottle->shelves;
    }
    ret= ( theBeer->shelves!=newShelfMembership );
    theBeer->shelves=newShelfMembership;
    return ret;
}

void setBeerSpeed( beer* theBeer, long speed )
{
    char* command;
    if( theBeer->speed != speed )
    {
        theBeer->speed = speed;
        asprintf( &command, "find %s -wholename *beerfridge_%d/cpu.cfs_quota_us -exec sh -c '/bin/echo %d > $1' - {} \\;", m_speedGroupLocation, theBeer->pid,  speed*100 );
        system2( command );  
        free( command );
    }
}

void freezeBeer( beer* theBeer )
{
      char* command;
      asprintf( &command, "find %s -wholename *beerfridge_%d/freezer.state -exec sh -c '/bin/echo FROZEN > $1' - {} \\;", m_freezerGroupLocation, theBeer->pid );
      system2( command );
      free( command );
}

void unfreezeBeer( beer* theBeer )
{
      char* command;
      asprintf( &command, "find %s -wholename *beerfridge_%d/freezer.state -exec sh -c '/bin/echo THAWED > $1' - {} \\;", m_freezerGroupLocation, theBeer->pid );
      system2( command );
      free( command );
}

void setTemperature( beer* theBeer, ThawState temperature )
{
    long currentReaction = theBeer->rule->cpuShares[ temperature ];
    long oldReaction = theBeer->rule->cpuShares[ theBeer->temperature ];
    theBeer->temperature = temperature;
    if( currentReaction== 0 )
    {
        if( oldReaction != 0 )
        {
            freezeBeer( theBeer );
        }
    }
    else
    {
        setBeerSpeed( theBeer, theBeer->rule->cpuShares[ temperature ] );
        if( oldReaction == 0 )
        {
            unfreezeBeer( theBeer );
        }
    }
}

void updateTemperature( beer* theBeer )
{
   long openMembership;
   if( openMembership = theBeer->shelves & m_openShelves )
   {
        for( int i=(int)ICE_COLD; i<NUM_TEMPERATURES; i++ )
        {
            if( m_shelfTemps[i] & openMembership )
            {
                setTemperature( theBeer, (ThawState)i );
                return;
            }
        }
   }
   for( int i=(int)NUM_TEMPERATURES-1; i>=ICE_COLD; i-- )
   {
        if( m_shelfTemps[i] & theBeer->shelves )
        {
            setTemperature( theBeer, (ThawState)i );
            return;
        }
   }
}

void addBottleToShelf( long xid, long pid, long shelfNum, bool real )
{
    bottle* theBottle=getBottle(xid);
    if( !theBottle )
    {
        theBottle = addBottle(xid, pid, real );
    }
    theBottle->shelves |= 1L<<shelfNum;
    if( !(theBottle->bottleBeer->shelves & 1L<<shelfNum) )
    {
        theBottle->bottleBeer->shelves |= 1L<<shelfNum;
        beerNode* theNode = malloc(sizeof(beerNode));
        theNode->thisBeer=theBottle->bottleBeer;
        LIST_INSERT_HEAD( &(m_shelfList[shelfNum].beerList), theNode, otherBeers );
        updateTemperature( theBottle->bottleBeer );
    }
}

void removeBottleFromShelf( long xid, long shelfNum )
{
    bottle* theBottle=getBottle(xid);
    if( theBottle )
    {
        theBottle->shelves &= ~(1L<<shelfNum);
        if( updateShelfMembership(theBottle->bottleBeer) )
        {
            updateTemperature( theBottle->bottleBeer );
            removeBeerNodeFromShelf( theBottle->bottleBeer, shelfNum );
        }
        if( theBottle->shelves == 0 )
        {
            removeBottle( theBottle, true );
        }
    }
}

void removeBeerNodeFromShelf( beer* theBeer, long shelfNum )
{
    beerNode* theBeerNode;
    beerNode* theBeerNode2;
    //Swap this out for an 0(1) algorithm
    LIST_FOREACH_SAFE(theBeerNode, &(m_shelfList[shelfNum].beerList), otherBeers, theBeerNode2)
    {
       if(theBeerNode->thisBeer == theBeer)
       {
          LIST_REMOVE(theBeerNode, otherBeers);
          free(theBeerNode);
       }
    }
}

void updateShelfTemperature( long shelfNum )
{
    beerNode* theBeerNode;
    LIST_FOREACH(theBeerNode, &(m_shelfList[shelfNum].beerList), otherBeers)
    {
        updateTemperature( theBeerNode->thisBeer );
    }
}

//if old and new temp are the same it gets temporarily set to the wrong value but that doesn't matter since it's single-threaded
void openShelf( long shelfNum )
{  
    long* oldTempInfo=&(m_shelfTemps[ m_shelfList[shelfNum].closedTemp ]); 
    long* newTempInfo=&(m_shelfTemps[ m_shelfList[shelfNum].openTemp ]); 
    (*oldTempInfo) &= ~( 1L<<shelfNum );
    (*newTempInfo) |= 1L<<shelfNum;
    m_openShelves |= 1L<<shelfNum;
    updateShelfTemperature(shelfNum);
}

void closeShelf( long shelfNum )
{  
    long* oldTempInfo=&(m_shelfTemps[ m_shelfList[shelfNum].openTemp ]); 
    long* newTempInfo=&(m_shelfTemps[ m_shelfList[shelfNum].closedTemp ]); 
    (*oldTempInfo) &= ~( 1L<<shelfNum );
    (*newTempInfo) |= 1L<<shelfNum;
    m_openShelves &= ~( 1L<<shelfNum );
    updateShelfTemperature(shelfNum);
}

Message receiveMessage(FILE* fifoin)
{
    char buffer[128];
    Message returnMessage;
    bool receivedMessage=false;
    bool error=false;
    while(!fgets(buffer,160,fifoin))
    {
      usleep(100000);
    }
    char* type=strtok(buffer," \n");
    if(type)
    {
      strncpy(returnMessage.type, type, 15);
      returnMessage.type[15]='\0';

      char* message=strtok(0,"\n");
      if(message)
      {
        strncpy(returnMessage.content, message,127);
        returnMessage.content[127]='\0';
        receivedMessage=true;
        printf("%s %s\n", returnMessage.type, returnMessage.content);
      }
      else
      {
        error=true;
      }
    }
    else
    {
      error=true;
    }
    if(error)
    {
      strcpy(returnMessage.type,"NONE");
    }

    return returnMessage;
}

void handleMessage(Message message)
{
        if(!strcmp(message.type,"echo"))
        {
            printf(message.content);
        }
        else if (!strcmp(message.type,"addFakeBottle")) 
        {
            char* endPoint;
            long xid = strtol(message.content, &endPoint, 10);
            long pid = strtol(endPoint, &endPoint, 10);
            long shelfNum = strtol(endPoint, NULL, 10);

            addBottleToShelf( xid, pid, shelfNum, 0 );
        }
        else if (!strcmp(message.type,"addBottle")) 
        {
            char* endPoint;
            long xid = strtol(message.content, &endPoint, 10);
            long pid = strtol(endPoint, &endPoint, 10);
            long shelfNum = strtol(endPoint, NULL, 10);

            addBottleToShelf( xid, pid, shelfNum, true );
        }
        else if (!strcmp(message.type,"removeBottle"))
        {
            char* endPoint;
            long xid = strtol(message.content, &endPoint, 10);
            long shelfNum = strtol(endPoint, NULL, 10);
            removeBottleFromShelf( xid, shelfNum );
        }
    
        else if (!strcmp(message.type,"openShelf"))
        {
            long shelfNum = strtol(message.content, NULL, 10);
            openShelf( shelfNum );
        }
        else if (!strcmp(message.type,"closeShelf"))
        {
            long shelfNum = strtol(message.content, NULL, 10);
            closeShelf( shelfNum );
        }
        else
        {
            fprintf(stderr, "Unknown message type: %s\n", message.type);
        }
}


char* getProcessNameFromPid(long pid, char* data, long size)
{
    char* fileName;

    asprintf( &fileName, "/proc/%d/comm", pid );
    FILE* file = fopen(fileName, "rt");
    fgets(data, size, file);

    //Remove trailing newline
    long length=strlen(data);
    if(length>0)
    {
      data[length-1]='\0';
    }

    free(fileName);
    return data;
}
