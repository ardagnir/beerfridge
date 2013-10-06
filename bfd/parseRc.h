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

#ifndef PARSERC_H
#define PARSERC_H

#include "types.h"

void parseRcFile();

const processRule* lookupRule(const char* processName);

static const processRule* slowLookup(char* processName, processRule** ruleList, long numberRules);

static processRule parseRuleLine(char* inString, processRule** ruleList, long numberRules);

static void processVariableLine(char* inString);

static void finalizeRules(processRule** ruleList, long numberRules);
        
long m_multiplier;
long m_denominator;
long m_guibias;
static processRule* m_ruleList;
static long m_numberRules;

static const processRule* m_defaultRule;
#endif
