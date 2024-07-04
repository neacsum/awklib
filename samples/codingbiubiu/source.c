/*
  Sample C code reading from a simulated stream
*/
#include <awklib/awk.h>

int strout(const char* buf, size_t sz)
{
  printf("%s\n", buf);
  return 1;
}

typedef struct {
  const char* str;
  size_t pos;
} StringStream;


static StringStream* stream; //global variable needed by my_get

// set global variable
void myget_setup(StringStream* s)
{
  stream = s;
}

int my_get() {
  if (stream->str[stream->pos] != '\0') {
    return stream->str[stream->pos++];
  }
  else {
    return EOF;  // End of string
  }
}

int main(int argc, char** argv)
{
  StringStream instr;
  instr.str = "A B\nC D\n";
  instr.pos = 0;

  myget_setup(&instr); // pass info required by my_get through some outside
                       // mechanism (in this case a static global)

  AWKINTERP* interp = awk_init(NULL);
  awk_setprog(interp, "{printf $0\" \"$1}");
  awk_compile(interp);
  awk_infunc(interp, my_get); //no casting needed 
  awk_outfunc(interp, strout);
  awk_exec(interp);
  awk_end(interp);
}

