#include <lapacke.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>

#include <algorithm>

using namespace std;

int main (){
  int INFO;
  //  B = (double*) malloc(NDIM*sizeof(double));

  // const int rows = 4;
  // const int cols = 3;
  // double A[rows*cols] = { 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }; // column-major order

  const int rows = 3;
  const int cols = 3;
  double A[rows*cols] = {1, 1, 0, 1, 1, 0, 0, 0, 1}; // z bad

  for (int i = 0;i < rows; i++) {
    for (int j = 0;j < cols; j++) {
      printf("   %lf  ", A[i+rows*j]);
    }
      printf("\n");
  }

  char cN = 'N';
  char cA = 'A';
  static double sigmas[rows+cols];
  static double work[(rows+cols)*30]; int work_sz = (rows+cols)*30;

  static double Vt[rows*cols];
  int ldvt = cols;

  int tmp_rows = rows, tmp_cols = cols;
  dgesvd_(&cN, &cA, &tmp_rows, &tmp_cols, A, &tmp_rows,
          sigmas, NULL, &tmp_rows, Vt, &ldvt, work, &work_sz, &INFO);
  printf("info %d \n", INFO); fflush(stdout);
 
  for (int i = min(cols, rows); i < cols; ++i)
    sigmas[i] = 0;
  for (int i = 0; i < cols; ++i)
    printf("%lf ", sigmas[i]);
  printf("\n");

  for (int i = 0; i < cols; ++i) {
    for (int j = 0; j < cols; ++j) {
      printf(" %10lf ", Vt[i+j*cols]);
    }
    printf("\n");
  }

  const double eps = 1e-9;
  set<int> good;
  for (int i = 0; i < cols; ++i) {
    if (fabs(sigmas[i]) < eps) {
      for (int j = 0; j < cols; ++j)
        if (fabs(Vt[i+j*cols]) > 0) {
          good.insert(j);
        }
    }
  }

  printf("good = ");
  for (int g : good) {
    printf("%d ", g);
  }
  printf("\n");

  // null vectors Vt[i][:] if sigmas[i] == 0
  // null vector has non-zero -> meaning it doesn't have to be zero -> not bad
  
  return 0;
}
