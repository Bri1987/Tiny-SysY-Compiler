//////const int len = 20;
//
//int main()
//{
//    int i, j, t, n, temp;
//    int mult1[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
//    int mult2[20] = {2, 3, 4, 2, 5, 7 ,9 ,9, 0, 1, 9, 8, 7, 6, 4, 3, 2, 1, 2, 2};
//
//    int len1 = 20;
//    int len2 = 20;
//    int c1[20 + 5];
//    int c2[20 + 5];
//    int result[20 * 2] = {};
//    i = 0;
//    while (i < len1) {
//        c1[i] = mult1[i];
//        i = i + 1;
//    }
//
//    i = 0;
//    while (i < len2) {
//        c2[i] = mult2[i];
//        i = i + 1;
//    }
//
//    n = len1 + len2 - 1;
//
//    i = 0;
//    while (i <= n) {
//        result[i]=0;
//        i = i + 1;
//    }
//
//    temp=0;
//
//    i = len2 - 1;
//    while (i > -1) {
//        t = c2[i];
//        j = len1 - 1;
//        while (j > -1) {
//            temp = result[n] + t * c1[j];
//            if(temp >= 10) {
//                result[n] = (temp);
//                result[n-1] = result[n-1] + temp / 10;
//            }
//            else
//                result[n] = temp;
//            j = j - 1;
//            n = n - 1;
//        }
//        n = n + len1 - 1;
//        i = i - 1;
//    }
//
//    if(result[0] != 0)
//        putint(result[0]);
//
//    i = 1;
//    while (i <= len1 + len2 - 1) {
//        putint(result[i]);
//        i = i + 1;
//    }
//
//    return 0;
//}
//
//int g = 1;
//int test(){
//    int a = getint();
//    int b = getint();
//    int c = 1;
//    int arr[10];
//
//    while(c < 10){
//        int temp2 = a + b;
//        int temp = temp2 + b;
//        arr[1] = temp;
//        g = temp;
//    }
//    putint(a);
//    return 0;
//}
//int sub(int a,int b){
//    int c=a-b;
//    return c;
//}
//int main(){
//    int a;
//    int b;
//    int c;
//    c=sub(a,b);
//    return 0;
//}

int test(){
    int a = 1;
    int c = 2;
    a = getint() + 1;
    if(a > 1){
        c = c + 1;
        a = a + 1;
    }else{
        a = a - 1;
    }
    return c;
}

//int main(){
//    int a = getint();
//    int b = getint();
//    int c = 1;
//    while(c < 1){
//        int temp = a;
//        a = b;
//        b = temp;
//        c = c + 1;
//    }
//    return 1;
//}


//phi的计算
//int main(){
//    int a = 5;
//    int b = 10;
//    int c = 15;
//    while(c > 5){
//        int temp = a;
//        a = b;
//        b = temp;
//        c = c - 1;
//    }
//    return a;
//}