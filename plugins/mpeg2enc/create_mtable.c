#include <stdio.h>

int main(char**, int)
{
  FILE *fout;
  int i, j, v;

  fout = fopen("mtable.h", "wt");
  fprintf(fout, "const unsigned char motion_lookup[256][256] = {\n");
  for (i = 0; i < 256; i++)
  {
    fprintf(fout, "{");
    for (j = 0; j < 256; j++)
    {
      v = i - j;
      if (v < 0)
        v = -v;
      fprintf(fout, "%3d", v);
      if (j != 255)
        fprintf(fout, ", ");
      if ((j) && (j % 16 == 0))
        fprintf(fout, "\n");
    }
    if (i != 255)
      fprintf(fout, "},\n");
  }
  fprintf(fout, "}};\n");
  fclose(fout);

  return 0;
}
