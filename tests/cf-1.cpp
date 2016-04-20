#include <algorithm>

#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <vector>
#include <iostream>

using namespace std;

#define FOR(i, a, b) for (int i = (a); i < (b); ++i)
#define REP(i, n) FOR(i, 0, n)
#define TRACE(x) cout << #x << " = " << x << endl
#define _ << " _ " <<

typedef long long llint;

const int MAXN = 2123;

int mat[MAXN][MAXN];

int main(void) {
  int ntc; scanf("%d", &ntc);
  REP(tc, ntc) {
    int n; scanf("%d", &n);

    REP(i, n) REP(j, n) {
      scanf("%d", &mat[i][j]);
      --mat[i][j];
    }

    vector<int> isin(n, 0);
    isin[0] = true;

    FOR(fit, 1, n) {
      int x = mat[0][fit];

      int nx = -1;
      REP(i, n) if (isin[mat[x][i]]) {
        nx = mat[x][i];
        break;
      }
      assert(nx != -1);

      isin[x] = true;
      printf("%d %d\n", nx+1, x+1);
    }
    printf("\n");
  }
  return 0;
}
