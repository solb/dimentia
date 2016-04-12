// Trivial test program with global variables

static int sum = 0, prod = 1;

int main(void) {
  for(int opnd = 1; opnd <= 10; ++opnd) {
    sum += opnd;
    prod *= opnd;
  }

  return 0;
}
