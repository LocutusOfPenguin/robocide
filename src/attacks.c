#include <stdlib.h>
#include "attacks.h"
#include "bb.h"

bb_t AttacksArrayKnight[64], AttacksArrayKing[64];

void AttacksInit()
{
  // Attack arrays for knight and king
  sq_t SqA, SqB;
  for(SqA=0;SqA<64;++SqA)
  {
    AttacksArrayKnight[SqA]=0;
    AttacksArrayKing[SqA]=0;
    for(SqB=0;SqB<64;++SqB)
    {
      int DX=abs(SQ_X(SqA)-SQ_X(SqB));
      int DY=abs(SQ_Y(SqA)-SQ_Y(SqB));
      if ((DX==1 && DY==2) || (DX==2 && DY==1))
        AttacksArrayKnight[SqA]|=BBSqToBB(SqB);
      if (DX<=1 && DY<=1 && (DX!=0 || DY!=0))
        AttacksArrayKing[SqA]|=BBSqToBB(SqB);
    }
  }
  
  // Magic move generation for sliders
  initmagicmoves();
}
