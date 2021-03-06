//
// Created by machiry on 7/20/19.
//

#include <llvm/Support/FileSystem.h>

#include "Utils.h"
#include "rs.hpp"
#include "llvm/Support/Path.h"

using namespace llvm;

bool getAbsoluteFilePath(std::string fileName, std::string &absoluteFP) {
  // get absolute path of the provided file
  // returns true if successful else false
  SmallString<255> abs_path(fileName);
  std::error_code ec = llvm::sys::fs::make_absolute(abs_path);
  if(!ec) {
    absoluteFP = abs_path.str();
    return true;
  }
  return false;
}

// static unsigned getHammingFlag(unsigned currFlag, unsigned numBits) {
//   unsigned toRet = 0;
//   unsigned currIdx = 0;
//   unsigned maxBits = sizeof(unsigned) * 8;
//   unsigned remainingBits = sizeof(unsigned) * 8;
//   assert(numBits < remainingBits && numBits);
//   while(numBits && (currIdx < maxBits)) {
//     unsigned currBit = (1 << currIdx);
//     bool toSet = ((((unsigned)rand()) % 2) == 1) && !(currBit & currFlag);
//     if(numBits >= remainingBits || toSet) {
//       toRet |= currBit;
//       numBits--;
//     }
//     remainingBits--;
//     currIdx++;
//   }
//   return toRet;
// }

// bool generateNumbersWithMaximumHamming(unsigned num, std::vector<unsigned> &output) {

//   unsigned hammingFlag = 0;
//   unsigned hammingBits = (sizeof(unsigned) * 8) / num;
//   assert(num > 0);
//   // generate first random numbers.
//   unsigned initR = rand();
//   unsigned prevNumber = initR;
//   output.push_back(initR);
//   while(num) {
//     unsigned currFlag = getHammingFlag(hammingFlag, hammingBits);
//     prevNumber ^= currFlag;
//     output.push_back(prevNumber);
//     hammingFlag |= currFlag;
//     num--;
//   }
//   return true;
// }

/**
 * https://github.com/mersinvald/Reed-Solomon
 */
bool generateNumbersWithMaximumHamming(unsigned count, std::vector<unsigned> &output) {

  // We currently only encode 2 bytes of data for the count [ MAX(ENUM) ]
  assert(count < 1<<16);

  const int msglen = 2; // bytes
  const int ecc_len = sizeof(unsigned); // bytes
  char encoded[msglen + ecc_len];

  RS::ReedSolomon<msglen, ecc_len> rs;
  
  char message[] = {0,0};
  for (unsigned i = 1; i <= count; i++) {
    message[0] = i;
    message[1] = (i >> 8);

    rs.Encode(message, encoded);

    uint64_t tmp_out = 0;
    for (uint x = msglen,shift = 0; x < msglen + ecc_len; x++, shift += 8) {
            tmp_out |= ((uint64_t)0xFF & encoded[x]) << shift;
    }
    output.push_back(tmp_out);
  }
  return true;

}