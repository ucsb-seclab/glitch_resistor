#include <stdlib.h>
// the flags containing the integrity values to be checked.
#define LONGINTEGRITYVALUE (~((long)0))
#define CHARINTEGRITYVALUE (char)((~((char)0)))

#define TRUE 0xAAAAAAAA
#define FALSE 0xBBBBBBBB
// This function checks for the integrity of the value being read from
// the provided src pointer.
// it expects the integrity value to be stored in the memory pointed by src_int
// If the integrity is valid then it reads size number of bytes
// into the memory pointed by the dst pointer.
// also it returns 1 if the integrity is valid else returns 0.
int gp_safe_read(void *src, void *src_int, void *dst, unsigned size) {
  // read safely from the provided src with verifying integrity.
  // try reading longs.
  int integrityIsFine = TRUE;
  if (size > 0) {
    // first verify the sanity of
    // long number of bytes.
    // We do it in 2 passes.
    // first we read in the multiple of long bytes.
    // next if the remaining bytes are less than long bytes
    // then we ready one byte at a time.
    while (size > sizeof(long)) {
      long rdVal = *((long *)src);
      long sanVal = *((long *)src_int);
      if ((rdVal ^ sanVal) == LONGINTEGRITYVALUE) {
        *((long *)dst) = rdVal;
      } else {
        integrityIsFine = FALSE;
        break;
      }
      src += sizeof(long);
      src_int += sizeof(long);
      dst += sizeof(long);
      size -= sizeof(long);
    }

    // only if the integrity is fine.
    if (integrityIsFine) {
      while (size) {
        char rdVal = *((char *)src);
        char sanVal = *((char *)src_int);
        if ((rdVal ^ sanVal) == CHARINTEGRITYVALUE) {
          *((char *)dst) = rdVal;
        } else {
          integrityIsFine = 0;
          break;
        }
        src++;
        src_int++;
        dst++;
        size--;
      }
    }
  }

  return integrityIsFine;
}

// helper function that reads an integer from the
// provided address
int gp_read_int(void *src, void *src_int) {
  int to_read = -1;
  if (gp_safe_read(src, src_int, (void *)(&to_read), sizeof(int)) != TRUE) {
    // integrity failed.
    // as of
    gr_glitch_detected();
  }
  return to_read;
}

// helper function that reads a charecter from the provided buffer
char gp_read_char(void *src, void *src_int) {
  char to_read = -1;
  if (gp_safe_read(src, src_int, (void *)(&to_read), sizeof(char)) != TRUE) {
    // integrity failed.
    gr_glitch_detected();
  }
  return to_read;
}

// helper function that reads a pointer value from the provided buffer
void *gp_read_ptr(void *src, void *src_int) {
  void *to_read = NULL;
  if (gp_safe_read(src, src_int, (void *)(&to_read), sizeof(void *)) != TRUE) {
    // integrity failed.
    gr_glitch_detected();
  }
  return to_read;
}

// This function writes size number of bytes into the memory pointed by dst
// from the memory pointed by the src pointer.
// It also writes the integrity value into the memory pointed by dst_int
// pointer.
void gp_safe_write(void *dst, void *dst_int, void *src, unsigned size) {
  if (size > 0) {
    // We do it in 2 passes.
    // first we write in the multiple of long bytes.
    // next if the remaining bytes are less than long bytes
    // then we write one byte at a time.
    while (size > sizeof(long)) {
      long wrVal = *((long *)src);
      long wrIntVal = wrVal ^ LONGINTEGRITYVALUE;
      *((long *)dst) = wrVal;
      *((long *)dst_int) = wrIntVal;

      dst += sizeof(long);
      dst_int += sizeof(long);
      src += sizeof(long);
      size -= sizeof(long);
    }
    while (size) {
      char wrVal = *((char *)src);
      char wrIntVal = wrVal ^ CHARINTEGRITYVALUE;
      *((char *)dst) = wrVal;
      *((char *)dst_int) = wrIntVal;

      dst++;
      dst_int++;
      src++;
      size--;
    }
  }
}

// helper function that writes an integer from the
// provided address
void gp_write_int(void *dst, void *dst_int, int toWrite) {
  gp_safe_write(dst, dst_int, (void *)(&toWrite), sizeof(int));
}

// helper function that writes a character from the provided buffer
void gp_write_char(void *dst, void *dst_int, char toWrite) {
  gp_safe_write(dst, dst_int, (void *)(&toWrite), sizeof(char));
}

// helper function that writes a pointer value into the provided buffer
void gp_write_ptr(void *dst, void *dst_int, void *toWrite) {
  gp_safe_write(dst, dst_int, (void *)(&toWrite), sizeof(void *));
}
