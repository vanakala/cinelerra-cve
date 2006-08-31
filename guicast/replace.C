#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "bcdisplayinfo.h"
#include "bchash.h"
#include "guicast.h"


class MWindow : public BC_Window
{
public:
	MWindow(int x, int y)
	 : BC_Window("Replace", x, y, 320, 200, 0, 0)
	{
	}

	int create_objects(char *input, char *output)
	{
		int x = 10, y = 10;

		add_subwindow(title = new BC_Title(x, y, "String to replace:"));
		y += title->get_h() + 10;
		add_subwindow(input_text = new BC_TextBox(x, y, 200, 1, input, 1));
		y += input_text->get_h() + 10;
		add_subwindow(title = new BC_Title(x, y, "Replacement string:"));
		y += title->get_h() + 10;
		add_subwindow(output_text = new BC_TextBox(x, y, 200, 1, output, 1));
		y += output_text->get_h() + 10;
		add_subwindow(new BC_OKButton(this));
		x = get_w() - 100;
		add_subwindow(new BC_CancelButton(this));
	}

	BC_Title *title;
	BC_TextBox *input_text, *output_text;
};


int main(int argc, char *argv[])
{
	char input[1024], output[1024];
	int result;
	BC_Hash defaults("~/.replacerc");

	input[0] = 0;
	output[0] = 0;
	defaults.load();
	defaults.get("INPUT", input);
	defaults.get("OUTPUT", output);
	BC_DisplayInfo info;
	
	
	MWindow window(info.get_abs_cursor_x(), info.get_abs_cursor_y());
	window.create_objects(input, output);
	result = window.run_window();

	if(result == 0)
	{
		strcpy(input, window.input_text->get_text());
		strcpy(output, window.output_text->get_text());
		while(--argc > 0)
		{
			FILE *file;
			char *buffer, *ptr1, *ptr2;
			long len;

			if(!(file = fopen(argv[argc], "r")))
			{
				printf("open %s for reading: %s", argv[argc], strerror(errno));
				continue;
			}

			fseek(file, 0, SEEK_END);
			len = ftell(file);
			fseek(file, 0, SEEK_SET);

			buffer = (char*)malloc(len);
			fread(buffer, 1, len, file);
			fclose(file);

			if(!(file = fopen(argv[argc], "w")))
			{
				printf("open %s for writing: %s", argv[argc], strerror(errno));
				continue;
			}

			ptr1 = buffer;
			do
			{
				ptr2 = strstr(ptr1, input);

				if(ptr2)
				{
					while(ptr1 < ptr2)
						fputc(*ptr1++, file);

					ptr1 += strlen(input);
					fprintf(file, "%s", output);
				}
				else
					while(ptr1 < buffer + len)
						fputc(*ptr1++, file);
			}while(ptr2);
		}
		printf("Replaced ""%s"" with ""%s"".\n", input, output);
	}
	else
		printf("Cancelled.\n");

	defaults.update("INPUT", input);
	defaults.update("OUTPUT", output);
	defaults.save();

}
