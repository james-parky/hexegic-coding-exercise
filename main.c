#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEFT 0
#define RIGHT 1

enum ROTATE_RETURN_CODE {
  ROTATE_SUCCESS = 0,
  ROTATE_READ_ERROR,
  ROTATE_WRITE_ERROR
};

/******************************************************************************
 * rotate_file:                                                               *
 *      Rotate a file bitwise, either to the left or to the right.            *
 *                                                                            *
 * @param direction: The direction to rotate, either left (0) or right (1).   *
 * @param in_file: A FILE pointer to the file to rotate.                      *
 * @param out_file: A FILE pointer to the file to write the rotate contents.  *
 *                                                                            *
 * @return The status of the rotation, either 0 (success), 1 (read error), or *
 *         2 (write error).                                                   *
 *****************************************************************************/
enum ROTATE_RETURN_CODE rotate_file(int direction, FILE *in_file,
                                    FILE *out_file) {
  int8_t first_read_byte, second_read_byte, first_write_byte;

  if ((first_read_byte = fgetc(in_file)) == EOF) {
    if (ferror(in_file))
      return ROTATE_READ_ERROR;
    return ROTATE_SUCCESS;
  }

  if (direction == LEFT) {
    // If left, the MSB of the first byte is saved to be appended later.
    int8_t carried_bit = first_read_byte >> 7;

    while ((second_read_byte = fgetc(in_file)) != EOF) {
      first_write_byte = (first_read_byte << 1) | (second_read_byte >> 7);
      if ((fputc(first_write_byte, out_file) == EOF) && ferror(out_file))
        return ROTATE_WRITE_ERROR;
      first_read_byte = second_read_byte;
    }

    if (ferror(in_file))
      return ROTATE_READ_ERROR;

    if (((fputc((first_read_byte << 1) | carried_bit, out_file)) == EOF) &&
        ferror(out_file))
      return ROTATE_WRITE_ERROR;
  } else {
    // If right, a dummy byte is written first which is overwritten later.
    int8_t dummy_byte = 0;
    if ((fputc(dummy_byte, out_file) == EOF) && ferror(out_file))
      return ROTATE_WRITE_ERROR;
    while ((second_read_byte = fgetc(in_file)) != EOF) {
      first_write_byte = (first_read_byte << 7) | (second_read_byte >> 1);
      if ((fputc(first_write_byte, out_file) == EOF) && ferror(out_file))
        return ROTATE_WRITE_ERROR;
      first_read_byte = second_read_byte;
    }

    if (ferror(in_file))
      return ROTATE_READ_ERROR;

    int8_t carried_bit = first_read_byte << 7;
    rewind(out_file);
    rewind(in_file);
    if (((first_read_byte = fgetc(in_file)) == EOF) && ferror(in_file))
      return ROTATE_READ_ERROR;
    if (((fputc(carried_bit | (first_read_byte >> 1), out_file)) == EOF) &&
        ferror(out_file))
      return ROTATE_WRITE_ERROR;
  }

  return ROTATE_SUCCESS;
}

int main(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, "Usage: %s {left right} <in_file> <out_file>\n", argv[0]);
    return EXIT_FAILURE;
  }

  if ((strcmp(argv[1], "left") != 0) && (strcmp(argv[1], "right") != 0)) {
    fprintf(stderr, "Invalid arguments: The first argument must either be "
                    "`left` or `right`.\n");
    return EXIT_FAILURE;
  }

  int direction = argv[1][0] == 'r' ? RIGHT : LEFT;
  FILE *in_file = fopen(argv[2], "r");
  FILE *out_file = fopen(argv[3], "w+");

  if (in_file == NULL) {
    fprintf(stderr, "Error: The <in_file> supplied could not be "
                    "opened correctly.\n");
    return EXIT_FAILURE;
  }

  if (out_file == NULL) {
    fprintf(stderr, "Error: The <out_file> supplied could not be "
                    "opened correctly.\n");
    return EXIT_FAILURE;
  }

  enum ROTATE_RETURN_CODE result = rotate_file(direction, in_file, out_file);

  fclose(in_file);
  fclose(out_file);

  switch (result) {
  case ROTATE_READ_ERROR:
    fprintf(stderr, "Error: An error ocurred whilst reading from %s.\n",
            argv[2]);
    return EXIT_FAILURE;
  case ROTATE_WRITE_ERROR:
    fprintf(stderr, "Error: An error ocurred whilst writing to %s.\n", argv[3]);
    return EXIT_FAILURE;
  default:
    break;
  }

  return EXIT_SUCCESS;
}
