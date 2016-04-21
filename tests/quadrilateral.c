#define _DEFAULT_SOURCE

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MODE  "ap"
#define SHAPE "rs"
#define PARAM "l:w:"

enum mode {
  MODE_UNSET,
  MODE_AREA,
  MODE_PERIMETER,
};
enum shape {
  SHAPE_UNSET,
  SHAPE_RECTANGLE,
  SHAPE_SQUARE,
};

#define SWITCHSTRING MODE SHAPE PARAM
static const char *const SWITCHES_MODE =  MODE;
static const char *const SWITCHES_SHAPE = SHAPE;
static const char *const SWITCHES_PARAM = PARAM;
static const bool EXPECTED_PARAMS[sizeof SWITCHES_SHAPE - 1][sizeof SWITCHES_PARAM / 2] = {
                      {'l',   'w'},
  [SHAPE_RECTANGLE] = {true,  true},
  [SHAPE_SQUARE] =    {true,  false},
};

int main(int argc, char **argv) {
  enum mode mode = MODE_UNSET;
  enum shape shape = SHAPE_UNSET;
  int param[sizeof SWITCHES_PARAM / 2];
  memset(param, -1, sizeof param);

  int flag;
  while((flag = getopt(argc, argv, SWITCHSTRING)) != -1) {
    const char *is_mode = NULL, *is_shape = NULL, *is_param = NULL;
    if((is_mode = strchr(SWITCHES_MODE, flag))) {
      if(mode) {
        fprintf(stderr, "Choose ONE of the switches: -%s\n", SWITCHES_MODE);
        return 1;
      }
      mode = 1 + is_mode - SWITCHES_MODE;
    } else if((is_shape = strchr(SWITCHES_SHAPE, flag))) {
      if(shape) {
        fprintf(stderr, "Choose ONE of the switches: -%s\n", SWITCHES_SHAPE);
        return 2;
      }
      shape = 1 + is_shape - SWITCHES_SHAPE;
    } else if((is_param = strchr(SWITCHES_PARAM, flag))) {
      int number = atoi(optarg);
      if(number < 0) {
        fputs("Measurements must be nonnegative", stderr);
        return 3;
      }
      param[(is_param - SWITCHES_PARAM) / 2] = number;
    } else
      return 4;
  }

  if(!mode) {
    fprintf(stderr, "Choose one of the switches: -%s\n", SWITCHES_MODE);
    return 5;
  }
  if(!shape) {
    fprintf(stderr, "Choose one of the switches: -%s\n", SWITCHES_SHAPE);
    return 6;
  }
  bool missing = false;
  for(size_t index = 0; index < sizeof param / sizeof *param; ++index) {
    bool provided = param[index] != -1;
    if(provided != EXPECTED_PARAMS[shape][index]) {
      char flag = SWITCHES_PARAM[index * 2];
      if(provided)
        fprintf(stderr, "Provided switch -%c incompatible with chosen shape\n", flag);
      else
        fprintf(stderr, "Missing switch -%c required by chosen shape\n", flag);
      missing = true;
    }
  }
  if(missing)
    return 7;

  if(mode == MODE_AREA) {
    int area;
    if(shape == SHAPE_RECTANGLE)
      area = param[0] * param[1];
    else // shape == SHAPE_SQUARE
      area = param[0] * param[0];
    printf("AREA: %d\n", area);
  } else { // mode == MODE_PERIMETER
    int perimeter;
    if(shape == SHAPE_RECTANGLE)
      perimeter = 2 * param[0] + 2 * param[1];
    else // shape == SHAPE_SQUARE
      perimeter = 4 * param[0];
    printf("PERIMETER: %d\n", perimeter);
  }
  return 0;
}
