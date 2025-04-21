int var1 = 0;
int global_var;


int foo(int a, int b) {
  static int var2 = 0;
  int var3 = 123;
  ++var2;
  return a + b + var1 + var2 + var3;
}

int func(int param) {
    int local = param + global_var;
    return local;
}

static int static_func() {
    static int count = 0;
    return count;
}
