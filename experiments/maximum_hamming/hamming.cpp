#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <cmath>
#include <iostream>
#include "rs.hpp"
#include <iomanip>



int hammingDistance (uint64_t x, uint64_t y) {
        uint64_t res = x ^ y;
        return __builtin_popcountll (res);
}

std::string bin2str(uint64_t n) {
        std::string rtn = "";
        while (n) {
                if (n & 1)
                        rtn = "1" + rtn;
                else
                        rtn = "0" + rtn;
                n >>= 1;
        }
        return rtn;
}

int bits_n(int x) {
        std::cout << "log " << std::log(x) / std::log(2) << std::endl;
    return (x != 0) ? std::ceil( std::log(x) / std::log(2)) : 1;
}

int generate(int bits, int count, std::vector<uint64_t> &output) {

        uint64_t start_value = 0;
        for (int i = 0; i < bits; i ++) {
                start_value |= 1 << i;
        }
        //values.push_back(start_value);
        
        float bits_to_encode = bits_n(count+1);
        printf("%f bits to encode %d\n", bits_to_encode, count);
        for (uint64_t idx = 1; idx <= count+1; idx++) {
                uint64_t new_val = idx;
                for (int shift = bits_to_encode; shift <= bits; shift+=bits_to_encode) {
                        new_val |= idx << shift;
                }
                output.push_back(new_val);
        } 


        

        return 1;
}

/**
 * https://github.com/mersinvald/Reed-Solomon
 */
int generate2(int bytes, int count, std::vector<uint64_t> &output) {

        const int msglen = 2; // bytes
        const int ecc_len = 8;
        char encoded[msglen + bytes-1];

        RS::ReedSolomon<msglen, 8> rs; // 2 bytes
        
        char message[] = {0,0};
        for (int i = 1; i <= count; i++) {
                message[0] = i;
                message[1] = (i >> 8);
                std::cout << i << std::endl;
                rs.Encode(message, encoded);

                uint64_t tmp_out = 0;
                for (uint x = msglen,shift = 0; x < msglen + ecc_len; x++, shift += 8) {
                        tmp_out |= ((uint64_t)0xFF & encoded[x]) << shift;
                        std::cout << std::hex << tmp_out << std::endl;
                }
                for (uint j = 0; j < msglen + ecc_len; j++) {
                        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)(0xFF &encoded[j]) << "";
                } 
                std::cout << std::endl;

                output.push_back(tmp_out);
        }
        return 0;

}

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
  for (int i = 1; i <= count; i++) {
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

void printValues(std::vector<unsigned> values) {
        unsigned min_ham = sizeof(unsigned)*8;
        for(auto const& value: values) {
                printf("%u\t %X\n", value, value);
                printf(bin2str(value).c_str());
                printf("\nHamming:");
                for(auto const& value2: values) {
                        if (value == value2) {
                                continue;
                        }
                        unsigned ham = hammingDistance(value, value2);
                        if (ham < min_ham)
                                min_ham = ham;
                        printf("%d ", ham);
                        
                }
                printf("\n\n");

        }
        std::cout << "Minimum Hamming: " << min_ham << std::endl;
}

int main() {
        uint64_t a = 1;
        uint64_t b = 0b101011;
        printf("%d", hammingDistance(a,b));

        std::vector<uint64_t> values1;
        generate(32, 10, values1);
        //printValues(values1);
        //generate(16, 12, values1);
        //generate(32, 2, values1);

        std::vector<unsigned> values2;
        // generate2(8, 10, values2);
        generateNumbersWithMaximumHamming(1000, values2);
        printValues(values2);
        // generate(16, 12, values1);
        // generate(32, 2, values1); 

    
    return 0;
}