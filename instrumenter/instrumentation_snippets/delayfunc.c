#include <stdlib.h> 
void gpdelay(void) {
   // make sure that you initialize
   // srand before this.
   unsigned randCount = rand();
   unsigned i = 0;
   while(i < randCount) {
       // some instructions to add delay.
       rand();
       i++;
   }
   return;
}
