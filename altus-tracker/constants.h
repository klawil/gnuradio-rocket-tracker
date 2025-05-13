#ifndef CONSTANTS_H
#define CONSTANTS_H

#define BAUD_RATE 38400
#define SAMPLE_RATE (BAUD_RATE * 66)
#define DECIMATED_SAMPLE_RATE (BAUD_RATE * 3)

// Sync word for Altus products
#define SYNC_WORD 0xd391

// Number of possible viterbi states
#define NUM_V_STATE 8

// Number of bits to buffer before starting decoding process
// This is 8 bits / byte * 2 (transmission rate) * 2 (bytes for interleaving)
#define BITS_TO_BUFFER 32

// Number of bits to keep in the viterbi history
#define NUM_V_HIST 24

// Number of bytes in an Altus message (32 bytes of data, 2 bytes checksum, 2 bytes terminator)
#define BYTES_PER_MESSAGE 36

#endif
