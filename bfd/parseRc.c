/*
 *  Helper for parsing the rc file.
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
#include <string.h>
#include <stdlib.h>
#include "ctype.h"
#include "parseRc.h"
#include "types.h"

void parseRcFile()
{
    FILE* bfdrc = fopen("bfdrc", "r");
    char inString[800];
    processRule** ruleList = malloc(sizeof(processRule*)*1024);
    long numberRules = 0;
    bool variables = false;
    char* lineData;
    while(fgets(inString, 799, bfdrc) )
    {
        lineData = inString;
        while(isspace(lineData[0]))
        {
            lineData++;
        }
        if(lineData[0]=='\0' || lineData[0]=='#')
        {
            continue;
        }
        else if(variables)
        {
            if(strncmp(lineData, "END_VARIABLES", strlen("END_VARIABLES")) ==0 )
            {
               variables = false; 
            }
            else
            {
               processVariableLine(lineData);
            }
        }
        else
        {
            if(strncmp(lineData, "START_VARIABLES", strlen("START_VARIABLES")) ==0 )
            {
               variables=true;
            }
            else
            {
               ruleList[numberRules]=malloc(sizeof(processRule));
               *ruleList[numberRules]=parseRuleLine(lineData, ruleList, numberRules);
               numberRules++;
            }
        }
    }
    m_numberRules=numberRules;
    finalizeRules(ruleList, numberRules);
    free(ruleList);
}

const processRule* slowLookup(char* processName, processRule** ruleList, long numberRules)
{
    for(int i=0; i<numberRules; i++)
    {
        if(strcmp(ruleList[i]->processName, processName) == 0)
            return ruleList[i];
    }
    return 0;
}

processRule parseRuleLine(char* lineData, processRule** ruleList, long numberRules)
{
    processRule ret;
    char* procName=strtok(lineData, " \t");
    char* value='\0';
    char* dashCheck;
    bool ellipsis=false;
    ret.processName=malloc(sizeof(char)*(strlen(procName)+1));
    strcpy(ret.processName, procName);
    for(int i=0; i<NUM_TEMPERATURES; i++)
    {
        if(!ellipsis)
        {
            value = strtok(0, " \t\n");
        }
        if(value[0]=='!')
        {
            char* location;
            if(!ellipsis && (location=strstr(value, "..")))
            {
              ellipsis = true;
              location[0]='\0';
            }

            const processRule* parentRule = slowLookup(value, ruleList, numberRules);

            if(parentRule)
            {
              ret.cpuShares[i] = parentRule->cpuShares[i];
              ret.cpuCap[i] = parentRule->cpuCap[i];
            }
        }
        else
        {
            ret.cpuShares[i] = strtol(value, &dashCheck, 10);
            ret.cpuCap[i] = ret.cpuShares[i];
            ret.muted[i] = 0;
            if(dashCheck)
            {
              if(strncmp(dashCheck, "..",2)==0)
              {
                  ellipsis = true;
              }
              else
              {
                if(dashCheck[0]=='m')
                {
                  ret.muted[i]=1;
                  dashCheck++;
                }
                else
                {
                  ret.cpuCap[i]=strtol(dashCheck+1, &dashCheck, 10);
                  if(dashCheck[0]=='m')
                  {
                    ret.muted[i]=1;
                    dashCheck++;
                  }
                }
                if(strncmp(dashCheck, "..",2) == 0)
                {
                    ellipsis = true;
                }
              }
            }
        }
    }
    return ret;
}

void processVariableLine(char* lineData)
{
    char* varName=strtok(lineData, ": ");
    if( strcmp(varName, "multiplier")==0 )
    {
      m_multiplier=atoi(strtok(0, ": "));
    }
    else if( strcmp(varName, "denominator")==0 )
    {
      m_denominator=atoi(strtok(0, ": "));
    }
    else if( strcmp(varName, "guibias")==0 )
    {
      m_guibias=atoi(strtok(0, ": "));
    }
}

static int compareRule(const void *r1, const void* r2)
{
    return strcmp(((processRule*)r1)->processName, ((processRule*)r2)->processName);
}

void finalizeRules(processRule** ruleList, long numberRules)
{
   //Combine the rules into contiguous memory
   m_ruleList=malloc(sizeof(processRule)*numberRules);
   for(long i=0; i<numberRules; i++)
   {
       m_ruleList[i]=*ruleList[i];
       free(ruleList[i]);
   }
   qsort(m_ruleList, numberRules, sizeof(processRule), compareRule);
   m_defaultRule=lookupRule("!DEFAULT");
}
        
const processRule* lookupRule(const char* processName)
{
    long minIndex=0;
    long maxIndex=m_numberRules-1;
    long middleIndex;
    long comparison;
    while( minIndex<=maxIndex )
    {
        middleIndex = ( minIndex + maxIndex )/2;
        comparison = strcmp( processName, m_ruleList[middleIndex].processName );
        if( comparison==0 )
        {
            return &m_ruleList[middleIndex];
        }
        else if( comparison > 0 )
        {
            minIndex=middleIndex+1;
        }
        else
        {
            maxIndex=middleIndex-1;
        }
    }
    if(strcmp(processName,"!DEFAULT")!=0)
    {
      return m_defaultRule;
    }
    else
    {
      //Missing a default rule. Let's return 0 to stop an infinite loop
      return 0;
    }
}
