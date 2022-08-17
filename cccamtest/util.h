//
//  util.h
//  cccamtest
//

#ifndef util_h
#define util_h

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>

int getUserInput (FILE* inputPointer, char * returnStr, int maxStringLength, int timeout_sec) {
   fd_set rfds;
   FD_ZERO(&rfds);
   FD_SET(0, &rfds);
   struct timeval timeout = {timeout_sec, 0};

   char    *tempStr;
   int     maxLen, totalCount = 0;
   size_t  len;
   maxLen = maxStringLength + 2;     //account for NULL and /newline
   tempStr = (char*)malloc(maxLen * sizeof(char));  //temporary holder
   do {
      if(timeout_sec >= 0) {
          int poll = select(1, &rfds, NULL, NULL, &timeout);
          if(poll <= 0)
             break;
       }
      if(fgets(tempStr, maxLen, inputPointer) == NULL){  // get chars from input buffer
         totalCount = 0;
         break;
      }
      len = strlen(tempStr);
      if (tempStr[len-1] == '\n') { // newline indicates end of string
         tempStr[len-1] = '\0';   // delete it
         len = strlen(tempStr);   // and recalc length
      }
      totalCount += (int)len;
   }
   while ((int)len > maxStringLength);  // continue to flush extras if too long
   strcpy(returnStr, tempStr);  // copy temp string into output
   free(tempStr);              // and release memory
   return totalCount;   // may be more than the number allowed
}


#endif /* util_h */
