/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 
 * USA
 */
 








#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

// Convert input files to .h files consisting of hex arrays

int main(int argc, char *argv[])
{
	if(argc < 2) return 1;

	for(argc--; argc > 0; argc--)
	{
		FILE *in;
		FILE *out;
		char variable[1024], header_fn[1024], output_fn[1024], *suffix, *prefix;
		int i;
		int bytes_per_row = 16;
		char row[1024], byte[1024], character;
		struct stat st;
		long total_bytes;

		in = fopen(argv[argc], "rb");
		if(!in) continue;

		stat(argv[argc], &st);
		total_bytes = (long)st.st_size;

// Replace . with _png and append .h to filename
		strcpy(output_fn, argv[argc]);
		suffix = strrchr(output_fn, '.');
		if(suffix) *suffix = '_';
		strcat(output_fn, ".h");

		out = fopen(output_fn, "w");
		if(!out)
		{
			fclose(in);
			continue;
		}


// Strip leading directories for variable and header
		prefix = strrchr(output_fn, '/');
		if(!prefix) 
			prefix = output_fn;
		else
			prefix++;

		strcpy(header_fn, prefix);
		for(i = 0; i < strlen(header_fn); i++)
		{
// Replace leading digits
			if(i == 0 && isdigit(header_fn[i]))
			{
				int k;
				for(k = strlen(header_fn); k >= 0; k--)
				{
					header_fn[k + 1] = header_fn[k];
				}
				header_fn[0] = '_';
			}

// Replace . with _ for header
			if(header_fn[i] == '.') 
				header_fn[i] = '_';
			else
				header_fn[i] = toupper(header_fn[i]);
		}

// Strip .h for variable
		strcpy(variable, prefix);
		suffix = strrchr(variable, '.');
		if(suffix) *suffix = 0;

// Replace leading digits
		if(isdigit(variable[0]))
		{
			int k;
			for(k = strlen(variable); k >= 0; k--)
			{
				variable[k + 1] = variable[k];
			}
			variable[0] = '_';
		}

// Print the header
		fprintf(out, "#ifndef %s\n"
					 "#define %s\n"
					 "\n"
					 "static unsigned char %s[] = \n{\n",
					 header_fn,
					 header_fn,
					 variable);

// Print the size of the file
		fprintf(out, "\t0x%02x, 0x%02x, 0x%02x, 0x%02x, \n",
			(unsigned long)(total_bytes & 0xff000000) >> 24,
			(unsigned long)(total_bytes & 0xff0000) >> 16,
			(unsigned long)(total_bytes & 0xff00) >> 8,
			(unsigned long)(total_bytes & 0xff));

		while(total_bytes > 0)
		{
			sprintf(row, "\t");
			for(i = 0; i < bytes_per_row && total_bytes > 0; i++)
			{
				if(i == 0)
					sprintf(byte, "0x%02x", fgetc(in));
				else
					sprintf(byte, ", 0x%02x", fgetc(in));
				strcat(row, byte);
				total_bytes--;
			}
			if(total_bytes > 0)
				sprintf(byte, ", \n");
			else
				sprintf(byte, "\n");

			fprintf(out, "%s%s", row, byte);
		}

		fprintf(out, "};\n\n#endif\n");
		fclose(out);
	}
}
