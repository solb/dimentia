// http://codeforces.com/contest/665/submission/17402226

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define MAX 200010
#define clr(ar) memset(ar, 0, sizeof(ar))
#define read() freopen("lol.txt", "r", stdin)

int n, dp[MAX][27];
bool visited[MAX][27];
char str[MAX], out[MAX];

int F(int i, int l){
    if (i == n) return 0;
    if (visited[i][l]) return dp[i][l];

    int j, k, r, x = str[i] - 97, res = 1 << 25;
    if (l != x){
        r = F(i + 1, x);
        if (r < res) res = r;
    }

    for (j = 0; j < 26; j++){
        if (j != l){
            r = 1 + F(i + 1, j);
            if (r < res) res = r;
        }
    }

    visited[i][l] = true;
    return (dp[i][l] = res);
}

void backtrack(int i, int l){
    if (i == n) return;

    int j, k, r, x = str[i] - 97, res = 1 << 25;
    if (l != x){
        r = F(i + 1, x);
        if (r < res) res = r, k = -1;
    }

    for (j = 0; j < 26; j++){
        if (j != l){
            r = 1 + F(i + 1, j);
            if (r < res) res = r, k = j;
        }
    }

    if (k == -1){
        out[i] = str[i];
        backtrack(i + 1, x);
    }
    else{
        out[i] = k + 97;
        backtrack(i + 1, k);
    }
}

int main(){
    int i, j, k, l, res;

    while (scanf("%s", str) != EOF){
        clr(visited);
        n = strlen(str);
        res = F(0, 26);
        backtrack(0, 26);
        out[n] = 0;
        puts(out);
    }
    return 0;
}

