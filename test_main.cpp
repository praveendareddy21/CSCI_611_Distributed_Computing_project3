#include<iostream>
#include "goldchase.h"
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <functional>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include "Map.h"
#include <cstring>

using namespace std;

#define  SHM_SM_NAME "/PD_semaphore"
#define  SHM_NAME "/PD_SharedMemory"
#define REAL_GOLD_MESSAGE "You found Real Gold!!"
#define FAKE_GOLD_MESSAGE "You found Fool's Gold!!"
#define EMPTY_MESSAGE ""
#define YOU_WON_MESSAGE "You Won!"

struct mapboard{
  int rows;
  int cols;
  unsigned char playing;
  unsigned char map[0];
};


mapboard * initSharedMemory(int rows, int columns){
  int fd, size;
  mapboard * mbp;
  fd = shm_open(SHM_NAME,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
  size = (rows*columns + sizeof(mapboard));
  ftruncate(fd, size);
  mbp = (mapboard*) mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  return mbp;
}

mapboard * readSharedMemory(){
  int fd, size, rows, columns;
  mapboard * mbp;
  fd = shm_open(SHM_NAME,O_RDWR, S_IRUSR|S_IWUSR);
  read(fd,&rows,sizeof(int));
  read(fd,&columns,sizeof(int));
  size = (rows*columns + sizeof(mapboard));
  ftruncate(fd, size);
  mbp = (mapboard*) mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  return mbp;
}

vector<vector< char > > readMapFromFile(char * mapFile, int &golds){
  vector<vector< char > > mapVector;
  vector< char > temp;
  string line;
  char c;
  ifstream mapStream(mapFile);
  mapStream >>golds;
  mapStream.get(c);

  while(getline(mapStream,line))
  {
     for(int i=0; i < line.length(); i++){
       temp.push_back(line[i]);
     }
     mapVector.push_back(temp);
     temp.clear();
  }
  //cout<<"ve size "<<mapVector.size()<<" col "<<mapVector[0].size()<<endl;;
  return mapVector;
}

void initGameMap(mapboard * mbp, vector<vector< char > > mapVector ){
  unsigned char * mp;
  mp = mbp->map;
  for(unsigned i=0;i<mapVector.size();i++){
    for(unsigned j=0;j<mapVector[i].size();j++){
      if(mapVector[i][j]==' ')
        *mp=0;
      else if(mapVector[i][j]== '*')
        *mp=G_WALL;
      mp++;
    }
  }
  return;
}

int placeElementOnMap(mapboard * mbp, int elem){
   srand(time(NULL));
   int pos, total_pos =  mbp->rows * mbp->cols;
   while(1){
     pos = rand() % total_pos;
     //cout<<"rand "<<pos<<endl;

     if(mbp->map[pos] == G_WALL)
      continue;
     if(mbp->map[pos] == G_GOLD)
       continue;
     if(mbp->map[pos] == G_FOOL)
        continue;
     if(mbp->map[pos] == G_PLR0)
        continue;
     if(mbp->map[pos] == G_PLR1)
        continue;
     if(mbp->map[pos] == G_PLR2)
        continue;
     if(mbp->map[pos] == G_PLR3)
       continue;
     if(mbp->map[pos] == G_PLR4)
       continue;


    mbp->map[pos] = elem;
    break;
   }
   return pos;
}

void placeGoldsOnMap(mapboard * mbp, int goldCount){
  if(goldCount > 0)
    placeElementOnMap(mbp, G_GOLD);
  for(int i= 0; i< (goldCount-1) ; i++)
    placeElementOnMap(mbp, G_FOOL);
  return;
}

int placeIncrementPlayerOnMap(mapboard * mbp,int & thisPlayerLoc){
  int thisPlayer = -1;
    if(!(mbp->playing & G_ANYP) ) // no one is playing
    {
      mbp->playing |= G_PLR0;
      thisPlayer = G_PLR0;
    }
    else if(!(mbp->playing & G_PLR0) ){
      mbp->playing |= G_PLR0;
      thisPlayer = G_PLR0;
    }
    else if(!(mbp->playing & G_PLR1) ){
      mbp->playing |= G_PLR1;
      thisPlayer = G_PLR1;
    }
    else if(!(mbp->playing & G_PLR2) ){
      mbp->playing |= G_PLR2;
      thisPlayer = G_PLR2;
    }
    else if(!(mbp->playing & G_PLR3) ){
      mbp->playing |= G_PLR3;
      thisPlayer = G_PLR3;
    }
    else if(!(mbp->playing & G_PLR4) ){
      mbp->playing |= G_PLR4;
      thisPlayer = G_PLR4;
    }
  thisPlayerLoc = placeElementOnMap(mbp, thisPlayer);
  return thisPlayer;
}

bool isCurrentMoveOffMap(mapboard * mbp, int currentPos , int nextPos){
  unsigned char * mp;
  mp = mbp->map;
  int rows = mbp->rows, cols = mbp->cols;
  if(currentPos < cols && nextPos < 0)
    return true;
  if( currentPos / cols == rows - 1 && nextPos >= rows * cols)
    return true;
  if( currentPos % cols == 0 && nextPos == currentPos -1 )
    return true;
  if( currentPos % cols == cols - 1 && nextPos == currentPos + 1)
    return true;

    return false;
}

bool isCurrentMoveValid(mapboard * mbp, int currentPos , int nextPos){
  unsigned char * mp;
  mp = mbp->map;
  int rows = mbp->rows, cols = mbp->cols;
  if(currentPos < cols && nextPos < 0)
    return false;
  if( currentPos / cols == rows - 1 && nextPos >= rows * cols)
    return false;
  if( currentPos % cols == 0 && nextPos == currentPos -1 )
    return false;
  if( currentPos % cols == cols - 1 && nextPos == currentPos + 1)
    return false;

  if(mp[nextPos] == G_WALL)
    return false;
  else
    return true;
}

const char * performGoldCheck(mapboard * mbp, int currentPos, bool & thisPlayerFoundGold){
  const char * realGoldMessage = REAL_GOLD_MESSAGE;
  const char * fakeGoldMessage = FAKE_GOLD_MESSAGE;
  const char * emptyMessage = EMPTY_MESSAGE;

  unsigned char * mp;
  mp = mbp->map;
  if(mp[currentPos] & G_GOLD)
    {
      thisPlayerFoundGold = true;
      mp[currentPos] &= ~G_GOLD;
      return realGoldMessage;
    }
  else if(mp[currentPos] & G_FOOL)
    return fakeGoldMessage;
  else
    return emptyMessage;

}

const char * processPlayerMove(mapboard * mbp, int & thisPlayerLoc, int thisPlayer, int keyInput, bool & thisPlayerFoundGold, bool & thisQuitGameloop){
  unsigned char * mp;
  const char * emptyMessage = EMPTY_MESSAGE;
  const char * youWonMessage = YOU_WON_MESSAGE;
  bool quitGameLoop = false;
  mp = mbp->map;
  int nextPos = 0, cols = mbp->cols, goldCheck = -1;
  switch (keyInput) {
    case 108: // key l move right
      nextPos = thisPlayerLoc + 1;
      break;

    case 104: // key h move left
      nextPos = thisPlayerLoc - 1;
      break;

    case 107: // key k move up
      nextPos = thisPlayerLoc - cols;
      break;

    case 106: // key j move down
      nextPos = thisPlayerLoc + cols ;
      break;

  }

  if((thisPlayerFoundGold) && isCurrentMoveOffMap(mbp, thisPlayerLoc, nextPos) ){
    thisQuitGameloop = true;
    return youWonMessage;
  }

  if(isCurrentMoveValid(mbp, thisPlayerLoc, nextPos) ){
    mp[thisPlayerLoc] &= ~thisPlayer;
    thisPlayerLoc = nextPos;
    mp[thisPlayerLoc] |= thisPlayer;

    return performGoldCheck(mbp, thisPlayerLoc, thisPlayerFoundGold);
  }

  return emptyMessage;
}

int main(int argc, char *argv[])
{
  mapboard * mbp = NULL;
  Map * gameMap = NULL;
  int rows, cols, goldCount, thisPlayer = 0, thisPlayerLoc= 0, keyInput = 0, currPlaying = -1;
  bool thisPlayerFoundGold = false , thisQuitGameloop = false;
  char * mapFile = "mymap.txt";
  const char * notice;
  unsigned char * mp; //map pointer
  vector<vector< char > > mapVector;
  sem_t* shm_sem;

  shm_sem = sem_open(SHM_SM_NAME ,O_RDWR,S_IRUSR|S_IWUSR,1);
  if(shm_sem == SEM_FAILED)
  {
     shm_sem=sem_open(SHM_SM_NAME,O_CREAT,S_IRUSR|S_IWUSR,1);
     //cout<<"first player"<<endl;
     mapVector = readMapFromFile(mapFile, goldCount);
     rows = mapVector.size();
     cols = mapVector[0].size();
     //cout<<"rows "<<rows<<"cols "<<cols<<endl;

     sem_wait(shm_sem);
     mbp = initSharedMemory(rows, cols);
     mbp->rows = rows;
     mbp->cols = cols;
     mbp->playing = 0;

     initGameMap(mbp, mapVector);
     placeGoldsOnMap(mbp, goldCount);
     thisPlayer = placeIncrementPlayerOnMap(mbp, thisPlayerLoc);
     sem_post(shm_sem);
     //cout<<"shm init done"<<endl;
   }
   else
   {
     //cout<<"not first player"<<endl;
     sem_wait(shm_sem);
     mbp = readSharedMemory();
     rows = mbp->rows;
     cols = mbp->cols;
     thisPlayer = placeIncrementPlayerOnMap(mbp, thisPlayerLoc);
     sem_post(shm_sem);
   }


   try
   {
     sem_wait(shm_sem);
     gameMap = new Map(reinterpret_cast<const unsigned char*>(mbp->map),rows,cols);
     sem_post(shm_sem);

     while(keyInput != 81){ // game loop  key Q
       keyInput =  (*gameMap).getKey();
       // code for player moves
       if(keyInput ==  108 || keyInput ==  107 || keyInput ==  106 || keyInput ==  104 ) // for l, k, j, h
       { sem_wait(shm_sem);
         notice = processPlayerMove(mbp, thisPlayerLoc,  thisPlayer, keyInput, thisPlayerFoundGold, thisQuitGameloop);

         if(notice == FAKE_GOLD_MESSAGE || notice == REAL_GOLD_MESSAGE || notice == YOU_WON_MESSAGE){
           (*gameMap).postNotice(notice);
         }
         (*gameMap).drawMap();
         sem_post(shm_sem);

         if(thisQuitGameloop)
          break;

       }

     }
   }
   catch (const runtime_error& error)
   {
     cout<<"runtime_error!!  Window size not large enough"<<endl;
     cout<<"Exiting gracefully"<<endl;
     sem_post(shm_sem);
   }

   sem_wait(shm_sem);
   mbp->map[thisPlayerLoc] &= ~thisPlayer;
   mbp->playing &= ~thisPlayer;
   currPlaying = mbp->playing;
   sem_post(shm_sem);

   delete gameMap;

   if(currPlaying == 0)
   {
      shm_unlink(SHM_NAME);
      sem_close(shm_sem);
      sem_unlink(SHM_SM_NAME);
   }
   return 0;
}
