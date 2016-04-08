extern "C" {
#include <lapacke.h>
}

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>

using namespace std;

int main (){
  int INFO;
  //  B = (double*) malloc(NDIM*sizeof(double));

  const int rows = 4;
  const int cols = 3;
  double A[rows*cols] = {  1, 1, 0 ,
                           0, 1, 0 ,
                           0, 0, 0 ,
                           0, 0, 0  };
                  
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
          sigmas, NULL, NULL, Vt, &ldvt, work, &work_sz, &INFO);
  printf("info %d \n", INFO);
 
  for (int i = 0; i < min(cols, rows); ++i)
    printf("%lf ", sigmas[i]);
  printf("\n");

  return 0;
}
