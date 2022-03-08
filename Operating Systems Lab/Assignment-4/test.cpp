#include <bits/stdc++.h>
using namespace std;

int main() {
    FILE *p;
    long size;
    p = fopen("text.txt", "rb");
    if(p == NULL) {
        printf("Error opening file");
        return 1;
    }else{
        fseek(p, 0, SEEK_END);
        size = ftell(p);
        fclose(p);
        cout << size << endl;
    }
}