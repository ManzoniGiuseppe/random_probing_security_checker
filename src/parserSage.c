//This file is part of the program Random Probing Security Checker, which checks the random probing security properties of a given gadget
//Copyright (C) 2022  Giuseppe Manzoni
//This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "parserSage.h"
#include "mem.h"


//#define MAX_LEN_VAR_NAMES
//#define MAX_FILE_SIZE


// assume:
//  - names are alphanumeric
//  - the # directives have no indentation
//  - any other # is a line-long comment
//  - only binary + and *


static bool prefix(char *pre, char *str){
  return strncmp(pre, str, strlen(pre)) == 0;
}

static char *readAndSimplify(char *fileName){
  FILE *fd = fopen(fileName, "rb");
  if(!fd) FAIL("Trying to open $s\n%s\n", fileName, strerror(errno));

  char *file = mem_calloc(sizeof(char), MAX_FILE_SIZE + 1, "parserSage_parse's initial file");
  long fileSize = fread(file, 1, MAX_FILE_SIZE, fd);
  fclose(fd);

  { // reduce memory consumption
    char *newFile = mem_calloc(sizeof(char), fileSize+2, "parserSage_parse's file");
    newFile[0] = '\n';
    strcpy(newFile+1, file);
    mem_free(file);
    file = newFile;
  }

  // make more regular
  for(char *pos = file; *pos; pos++){
    if(*pos == '\r') *pos = '\n';
    if(*pos == '\v') *pos = ' ';
    if(*pos == '\t') *pos = ' ';
  }

  { // remove comments
    char *pos = file;
    bool isComment = 0;
    while(*(++pos)){
      if(isComment){
        if(*pos != '\n'){
          *pos = ' ';
        }else{
          isComment = 0;
        }
      }

      if(*pos != '#') continue;
      if(prefix("\n#IN ", pos-1)) continue;
      if(prefix("\n#OUT ", pos-1)) continue;
      if(prefix("\n#SHARES ", pos-1)) continue;
      if(prefix("\n#RANDOMS ", pos-1)) continue;
      isComment = 1;
      *pos = ' ';
    }
  }

  return file;
}

static size_t wordLen(char *str){
  size_t ret = 0;
  while(isalnum(str[ret])) ret++;
  return ret;
}

static int findWire(int wires_num, char wires_n[][MAX_LEN_VAR_NAMES], char *str){
  for(int i = 0; i < wires_num; i++)
    if(prefix(wires_n[i], str) && wordLen(str) == strlen(wires_n[i]))
      return i;
  return wires_num;
}

static T__THREAD_SAFE void parser_fn(void *fn_info, gadget_fnBuilder_t builder);

gadget_raw_t parserSage_parse(char *fileName){
  return (gadget_raw_t){
    .fn_info = readAndSimplify(fileName),
    .fn = parser_fn,
    .free = mem_free
  };
}

static T__THREAD_SAFE void parser_fn(void *fn_info, gadget_fnBuilder_t builder){
  char *fileP = fn_info;
  char *file = mem_calloc(sizeof(char), strlen(fileP) + 1, "parserSage_parse's temporary file");
  strcpy(file, fileP);

  wire_t maxNumTotOuts = 0;
  for(size_t i = 0; file[i] != 0; i++)
    if(file[i] == '=')
      maxNumTotOuts++; // one for the output, two for inputs
  maxNumTotOuts *= 3;

  char *pos = & file[0];

  char *inLine = NULL;
  char *outLine = NULL;
  char *dLine = NULL;
  char *rndLine = NULL;

  while(!inLine || !outLine || !dLine || !rndLine){
    if(!*pos) FAIL("EOF before initial parameters!\n")
    else if(*pos == '\n' || *pos == ' '){
      pos++;
    }
    else if(prefix("#IN ", pos)){
      if(inLine) FAIL("Line with #IN already present!\n")
      inLine = pos;
      while(*pos && *pos != '\n') pos++;
      if(*pos) *(pos++) = 0;
    }
    else if(prefix("#OUT ", pos)){
      if(outLine) FAIL("Line with #OUT already present!\n")
      outLine = pos;
      while(*pos && *pos != '\n') pos++;
      if(*pos) *(pos++) = 0;
    }
    else if(prefix("#RANDOMS ", pos)){
      if(rndLine) FAIL("Line with #RANDOMS already present!\n")
      rndLine = pos;
      while(*pos && *pos != '\n') pos++;
      if(*pos) *(pos++) = 0;
    }
    else if(prefix("#SHARES ", pos)){
      if(dLine) FAIL("Line with #SHARES already present!\n")
      dLine = pos;
      while(*pos && *pos != '\n') pos++;
      if(*pos) *(pos++) = 0;
    }
    else{
      char out[11];
      memcpy(out, pos, 10);
      out[10] = '\0';
      FAIL("Reached non initial parameter while looking for them! pos=%s\n",out)
    }
  }

  wire_t d = atoi(dLine + strlen("#SHARES "));

  char wires_n[maxNumTotOuts][MAX_LEN_VAR_NAMES];
  gadget_wireId_t wires_v[maxNumTotOuts];
  wire_t wires_size = 0;

  wire_t numIns = 0;
  for(char *it = inLine + strlen("#IN "); *it;){
    if(*it == ' '){
      it++;
      continue;
    }
    int len = wordLen(it);
    for(wire_t i = 0; i < d; i++){
      strncpy(wires_n[wires_size], it, len);
      sprintf(wires_n[wires_size] + len, "%ld", (long) i);
      wires_v[wires_size] = builder.in(builder.state, numIns, i);
      wires_size++;
    }
    numIns++;
    it += len;
  }

  for(char *it = rndLine + strlen("#RANDOMS "); *it;){
    if(*it == ' '){
      it++;
      continue;
    }
    int len = wordLen(it);
    strncpy(wires_n[wires_size], it, len);
    wires_n[wires_size][len] = 0;
    wires_v[wires_size] = builder.rnd(builder.state);
    wires_size++;
    it += len;
  }

  char outs[maxNumTotOuts][MAX_LEN_VAR_NAMES];
  wire_t numOuts = 0;
  for(char *it = outLine + strlen("#OUT "); *it;){
    if(*it == ' '){
      it++;
      continue;
    }
    int len = wordLen(it);
    for(wire_t i = 0; i < d; i++){
      strncpy(outs[numOuts * d + i], it, len);
      sprintf(outs[numOuts * d + i] + len, "%ld", (long) i);
    }
    numOuts++;
    it += len;
  }

  while(*pos){
    while(*pos == ' ' || *pos == '\n') pos++;
    if(!*pos) break;

    wire_t ret = findWire(wires_size, wires_n, pos);
    if(ret == wires_size){
      strncpy(wires_n[wires_size], pos, wordLen(pos));
      wires_n[wires_size][wordLen(pos)] = 0;
    }
    pos += wordLen(pos);
    while(*pos == ' ') pos++;
    if(*pos != '=') FAIL("Illegal character %c in the middle of an assignment, expected '='\n", *pos);
    pos++; // go past the =
    while(*pos == ' ') pos++;
    if(!*pos) FAIL("EOF in the middle of an assignment!\n");

    wire_t v1 = findWire(wires_size, wires_n, pos);
    if(v1 == wires_size) FAIL("The input 1 of an expression was never defined!\n");
    pos += wordLen(pos);
    while(*pos == ' ') pos++;
    if(*pos != '+' && *pos != '*') FAIL("Illegal character in the middle of an assignment!\n");

    bool isSumElseProd = (*pos == '+');
    pos++; // go past the + or *
    while(*pos == ' ') pos++;
    if(!*pos) FAIL("EOF in the middle of an assignment!\n");

    wire_t v2 = findWire(wires_size, wires_n, pos);
    if(v2 == wires_size) FAIL("The input 2 of an expression was never defined!\n");
    pos += wordLen(pos);
    while(*pos == ' ') pos++;
    if(!*pos && *pos != '\n' && *pos != '\r') FAIL("Illegal character at the end of an assignment!\n");
    if(*pos) pos++; // go past the end of the line

    wires_v[ret] = builder.binary(builder.state, isSumElseProd ? GADGET_BIN_XOR : GADGET_BIN_AND, wires_v[v1], wires_v[v2]);
    if(ret == wires_size) wires_size++;
  }


  for(int i = 0; i < d*numOuts; i++){
    wire_t w = findWire(wires_size, wires_n, outs[i]);
    if(w == wires_size) FAIL("Out '%s' not assigned!\n", outs[i])
    builder.out(builder.state, wires_v[w], i/d, i%d);
  }

  mem_free(file);
}
