struct S {
  int x;
};

int main(void) {
  struct S a, b;
  b.x += a.x*a.x;
  return 0;
}
