// Trivial test program with local variables

int main(void) {
	int sum = 0, prod = 1;
	for(int opnd = 1; opnd <= 10; ++opnd) {
		sum += opnd;
		prod *= opnd;
	}
	return 0;
}
