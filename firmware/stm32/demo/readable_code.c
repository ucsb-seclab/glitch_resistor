unsigned int gr_tick = -1;
enum valueRtn {
  GR_SUCCESS = 3889321827,
  GR_FAILURE = 3552161478,
  GR_UNKNOWN = 879491493
};
int checkValue(int value) {
  if (value == 0) {
    return GR_SUCCESS;
  } else {
    return GR_FAILURE;
  }
}
int checkTick() {
  if (gr_tick == 0) {
    return 0;
  } else {
    return -1;
  }
}
int main() {
  while (gr_tick != 0) {

    gr_tick = HAL_GetTick();

    if (checkValue(gr_tick) == GR_SUCCESS || checkTick() == 0) {
      while (1) {
        printf("You Win!");
      }
    } else {
      printf("No");
    }
  }
  while (1) {
    printf("You Win!");
  }
}
