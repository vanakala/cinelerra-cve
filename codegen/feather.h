/* Only for odd N */
#define DO_FEATHER_MAX_N 30

#define DO_FEATHER_3(type, int_type, max, N); \
{ \
	int offset = N / 2; \
        int frame_w = input->get_w(); \
        int frame_h = input->get_h(); \
        int start_in = start_out - offset; \
        int end_in = end_out + offset; \
        type **in_rows = (type**)input->get_rows(); \
        type **out_rows = (type**)output->get_rows(); \
        int i,j; \
	int_type tmp1, tmp2; \
	int_type SR0, SR1; \
	int_type *SC0 = new int_type[(frame_w + offset * 2) * 2]; \
	memset(SC0, 0, sizeof(int_type) * (frame_w  + offset * 2) * 2); \
	for (i = start_in; i < end_in; i++) \
	{ \
		type *in_row; \
		if (i < 0) \
	           	in_row = in_rows[0]; \
		else if (i >= frame_h) \
			in_row = in_rows[frame_h - 1]; \
		else \
			in_row = in_rows[i]; \
		SR0 = SR1 = 0; \
              { \
                        type *out_row; \
                        if (i >= start_out + offset && i<= end_out + offset) \
                        	out_row = out_rows[i - offset]; \
                        else \
                        	out_row = 0; \
			int_type *SC = SC0; \
			for (j = 0; j < frame_w + offset * 2; j++) \
                        { \
                        	if (j < offset) \
                           		tmp1 = in_row[0]; \
                           	else if (j >= frame_w + offset) \
                           		tmp1 = in_row[frame_w - 1]; \
                           	else \
                           		tmp1 = in_row[j - offset]; \
				tmp2 = SR0 + tmp1; \
				SR0 = tmp1; \
				tmp1 = SR1 + tmp2; \
				SR1 = tmp2; \
				tmp2 = SC[0] + tmp1; \
				SC[0] = tmp1; \
				if (j >= offset * 2) \
					if (out_row) out_row[j - offset * 2] = (8 + SC[1] + tmp2) >> 4; \
				SC[1] = tmp2; \
				SC += 2;\
			} \
		} \
	} \
	delete [] SC0; \
}
#define DO_FEATHER_5(type, int_type, max, N); \
{ \
	int offset = N / 2; \
        int frame_w = input->get_w(); \
        int frame_h = input->get_h(); \
        int start_in = start_out - offset; \
        int end_in = end_out + offset; \
        type **in_rows = (type**)input->get_rows(); \
        type **out_rows = (type**)output->get_rows(); \
        int i,j; \
	int_type tmp1, tmp2; \
	int_type SR0, SR1, SR2, SR3; \
	int_type *SC0 = new int_type[(frame_w + offset * 2) * 4]; \
	memset(SC0, 0, sizeof(int_type) * (frame_w  + offset * 2) * 4); \
	for (i = start_in; i < end_in; i++) \
	{ \
		type *in_row; \
		if (i < 0) \
	           	in_row = in_rows[0]; \
		else if (i >= frame_h) \
			in_row = in_rows[frame_h - 1]; \
		else \
			in_row = in_rows[i]; \
		SR0 = SR1 = SR2 = SR3 = 0; \
              { \
                        type *out_row; \
                        if (i >= start_out + offset && i<= end_out + offset) \
                        	out_row = out_rows[i - offset]; \
                        else \
                        	out_row = 0; \
			int_type *SC = SC0; \
			for (j = 0; j < frame_w + offset * 2; j++) \
                        { \
                        	if (j < offset) \
                           		tmp1 = in_row[0]; \
                           	else if (j >= frame_w + offset) \
                           		tmp1 = in_row[frame_w - 1]; \
                           	else \
                           		tmp1 = in_row[j - offset]; \
				tmp2 = SR0 + tmp1; \
				SR0 = tmp1; \
				tmp1 = SR1 + tmp2; \
				SR1 = tmp2; \
				tmp2 = SR2 + tmp1; \
				SR2 = tmp1; \
				tmp1 = SR3 + tmp2; \
				SR3 = tmp2; \
				tmp2 = SC[0] + tmp1; \
				SC[0] = tmp1; \
				tmp1 = SC[1] + tmp2; \
				SC[1] = tmp2; \
				tmp2 = SC[2] + tmp1; \
				SC[2] = tmp1; \
				if (j >= offset * 2) \
					if (out_row) out_row[j - offset * 2] = (128 + SC[3] + tmp2) >> 8; \
				SC[3] = tmp2; \
				SC += 4;\
			} \
		} \
	} \
	delete [] SC0; \
}
#define DO_FEATHER_7(type, int_type, max, N); \
{ \
	int offset = N / 2; \
        int frame_w = input->get_w(); \
        int frame_h = input->get_h(); \
        int start_in = start_out - offset; \
        int end_in = end_out + offset; \
        type **in_rows = (type**)input->get_rows(); \
        type **out_rows = (type**)output->get_rows(); \
        int i,j; \
	int_type tmp1, tmp2; \
	int_type SR0, SR1, SR2, SR3, SR4, SR5; \
	int_type *SC0 = new int_type[(frame_w + offset * 2) * 6]; \
	memset(SC0, 0, sizeof(int_type) * (frame_w  + offset * 2) * 6); \
	for (i = start_in; i < end_in; i++) \
	{ \
		type *in_row; \
		if (i < 0) \
	           	in_row = in_rows[0]; \
		else if (i >= frame_h) \
			in_row = in_rows[frame_h - 1]; \
		else \
			in_row = in_rows[i]; \
		SR0 = SR1 = SR2 = SR3 = SR4 = SR5 = 0; \
              { \
                        type *out_row; \
                        if (i >= start_out + offset && i<= end_out + offset) \
                        	out_row = out_rows[i - offset]; \
                        else \
                        	out_row = 0; \
			int_type *SC = SC0; \
			for (j = 0; j < frame_w + offset * 2; j++) \
                        { \
                        	if (j < offset) \
                           		tmp1 = in_row[0]; \
                           	else if (j >= frame_w + offset) \
                           		tmp1 = in_row[frame_w - 1]; \
                           	else \
                           		tmp1 = in_row[j - offset]; \
				tmp2 = SR0 + tmp1; \
				SR0 = tmp1; \
				tmp1 = SR1 + tmp2; \
				SR1 = tmp2; \
				tmp2 = SR2 + tmp1; \
				SR2 = tmp1; \
				tmp1 = SR3 + tmp2; \
				SR3 = tmp2; \
				tmp2 = SR4 + tmp1; \
				SR4 = tmp1; \
				tmp1 = SR5 + tmp2; \
				SR5 = tmp2; \
				tmp2 = SC[0] + tmp1; \
				SC[0] = tmp1; \
				tmp1 = SC[1] + tmp2; \
				SC[1] = tmp2; \
				tmp2 = SC[2] + tmp1; \
				SC[2] = tmp1; \
				tmp1 = SC[3] + tmp2; \
				SC[3] = tmp2; \
				tmp2 = SC[4] + tmp1; \
				SC[4] = tmp1; \
				if (j >= offset * 2) \
					if (out_row) out_row[j - offset * 2] = (2048 + SC[5] + tmp2) >> 12; \
				SC[5] = tmp2; \
				SC += 6;\
			} \
		} \
	} \
	delete [] SC0; \
}
#define DO_FEATHER_9(type, int_type, max, N); \
{ \
	int offset = N / 2; \
        int frame_w = input->get_w(); \
        int frame_h = input->get_h(); \
        int start_in = start_out - offset; \
        int end_in = end_out + offset; \
        type **in_rows = (type**)input->get_rows(); \
        type **out_rows = (type**)output->get_rows(); \
        int i,j; \
	int_type tmp1, tmp2; \
	int_type SR0, SR1, SR2, SR3, SR4, SR5, SR6, SR7; \
	int_type *SC0 = new int_type[(frame_w + offset * 2) * 8]; \
	memset(SC0, 0, sizeof(int_type) * (frame_w  + offset * 2) * 8); \
	for (i = start_in; i < end_in; i++) \
	{ \
		type *in_row; \
		if (i < 0) \
	           	in_row = in_rows[0]; \
		else if (i >= frame_h) \
			in_row = in_rows[frame_h - 1]; \
		else \
			in_row = in_rows[i]; \
		SR0 = SR1 = SR2 = SR3 = SR4 = SR5 = SR6 = SR7 = 0; \
              { \
                        type *out_row; \
                        if (i >= start_out + offset && i<= end_out + offset) \
                        	out_row = out_rows[i - offset]; \
                        else \
                        	out_row = 0; \
			int_type *SC = SC0; \
			for (j = 0; j < frame_w + offset * 2; j++) \
                        { \
                        	if (j < offset) \
                           		tmp1 = in_row[0]; \
                           	else if (j >= frame_w + offset) \
                           		tmp1 = in_row[frame_w - 1]; \
                           	else \
                           		tmp1 = in_row[j - offset]; \
				tmp2 = SR0 + tmp1; \
				SR0 = tmp1; \
				tmp1 = SR1 + tmp2; \
				SR1 = tmp2; \
				tmp2 = SR2 + tmp1; \
				SR2 = tmp1; \
				tmp1 = SR3 + tmp2; \
				SR3 = tmp2; \
				tmp2 = SR4 + tmp1; \
				SR4 = tmp1; \
				tmp1 = SR5 + tmp2; \
				SR5 = tmp2; \
				tmp2 = SR6 + tmp1; \
				SR6 = tmp1; \
				tmp1 = SR7 + tmp2; \
				SR7 = tmp2; \
				tmp2 = SC[0] + tmp1; \
				SC[0] = tmp1; \
				tmp1 = SC[1] + tmp2; \
				SC[1] = tmp2; \
				tmp2 = SC[2] + tmp1; \
				SC[2] = tmp1; \
				tmp1 = SC[3] + tmp2; \
				SC[3] = tmp2; \
				tmp2 = SC[4] + tmp1; \
				SC[4] = tmp1; \
				tmp1 = SC[5] + tmp2; \
				SC[5] = tmp2; \
				tmp2 = SC[6] + tmp1; \
				SC[6] = tmp1; \
				if (j >= offset * 2) \
					if (out_row) out_row[j - offset * 2] = (32768 + SC[7] + tmp2) >> 16; \
				SC[7] = tmp2; \
				SC += 8;\
			} \
		} \
	} \
	delete [] SC0; \
}
#define DO_FEATHER_11(type, int_type, max, N); \
{ \
	int offset = N / 2; \
        int frame_w = input->get_w(); \
        int frame_h = input->get_h(); \
        int start_in = start_out - offset; \
        int end_in = end_out + offset; \
        type **in_rows = (type**)input->get_rows(); \
        type **out_rows = (type**)output->get_rows(); \
        int i,j; \
	int_type tmp1, tmp2; \
	int_type SR0, SR1, SR2, SR3, SR4, SR5, SR6, SR7, SR8, SR9; \
	int_type *SC0 = new int_type[(frame_w + offset * 2) * 10]; \
	memset(SC0, 0, sizeof(int_type) * (frame_w  + offset * 2) * 10); \
	for (i = start_in; i < end_in; i++) \
	{ \
		type *in_row; \
		if (i < 0) \
	           	in_row = in_rows[0]; \
		else if (i >= frame_h) \
			in_row = in_rows[frame_h - 1]; \
		else \
			in_row = in_rows[i]; \
		SR0 = SR1 = SR2 = SR3 = SR4 = SR5 = SR6 = SR7 = SR8 = SR9 = 0; \
              { \
                        type *out_row; \
                        if (i >= start_out + offset && i<= end_out + offset) \
                        	out_row = out_rows[i - offset]; \
                        else \
                        	out_row = 0; \
			int_type *SC = SC0; \
			for (j = 0; j < frame_w + offset * 2; j++) \
                        { \
                        	if (j < offset) \
                           		tmp1 = in_row[0]; \
                           	else if (j >= frame_w + offset) \
                           		tmp1 = in_row[frame_w - 1]; \
                           	else \
                           		tmp1 = in_row[j - offset]; \
				tmp2 = SR0 + tmp1; \
				SR0 = tmp1; \
				tmp1 = SR1 + tmp2; \
				SR1 = tmp2; \
				tmp2 = SR2 + tmp1; \
				SR2 = tmp1; \
				tmp1 = SR3 + tmp2; \
				SR3 = tmp2; \
				tmp2 = SR4 + tmp1; \
				SR4 = tmp1; \
				tmp1 = SR5 + tmp2; \
				SR5 = tmp2; \
				tmp2 = SR6 + tmp1; \
				SR6 = tmp1; \
				tmp1 = SR7 + tmp2; \
				SR7 = tmp2; \
				tmp2 = SR8 + tmp1; \
				SR8 = tmp1; \
				tmp1 = SR9 + tmp2; \
				SR9 = tmp2; \
				tmp2 = SC[0] + tmp1; \
				SC[0] = tmp1; \
				tmp1 = SC[1] + tmp2; \
				SC[1] = tmp2; \
				tmp2 = SC[2] + tmp1; \
				SC[2] = tmp1; \
				tmp1 = SC[3] + tmp2; \
				SC[3] = tmp2; \
				tmp2 = SC[4] + tmp1; \
				SC[4] = tmp1; \
				tmp1 = SC[5] + tmp2; \
				SC[5] = tmp2; \
				tmp2 = SC[6] + tmp1; \
				SC[6] = tmp1; \
				tmp1 = SC[7] + tmp2; \
				SC[7] = tmp2; \
				tmp2 = SC[8] + tmp1; \
				SC[8] = tmp1; \
				if (j >= offset * 2) \
					if (out_row) out_row[j - offset * 2] = (524288 + SC[9] + tmp2) >> 20; \
				SC[9] = tmp2; \
				SC += 10;\
			} \
		} \
	} \
	delete [] SC0; \
}
#define DO_FEATHER_13(type, int_type, max, N); \
{ \
	int offset = N / 2; \
        int frame_w = input->get_w(); \
        int frame_h = input->get_h(); \
        int start_in = start_out - offset; \
        int end_in = end_out + offset; \
        type **in_rows = (type**)input->get_rows(); \
        type **out_rows = (type**)output->get_rows(); \
        int i,j; \
	int_type tmp1, tmp2; \
	int_type SR0, SR1, SR2, SR3, SR4, SR5, SR6, SR7, SR8, SR9, SR10, SR11; \
	int_type *SC0 = new int_type[(frame_w + offset * 2) * 12]; \
	memset(SC0, 0, sizeof(int_type) * (frame_w  + offset * 2) * 12); \
	for (i = start_in; i < end_in; i++) \
	{ \
		type *in_row; \
		if (i < 0) \
	           	in_row = in_rows[0]; \
		else if (i >= frame_h) \
			in_row = in_rows[frame_h - 1]; \
		else \
			in_row = in_rows[i]; \
		SR0 = SR1 = SR2 = SR3 = SR4 = SR5 = SR6 = SR7 = SR8 = SR9 = SR10 = SR11 = 0; \
              { \
                        type *out_row; \
                        if (i >= start_out + offset && i<= end_out + offset) \
                        	out_row = out_rows[i - offset]; \
                        else \
                        	out_row = 0; \
			int_type *SC = SC0; \
			for (j = 0; j < frame_w + offset * 2; j++) \
                        { \
                        	if (j < offset) \
                           		tmp1 = in_row[0]; \
                           	else if (j >= frame_w + offset) \
                           		tmp1 = in_row[frame_w - 1]; \
                           	else \
                           		tmp1 = in_row[j - offset]; \
				tmp2 = SR0 + tmp1; \
				SR0 = tmp1; \
				tmp1 = SR1 + tmp2; \
				SR1 = tmp2; \
				tmp2 = SR2 + tmp1; \
				SR2 = tmp1; \
				tmp1 = SR3 + tmp2; \
				SR3 = tmp2; \
				tmp2 = SR4 + tmp1; \
				SR4 = tmp1; \
				tmp1 = SR5 + tmp2; \
				SR5 = tmp2; \
				tmp2 = SR6 + tmp1; \
				SR6 = tmp1; \
				tmp1 = SR7 + tmp2; \
				SR7 = tmp2; \
				tmp2 = SR8 + tmp1; \
				SR8 = tmp1; \
				tmp1 = SR9 + tmp2; \
				SR9 = tmp2; \
				tmp2 = SR10 + tmp1; \
				SR10 = tmp1; \
				tmp1 = SR11 + tmp2; \
				SR11 = tmp2; \
				tmp2 = SC[0] + tmp1; \
				SC[0] = tmp1; \
				tmp1 = SC[1] + tmp2; \
				SC[1] = tmp2; \
				tmp2 = SC[2] + tmp1; \
				SC[2] = tmp1; \
				tmp1 = SC[3] + tmp2; \
				SC[3] = tmp2; \
				tmp2 = SC[4] + tmp1; \
				SC[4] = tmp1; \
				tmp1 = SC[5] + tmp2; \
				SC[5] = tmp2; \
				tmp2 = SC[6] + tmp1; \
				SC[6] = tmp1; \
				tmp1 = SC[7] + tmp2; \
				SC[7] = tmp2; \
				tmp2 = SC[8] + tmp1; \
				SC[8] = tmp1; \
				tmp1 = SC[9] + tmp2; \
				SC[9] = tmp2; \
				tmp2 = SC[10] + tmp1; \
				SC[10] = tmp1; \
				if (j >= offset * 2) \
					if (out_row) out_row[j - offset * 2] = (8388608 + SC[11] + tmp2) >> 24; \
				SC[11] = tmp2; \
				SC += 12;\
			} \
		} \
	} \
	delete [] SC0; \
}
#define DO_FEATHER_15(type, int_type, max, N); \
{ \
	int offset = N / 2; \
        int frame_w = input->get_w(); \
        int frame_h = input->get_h(); \
        int start_in = start_out - offset; \
        int end_in = end_out + offset; \
        type **in_rows = (type**)input->get_rows(); \
        type **out_rows = (type**)output->get_rows(); \
        int i,j; \
	int_type tmp1, tmp2; \
	int_type SR0, SR1, SR2, SR3, SR4, SR5, SR6, SR7, SR8, SR9, SR10, SR11, SR12, SR13; \
	int_type *SC0 = new int_type[(frame_w + offset * 2) * 14]; \
	memset(SC0, 0, sizeof(int_type) * (frame_w  + offset * 2) * 14); \
	for (i = start_in; i < end_in; i++) \
	{ \
		type *in_row; \
		if (i < 0) \
	           	in_row = in_rows[0]; \
		else if (i >= frame_h) \
			in_row = in_rows[frame_h - 1]; \
		else \
			in_row = in_rows[i]; \
		SR0 = SR1 = SR2 = SR3 = SR4 = SR5 = SR6 = SR7 = SR8 = SR9 = SR10 = SR11 = SR12 = SR13 = 0; \
              { \
                        type *out_row; \
                        if (i >= start_out + offset && i<= end_out + offset) \
                        	out_row = out_rows[i - offset]; \
                        else \
                        	out_row = 0; \
			int_type *SC = SC0; \
			for (j = 0; j < frame_w + offset * 2; j++) \
                        { \
                        	if (j < offset) \
                           		tmp1 = in_row[0]; \
                           	else if (j >= frame_w + offset) \
                           		tmp1 = in_row[frame_w - 1]; \
                           	else \
                           		tmp1 = in_row[j - offset]; \
				tmp2 = SR0 + tmp1; \
				SR0 = tmp1; \
				tmp1 = SR1 + tmp2; \
				SR1 = tmp2; \
				tmp2 = SR2 + tmp1; \
				SR2 = tmp1; \
				tmp1 = SR3 + tmp2; \
				SR3 = tmp2; \
				tmp2 = SR4 + tmp1; \
				SR4 = tmp1; \
				tmp1 = SR5 + tmp2; \
				SR5 = tmp2; \
				tmp2 = SR6 + tmp1; \
				SR6 = tmp1; \
				tmp1 = SR7 + tmp2; \
				SR7 = tmp2; \
				tmp2 = SR8 + tmp1; \
				SR8 = tmp1; \
				tmp1 = SR9 + tmp2; \
				SR9 = tmp2; \
				tmp2 = SR10 + tmp1; \
				SR10 = tmp1; \
				tmp1 = SR11 + tmp2; \
				SR11 = tmp2; \
				tmp2 = SR12 + tmp1; \
				SR12 = tmp1; \
				tmp1 = SR13 + tmp2; \
				SR13 = tmp2; \
				tmp2 = SC[0] + tmp1; \
				SC[0] = tmp1; \
				tmp1 = SC[1] + tmp2; \
				SC[1] = tmp2; \
				tmp2 = SC[2] + tmp1; \
				SC[2] = tmp1; \
				tmp1 = SC[3] + tmp2; \
				SC[3] = tmp2; \
				tmp2 = SC[4] + tmp1; \
				SC[4] = tmp1; \
				tmp1 = SC[5] + tmp2; \
				SC[5] = tmp2; \
				tmp2 = SC[6] + tmp1; \
				SC[6] = tmp1; \
				tmp1 = SC[7] + tmp2; \
				SC[7] = tmp2; \
				tmp2 = SC[8] + tmp1; \
				SC[8] = tmp1; \
				tmp1 = SC[9] + tmp2; \
				SC[9] = tmp2; \
				tmp2 = SC[10] + tmp1; \
				SC[10] = tmp1; \
				tmp1 = SC[11] + tmp2; \
				SC[11] = tmp2; \
				tmp2 = SC[12] + tmp1; \
				SC[12] = tmp1; \
				if (j >= offset * 2) \
					if (out_row) out_row[j - offset * 2] = (((uint64_t)1 <<27) + SC[13] + tmp2) >> 28; \
				SC[13] = tmp2; \
				SC += 14;\
			} \
		} \
	} \
	delete [] SC0; \
}
#define DO_FEATHER_17(type, int_type, max, N); \
{ \
	int offset = N / 2; \
        int frame_w = input->get_w(); \
        int frame_h = input->get_h(); \
        int start_in = start_out - offset; \
        int end_in = end_out + offset; \
        type **in_rows = (type**)input->get_rows(); \
        type **out_rows = (type**)output->get_rows(); \
        int i,j; \
	int_type tmp1, tmp2; \
	int_type SR0, SR1, SR2, SR3, SR4, SR5, SR6, SR7, SR8, SR9, SR10, SR11, SR12, SR13, SR14, SR15; \
	int_type *SC0 = new int_type[(frame_w + offset * 2) * 16]; \
	memset(SC0, 0, sizeof(int_type) * (frame_w  + offset * 2) * 16); \
	for (i = start_in; i < end_in; i++) \
	{ \
		type *in_row; \
		if (i < 0) \
	           	in_row = in_rows[0]; \
		else if (i >= frame_h) \
			in_row = in_rows[frame_h - 1]; \
		else \
			in_row = in_rows[i]; \
		SR0 = SR1 = SR2 = SR3 = SR4 = SR5 = SR6 = SR7 = SR8 = SR9 = SR10 = SR11 = SR12 = SR13 = SR14 = SR15 = 0; \
              { \
                        type *out_row; \
                        if (i >= start_out + offset && i<= end_out + offset) \
                        	out_row = out_rows[i - offset]; \
                        else \
                        	out_row = 0; \
			int_type *SC = SC0; \
			for (j = 0; j < frame_w + offset * 2; j++) \
                        { \
                        	if (j < offset) \
                           		tmp1 = in_row[0]; \
                           	else if (j >= frame_w + offset) \
                           		tmp1 = in_row[frame_w - 1]; \
                           	else \
                           		tmp1 = in_row[j - offset]; \
				tmp2 = SR0 + tmp1; \
				SR0 = tmp1; \
				tmp1 = SR1 + tmp2; \
				SR1 = tmp2; \
				tmp2 = SR2 + tmp1; \
				SR2 = tmp1; \
				tmp1 = SR3 + tmp2; \
				SR3 = tmp2; \
				tmp2 = SR4 + tmp1; \
				SR4 = tmp1; \
				tmp1 = SR5 + tmp2; \
				SR5 = tmp2; \
				tmp2 = SR6 + tmp1; \
				SR6 = tmp1; \
				tmp1 = SR7 + tmp2; \
				SR7 = tmp2; \
				tmp2 = SR8 + tmp1; \
				SR8 = tmp1; \
				tmp1 = SR9 + tmp2; \
				SR9 = tmp2; \
				tmp2 = SR10 + tmp1; \
				SR10 = tmp1; \
				tmp1 = SR11 + tmp2; \
				SR11 = tmp2; \
				tmp2 = SR12 + tmp1; \
				SR12 = tmp1; \
				tmp1 = SR13 + tmp2; \
				SR13 = tmp2; \
				tmp2 = SR14 + tmp1; \
				SR14 = tmp1; \
				tmp1 = SR15 + tmp2; \
				SR15 = tmp2; \
				tmp2 = SC[0] + tmp1; \
				SC[0] = tmp1; \
				tmp1 = SC[1] + tmp2; \
				SC[1] = tmp2; \
				tmp2 = SC[2] + tmp1; \
				SC[2] = tmp1; \
				tmp1 = SC[3] + tmp2; \
				SC[3] = tmp2; \
				tmp2 = SC[4] + tmp1; \
				SC[4] = tmp1; \
				tmp1 = SC[5] + tmp2; \
				SC[5] = tmp2; \
				tmp2 = SC[6] + tmp1; \
				SC[6] = tmp1; \
				tmp1 = SC[7] + tmp2; \
				SC[7] = tmp2; \
				tmp2 = SC[8] + tmp1; \
				SC[8] = tmp1; \
				tmp1 = SC[9] + tmp2; \
				SC[9] = tmp2; \
				tmp2 = SC[10] + tmp1; \
				SC[10] = tmp1; \
				tmp1 = SC[11] + tmp2; \
				SC[11] = tmp2; \
				tmp2 = SC[12] + tmp1; \
				SC[12] = tmp1; \
				tmp1 = SC[13] + tmp2; \
				SC[13] = tmp2; \
				tmp2 = SC[14] + tmp1; \
				SC[14] = tmp1; \
				if (j >= offset * 2) \
					if (out_row) out_row[j - offset * 2] = (((uint64_t)1 <<31) + SC[15] + tmp2) >> 32; \
				SC[15] = tmp2; \
				SC += 16;\
			} \
		} \
	} \
	delete [] SC0; \
}
#define DO_FEATHER_19(type, int_type, max, N); \
{ \
	int offset = N / 2; \
        int frame_w = input->get_w(); \
        int frame_h = input->get_h(); \
        int start_in = start_out - offset; \
        int end_in = end_out + offset; \
        type **in_rows = (type**)input->get_rows(); \
        type **out_rows = (type**)output->get_rows(); \
        int i,j; \
	int_type tmp1, tmp2; \
	int_type SR0, SR1, SR2, SR3, SR4, SR5, SR6, SR7, SR8, SR9, SR10, SR11, SR12, SR13, SR14, SR15, SR16, SR17; \
	int_type *SC0 = new int_type[(frame_w + offset * 2) * 18]; \
	memset(SC0, 0, sizeof(int_type) * (frame_w  + offset * 2) * 18); \
	for (i = start_in; i < end_in; i++) \
	{ \
		type *in_row; \
		if (i < 0) \
	           	in_row = in_rows[0]; \
		else if (i >= frame_h) \
			in_row = in_rows[frame_h - 1]; \
		else \
			in_row = in_rows[i]; \
		SR0 = SR1 = SR2 = SR3 = SR4 = SR5 = SR6 = SR7 = SR8 = SR9 = SR10 = SR11 = SR12 = SR13 = SR14 = SR15 = SR16 = SR17 = 0; \
              { \
                        type *out_row; \
                        if (i >= start_out + offset && i<= end_out + offset) \
                        	out_row = out_rows[i - offset]; \
                        else \
                        	out_row = 0; \
			int_type *SC = SC0; \
			for (j = 0; j < frame_w + offset * 2; j++) \
                        { \
                        	if (j < offset) \
                           		tmp1 = in_row[0]; \
                           	else if (j >= frame_w + offset) \
                           		tmp1 = in_row[frame_w - 1]; \
                           	else \
                           		tmp1 = in_row[j - offset]; \
				tmp2 = SR0 + tmp1; \
				SR0 = tmp1; \
				tmp1 = SR1 + tmp2; \
				SR1 = tmp2; \
				tmp2 = SR2 + tmp1; \
				SR2 = tmp1; \
				tmp1 = SR3 + tmp2; \
				SR3 = tmp2; \
				tmp2 = SR4 + tmp1; \
				SR4 = tmp1; \
				tmp1 = SR5 + tmp2; \
				SR5 = tmp2; \
				tmp2 = SR6 + tmp1; \
				SR6 = tmp1; \
				tmp1 = SR7 + tmp2; \
				SR7 = tmp2; \
				tmp2 = SR8 + tmp1; \
				SR8 = tmp1; \
				tmp1 = SR9 + tmp2; \
				SR9 = tmp2; \
				tmp2 = SR10 + tmp1; \
				SR10 = tmp1; \
				tmp1 = SR11 + tmp2; \
				SR11 = tmp2; \
				tmp2 = SR12 + tmp1; \
				SR12 = tmp1; \
				tmp1 = SR13 + tmp2; \
				SR13 = tmp2; \
				tmp2 = SR14 + tmp1; \
				SR14 = tmp1; \
				tmp1 = SR15 + tmp2; \
				SR15 = tmp2; \
				tmp2 = SR16 + tmp1; \
				SR16 = tmp1; \
				tmp1 = SR17 + tmp2; \
				SR17 = tmp2; \
				tmp2 = SC[0] + tmp1; \
				SC[0] = tmp1; \
				tmp1 = SC[1] + tmp2; \
				SC[1] = tmp2; \
				tmp2 = SC[2] + tmp1; \
				SC[2] = tmp1; \
				tmp1 = SC[3] + tmp2; \
				SC[3] = tmp2; \
				tmp2 = SC[4] + tmp1; \
				SC[4] = tmp1; \
				tmp1 = SC[5] + tmp2; \
				SC[5] = tmp2; \
				tmp2 = SC[6] + tmp1; \
				SC[6] = tmp1; \
				tmp1 = SC[7] + tmp2; \
				SC[7] = tmp2; \
				tmp2 = SC[8] + tmp1; \
				SC[8] = tmp1; \
				tmp1 = SC[9] + tmp2; \
				SC[9] = tmp2; \
				tmp2 = SC[10] + tmp1; \
				SC[10] = tmp1; \
				tmp1 = SC[11] + tmp2; \
				SC[11] = tmp2; \
				tmp2 = SC[12] + tmp1; \
				SC[12] = tmp1; \
				tmp1 = SC[13] + tmp2; \
				SC[13] = tmp2; \
				tmp2 = SC[14] + tmp1; \
				SC[14] = tmp1; \
				tmp1 = SC[15] + tmp2; \
				SC[15] = tmp2; \
				tmp2 = SC[16] + tmp1; \
				SC[16] = tmp1; \
				if (j >= offset * 2) \
					if (out_row) out_row[j - offset * 2] = (((uint64_t)1 <<35) + SC[17] + tmp2) >> 36; \
				SC[17] = tmp2; \
				SC += 18;\
			} \
		} \
	} \
	delete [] SC0; \
}
#define DO_FEATHER_21(type, int_type, max, N); \
{ \
	int offset = N / 2; \
        int frame_w = input->get_w(); \
        int frame_h = input->get_h(); \
        int start_in = start_out - offset; \
        int end_in = end_out + offset; \
        type **in_rows = (type**)input->get_rows(); \
        type **out_rows = (type**)output->get_rows(); \
        int i,j; \
	int_type tmp1, tmp2; \
	int_type SR0, SR1, SR2, SR3, SR4, SR5, SR6, SR7, SR8, SR9, SR10, SR11, SR12, SR13, SR14, SR15, SR16, SR17, SR18, SR19; \
	int_type *SC0 = new int_type[(frame_w + offset * 2) * 20]; \
	memset(SC0, 0, sizeof(int_type) * (frame_w  + offset * 2) * 20); \
	for (i = start_in; i < end_in; i++) \
	{ \
		type *in_row; \
		if (i < 0) \
	           	in_row = in_rows[0]; \
		else if (i >= frame_h) \
			in_row = in_rows[frame_h - 1]; \
		else \
			in_row = in_rows[i]; \
		SR0 = SR1 = SR2 = SR3 = SR4 = SR5 = SR6 = SR7 = SR8 = SR9 = SR10 = SR11 = SR12 = SR13 = SR14 = SR15 = SR16 = SR17 = SR18 = SR19 = 0; \
              { \
                        type *out_row; \
                        if (i >= start_out + offset && i<= end_out + offset) \
                        	out_row = out_rows[i - offset]; \
                        else \
                        	out_row = 0; \
			int_type *SC = SC0; \
			for (j = 0; j < frame_w + offset * 2; j++) \
                        { \
                        	if (j < offset) \
                           		tmp1 = in_row[0]; \
                           	else if (j >= frame_w + offset) \
                           		tmp1 = in_row[frame_w - 1]; \
                           	else \
                           		tmp1 = in_row[j - offset]; \
				tmp2 = SR0 + tmp1; \
				SR0 = tmp1; \
				tmp1 = SR1 + tmp2; \
				SR1 = tmp2; \
				tmp2 = SR2 + tmp1; \
				SR2 = tmp1; \
				tmp1 = SR3 + tmp2; \
				SR3 = tmp2; \
				tmp2 = SR4 + tmp1; \
				SR4 = tmp1; \
				tmp1 = SR5 + tmp2; \
				SR5 = tmp2; \
				tmp2 = SR6 + tmp1; \
				SR6 = tmp1; \
				tmp1 = SR7 + tmp2; \
				SR7 = tmp2; \
				tmp2 = SR8 + tmp1; \
				SR8 = tmp1; \
				tmp1 = SR9 + tmp2; \
				SR9 = tmp2; \
				tmp2 = SR10 + tmp1; \
				SR10 = tmp1; \
				tmp1 = SR11 + tmp2; \
				SR11 = tmp2; \
				tmp2 = SR12 + tmp1; \
				SR12 = tmp1; \
				tmp1 = SR13 + tmp2; \
				SR13 = tmp2; \
				tmp2 = SR14 + tmp1; \
				SR14 = tmp1; \
				tmp1 = SR15 + tmp2; \
				SR15 = tmp2; \
				tmp2 = SR16 + tmp1; \
				SR16 = tmp1; \
				tmp1 = SR17 + tmp2; \
				SR17 = tmp2; \
				tmp2 = SR18 + tmp1; \
				SR18 = tmp1; \
				tmp1 = SR19 + tmp2; \
				SR19 = tmp2; \
				tmp2 = SC[0] + tmp1; \
				SC[0] = tmp1; \
				tmp1 = SC[1] + tmp2; \
				SC[1] = tmp2; \
				tmp2 = SC[2] + tmp1; \
				SC[2] = tmp1; \
				tmp1 = SC[3] + tmp2; \
				SC[3] = tmp2; \
				tmp2 = SC[4] + tmp1; \
				SC[4] = tmp1; \
				tmp1 = SC[5] + tmp2; \
				SC[5] = tmp2; \
				tmp2 = SC[6] + tmp1; \
				SC[6] = tmp1; \
				tmp1 = SC[7] + tmp2; \
				SC[7] = tmp2; \
				tmp2 = SC[8] + tmp1; \
				SC[8] = tmp1; \
				tmp1 = SC[9] + tmp2; \
				SC[9] = tmp2; \
				tmp2 = SC[10] + tmp1; \
				SC[10] = tmp1; \
				tmp1 = SC[11] + tmp2; \
				SC[11] = tmp2; \
				tmp2 = SC[12] + tmp1; \
				SC[12] = tmp1; \
				tmp1 = SC[13] + tmp2; \
				SC[13] = tmp2; \
				tmp2 = SC[14] + tmp1; \
				SC[14] = tmp1; \
				tmp1 = SC[15] + tmp2; \
				SC[15] = tmp2; \
				tmp2 = SC[16] + tmp1; \
				SC[16] = tmp1; \
				tmp1 = SC[17] + tmp2; \
				SC[17] = tmp2; \
				tmp2 = SC[18] + tmp1; \
				SC[18] = tmp1; \
				if (j >= offset * 2) \
					if (out_row) out_row[j - offset * 2] = (((uint64_t)1 <<39) + SC[19] + tmp2) >> 40; \
				SC[19] = tmp2; \
				SC += 20;\
			} \
		} \
	} \
	delete [] SC0; \
}
#define DO_FEATHER_23(type, int_type, max, N); \
{ \
	int offset = N / 2; \
        int frame_w = input->get_w(); \
        int frame_h = input->get_h(); \
        int start_in = start_out - offset; \
        int end_in = end_out + offset; \
        type **in_rows = (type**)input->get_rows(); \
        type **out_rows = (type**)output->get_rows(); \
        int i,j; \
	int_type tmp1, tmp2; \
	int_type SR0, SR1, SR2, SR3, SR4, SR5, SR6, SR7, SR8, SR9, SR10, SR11, SR12, SR13, SR14, SR15, SR16, SR17, SR18, SR19, SR20, SR21; \
	int_type *SC0 = new int_type[(frame_w + offset * 2) * 22]; \
	memset(SC0, 0, sizeof(int_type) * (frame_w  + offset * 2) * 22); \
	for (i = start_in; i < end_in; i++) \
	{ \
		type *in_row; \
		if (i < 0) \
	           	in_row = in_rows[0]; \
		else if (i >= frame_h) \
			in_row = in_rows[frame_h - 1]; \
		else \
			in_row = in_rows[i]; \
		SR0 = SR1 = SR2 = SR3 = SR4 = SR5 = SR6 = SR7 = SR8 = SR9 = SR10 = SR11 = SR12 = SR13 = SR14 = SR15 = SR16 = SR17 = SR18 = SR19 = SR20 = SR21 = 0; \
              { \
                        type *out_row; \
                        if (i >= start_out + offset && i<= end_out + offset) \
                        	out_row = out_rows[i - offset]; \
                        else \
                        	out_row = 0; \
			int_type *SC = SC0; \
			for (j = 0; j < frame_w + offset * 2; j++) \
                        { \
                        	if (j < offset) \
                           		tmp1 = in_row[0]; \
                           	else if (j >= frame_w + offset) \
                           		tmp1 = in_row[frame_w - 1]; \
                           	else \
                           		tmp1 = in_row[j - offset]; \
				tmp2 = SR0 + tmp1; \
				SR0 = tmp1; \
				tmp1 = SR1 + tmp2; \
				SR1 = tmp2; \
				tmp2 = SR2 + tmp1; \
				SR2 = tmp1; \
				tmp1 = SR3 + tmp2; \
				SR3 = tmp2; \
				tmp2 = SR4 + tmp1; \
				SR4 = tmp1; \
				tmp1 = SR5 + tmp2; \
				SR5 = tmp2; \
				tmp2 = SR6 + tmp1; \
				SR6 = tmp1; \
				tmp1 = SR7 + tmp2; \
				SR7 = tmp2; \
				tmp2 = SR8 + tmp1; \
				SR8 = tmp1; \
				tmp1 = SR9 + tmp2; \
				SR9 = tmp2; \
				tmp2 = SR10 + tmp1; \
				SR10 = tmp1; \
				tmp1 = SR11 + tmp2; \
				SR11 = tmp2; \
				tmp2 = SR12 + tmp1; \
				SR12 = tmp1; \
				tmp1 = SR13 + tmp2; \
				SR13 = tmp2; \
				tmp2 = SR14 + tmp1; \
				SR14 = tmp1; \
				tmp1 = SR15 + tmp2; \
				SR15 = tmp2; \
				tmp2 = SR16 + tmp1; \
				SR16 = tmp1; \
				tmp1 = SR17 + tmp2; \
				SR17 = tmp2; \
				tmp2 = SR18 + tmp1; \
				SR18 = tmp1; \
				tmp1 = SR19 + tmp2; \
				SR19 = tmp2; \
				tmp2 = SR20 + tmp1; \
				SR20 = tmp1; \
				tmp1 = SR21 + tmp2; \
				SR21 = tmp2; \
				tmp2 = SC[0] + tmp1; \
				SC[0] = tmp1; \
				tmp1 = SC[1] + tmp2; \
				SC[1] = tmp2; \
				tmp2 = SC[2] + tmp1; \
				SC[2] = tmp1; \
				tmp1 = SC[3] + tmp2; \
				SC[3] = tmp2; \
				tmp2 = SC[4] + tmp1; \
				SC[4] = tmp1; \
				tmp1 = SC[5] + tmp2; \
				SC[5] = tmp2; \
				tmp2 = SC[6] + tmp1; \
				SC[6] = tmp1; \
				tmp1 = SC[7] + tmp2; \
				SC[7] = tmp2; \
				tmp2 = SC[8] + tmp1; \
				SC[8] = tmp1; \
				tmp1 = SC[9] + tmp2; \
				SC[9] = tmp2; \
				tmp2 = SC[10] + tmp1; \
				SC[10] = tmp1; \
				tmp1 = SC[11] + tmp2; \
				SC[11] = tmp2; \
				tmp2 = SC[12] + tmp1; \
				SC[12] = tmp1; \
				tmp1 = SC[13] + tmp2; \
				SC[13] = tmp2; \
				tmp2 = SC[14] + tmp1; \
				SC[14] = tmp1; \
				tmp1 = SC[15] + tmp2; \
				SC[15] = tmp2; \
				tmp2 = SC[16] + tmp1; \
				SC[16] = tmp1; \
				tmp1 = SC[17] + tmp2; \
				SC[17] = tmp2; \
				tmp2 = SC[18] + tmp1; \
				SC[18] = tmp1; \
				tmp1 = SC[19] + tmp2; \
				SC[19] = tmp2; \
				tmp2 = SC[20] + tmp1; \
				SC[20] = tmp1; \
				if (j >= offset * 2) \
					if (out_row) out_row[j - offset * 2] = (((uint64_t)1 <<43) + SC[21] + tmp2) >> 44; \
				SC[21] = tmp2; \
				SC += 22;\
			} \
		} \
	} \
	delete [] SC0; \
}
#define DO_FEATHER_25(type, int_type, max, N); \
{ \
	int offset = N / 2; \
        int frame_w = input->get_w(); \
        int frame_h = input->get_h(); \
        int start_in = start_out - offset; \
        int end_in = end_out + offset; \
        type **in_rows = (type**)input->get_rows(); \
        type **out_rows = (type**)output->get_rows(); \
        int i,j; \
	int_type tmp1, tmp2; \
	int_type SR0, SR1, SR2, SR3, SR4, SR5, SR6, SR7, SR8, SR9, SR10, SR11, SR12, SR13, SR14, SR15, SR16, SR17, SR18, SR19, SR20, SR21, SR22, SR23; \
	int_type *SC0 = new int_type[(frame_w + offset * 2) * 24]; \
	memset(SC0, 0, sizeof(int_type) * (frame_w  + offset * 2) * 24); \
	for (i = start_in; i < end_in; i++) \
	{ \
		type *in_row; \
		if (i < 0) \
	           	in_row = in_rows[0]; \
		else if (i >= frame_h) \
			in_row = in_rows[frame_h - 1]; \
		else \
			in_row = in_rows[i]; \
		SR0 = SR1 = SR2 = SR3 = SR4 = SR5 = SR6 = SR7 = SR8 = SR9 = SR10 = SR11 = SR12 = SR13 = SR14 = SR15 = SR16 = SR17 = SR18 = SR19 = SR20 = SR21 = SR22 = SR23 = 0; \
              { \
                        type *out_row; \
                        if (i >= start_out + offset && i<= end_out + offset) \
                        	out_row = out_rows[i - offset]; \
                        else \
                        	out_row = 0; \
			int_type *SC = SC0; \
			for (j = 0; j < frame_w + offset * 2; j++) \
                        { \
                        	if (j < offset) \
                           		tmp1 = in_row[0]; \
                           	else if (j >= frame_w + offset) \
                           		tmp1 = in_row[frame_w - 1]; \
                           	else \
                           		tmp1 = in_row[j - offset]; \
				tmp2 = SR0 + tmp1; \
				SR0 = tmp1; \
				tmp1 = SR1 + tmp2; \
				SR1 = tmp2; \
				tmp2 = SR2 + tmp1; \
				SR2 = tmp1; \
				tmp1 = SR3 + tmp2; \
				SR3 = tmp2; \
				tmp2 = SR4 + tmp1; \
				SR4 = tmp1; \
				tmp1 = SR5 + tmp2; \
				SR5 = tmp2; \
				tmp2 = SR6 + tmp1; \
				SR6 = tmp1; \
				tmp1 = SR7 + tmp2; \
				SR7 = tmp2; \
				tmp2 = SR8 + tmp1; \
				SR8 = tmp1; \
				tmp1 = SR9 + tmp2; \
				SR9 = tmp2; \
				tmp2 = SR10 + tmp1; \
				SR10 = tmp1; \
				tmp1 = SR11 + tmp2; \
				SR11 = tmp2; \
				tmp2 = SR12 + tmp1; \
				SR12 = tmp1; \
				tmp1 = SR13 + tmp2; \
				SR13 = tmp2; \
				tmp2 = SR14 + tmp1; \
				SR14 = tmp1; \
				tmp1 = SR15 + tmp2; \
				SR15 = tmp2; \
				tmp2 = SR16 + tmp1; \
				SR16 = tmp1; \
				tmp1 = SR17 + tmp2; \
				SR17 = tmp2; \
				tmp2 = SR18 + tmp1; \
				SR18 = tmp1; \
				tmp1 = SR19 + tmp2; \
				SR19 = tmp2; \
				tmp2 = SR20 + tmp1; \
				SR20 = tmp1; \
				tmp1 = SR21 + tmp2; \
				SR21 = tmp2; \
				tmp2 = SR22 + tmp1; \
				SR22 = tmp1; \
				tmp1 = SR23 + tmp2; \
				SR23 = tmp2; \
				tmp2 = SC[0] + tmp1; \
				SC[0] = tmp1; \
				tmp1 = SC[1] + tmp2; \
				SC[1] = tmp2; \
				tmp2 = SC[2] + tmp1; \
				SC[2] = tmp1; \
				tmp1 = SC[3] + tmp2; \
				SC[3] = tmp2; \
				tmp2 = SC[4] + tmp1; \
				SC[4] = tmp1; \
				tmp1 = SC[5] + tmp2; \
				SC[5] = tmp2; \
				tmp2 = SC[6] + tmp1; \
				SC[6] = tmp1; \
				tmp1 = SC[7] + tmp2; \
				SC[7] = tmp2; \
				tmp2 = SC[8] + tmp1; \
				SC[8] = tmp1; \
				tmp1 = SC[9] + tmp2; \
				SC[9] = tmp2; \
				tmp2 = SC[10] + tmp1; \
				SC[10] = tmp1; \
				tmp1 = SC[11] + tmp2; \
				SC[11] = tmp2; \
				tmp2 = SC[12] + tmp1; \
				SC[12] = tmp1; \
				tmp1 = SC[13] + tmp2; \
				SC[13] = tmp2; \
				tmp2 = SC[14] + tmp1; \
				SC[14] = tmp1; \
				tmp1 = SC[15] + tmp2; \
				SC[15] = tmp2; \
				tmp2 = SC[16] + tmp1; \
				SC[16] = tmp1; \
				tmp1 = SC[17] + tmp2; \
				SC[17] = tmp2; \
				tmp2 = SC[18] + tmp1; \
				SC[18] = tmp1; \
				tmp1 = SC[19] + tmp2; \
				SC[19] = tmp2; \
				tmp2 = SC[20] + tmp1; \
				SC[20] = tmp1; \
				tmp1 = SC[21] + tmp2; \
				SC[21] = tmp2; \
				tmp2 = SC[22] + tmp1; \
				SC[22] = tmp1; \
				if (j >= offset * 2) \
					if (out_row) out_row[j - offset * 2] = (((uint64_t)1 <<47) + SC[23] + tmp2) >> 48; \
				SC[23] = tmp2; \
				SC += 24;\
			} \
		} \
	} \
	delete [] SC0; \
}
#define DO_FEATHER_27(type, int_type, max, N); \
{ \
	int offset = N / 2; \
        int frame_w = input->get_w(); \
        int frame_h = input->get_h(); \
        int start_in = start_out - offset; \
        int end_in = end_out + offset; \
        type **in_rows = (type**)input->get_rows(); \
        type **out_rows = (type**)output->get_rows(); \
        int i,j; \
	int_type tmp1, tmp2; \
	int_type SR0, SR1, SR2, SR3, SR4, SR5, SR6, SR7, SR8, SR9, SR10, SR11, SR12, SR13, SR14, SR15, SR16, SR17, SR18, SR19, SR20, SR21, SR22, SR23, SR24, SR25; \
	int_type *SC0 = new int_type[(frame_w + offset * 2) * 26]; \
	memset(SC0, 0, sizeof(int_type) * (frame_w  + offset * 2) * 26); \
	for (i = start_in; i < end_in; i++) \
	{ \
		type *in_row; \
		if (i < 0) \
	           	in_row = in_rows[0]; \
		else if (i >= frame_h) \
			in_row = in_rows[frame_h - 1]; \
		else \
			in_row = in_rows[i]; \
		SR0 = SR1 = SR2 = SR3 = SR4 = SR5 = SR6 = SR7 = SR8 = SR9 = SR10 = SR11 = SR12 = SR13 = SR14 = SR15 = SR16 = SR17 = SR18 = SR19 = SR20 = SR21 = SR22 = SR23 = SR24 = SR25 = 0; \
              { \
                        type *out_row; \
                        if (i >= start_out + offset && i<= end_out + offset) \
                        	out_row = out_rows[i - offset]; \
                        else \
                        	out_row = 0; \
			int_type *SC = SC0; \
			for (j = 0; j < frame_w + offset * 2; j++) \
                        { \
                        	if (j < offset) \
                           		tmp1 = in_row[0]; \
                           	else if (j >= frame_w + offset) \
                           		tmp1 = in_row[frame_w - 1]; \
                           	else \
                           		tmp1 = in_row[j - offset]; \
				tmp2 = SR0 + tmp1; \
				SR0 = tmp1; \
				tmp1 = SR1 + tmp2; \
				SR1 = tmp2; \
				tmp2 = SR2 + tmp1; \
				SR2 = tmp1; \
				tmp1 = SR3 + tmp2; \
				SR3 = tmp2; \
				tmp2 = SR4 + tmp1; \
				SR4 = tmp1; \
				tmp1 = SR5 + tmp2; \
				SR5 = tmp2; \
				tmp2 = SR6 + tmp1; \
				SR6 = tmp1; \
				tmp1 = SR7 + tmp2; \
				SR7 = tmp2; \
				tmp2 = SR8 + tmp1; \
				SR8 = tmp1; \
				tmp1 = SR9 + tmp2; \
				SR9 = tmp2; \
				tmp2 = SR10 + tmp1; \
				SR10 = tmp1; \
				tmp1 = SR11 + tmp2; \
				SR11 = tmp2; \
				tmp2 = SR12 + tmp1; \
				SR12 = tmp1; \
				tmp1 = SR13 + tmp2; \
				SR13 = tmp2; \
				tmp2 = SR14 + tmp1; \
				SR14 = tmp1; \
				tmp1 = SR15 + tmp2; \
				SR15 = tmp2; \
				tmp2 = SR16 + tmp1; \
				SR16 = tmp1; \
				tmp1 = SR17 + tmp2; \
				SR17 = tmp2; \
				tmp2 = SR18 + tmp1; \
				SR18 = tmp1; \
				tmp1 = SR19 + tmp2; \
				SR19 = tmp2; \
				tmp2 = SR20 + tmp1; \
				SR20 = tmp1; \
				tmp1 = SR21 + tmp2; \
				SR21 = tmp2; \
				tmp2 = SR22 + tmp1; \
				SR22 = tmp1; \
				tmp1 = SR23 + tmp2; \
				SR23 = tmp2; \
				tmp2 = SR24 + tmp1; \
				SR24 = tmp1; \
				tmp1 = SR25 + tmp2; \
				SR25 = tmp2; \
				tmp2 = SC[0] + tmp1; \
				SC[0] = tmp1; \
				tmp1 = SC[1] + tmp2; \
				SC[1] = tmp2; \
				tmp2 = SC[2] + tmp1; \
				SC[2] = tmp1; \
				tmp1 = SC[3] + tmp2; \
				SC[3] = tmp2; \
				tmp2 = SC[4] + tmp1; \
				SC[4] = tmp1; \
				tmp1 = SC[5] + tmp2; \
				SC[5] = tmp2; \
				tmp2 = SC[6] + tmp1; \
				SC[6] = tmp1; \
				tmp1 = SC[7] + tmp2; \
				SC[7] = tmp2; \
				tmp2 = SC[8] + tmp1; \
				SC[8] = tmp1; \
				tmp1 = SC[9] + tmp2; \
				SC[9] = tmp2; \
				tmp2 = SC[10] + tmp1; \
				SC[10] = tmp1; \
				tmp1 = SC[11] + tmp2; \
				SC[11] = tmp2; \
				tmp2 = SC[12] + tmp1; \
				SC[12] = tmp1; \
				tmp1 = SC[13] + tmp2; \
				SC[13] = tmp2; \
				tmp2 = SC[14] + tmp1; \
				SC[14] = tmp1; \
				tmp1 = SC[15] + tmp2; \
				SC[15] = tmp2; \
				tmp2 = SC[16] + tmp1; \
				SC[16] = tmp1; \
				tmp1 = SC[17] + tmp2; \
				SC[17] = tmp2; \
				tmp2 = SC[18] + tmp1; \
				SC[18] = tmp1; \
				tmp1 = SC[19] + tmp2; \
				SC[19] = tmp2; \
				tmp2 = SC[20] + tmp1; \
				SC[20] = tmp1; \
				tmp1 = SC[21] + tmp2; \
				SC[21] = tmp2; \
				tmp2 = SC[22] + tmp1; \
				SC[22] = tmp1; \
				tmp1 = SC[23] + tmp2; \
				SC[23] = tmp2; \
				tmp2 = SC[24] + tmp1; \
				SC[24] = tmp1; \
				if (j >= offset * 2) \
					if (out_row) out_row[j - offset * 2] = (((uint64_t)1 <<51) + SC[25] + tmp2) >> 52; \
				SC[25] = tmp2; \
				SC += 26;\
			} \
		} \
	} \
	delete [] SC0; \
}
#define DO_FEATHER_29(type, int_type, max, N); \
{ \
	int offset = N / 2; \
        int frame_w = input->get_w(); \
        int frame_h = input->get_h(); \
        int start_in = start_out - offset; \
        int end_in = end_out + offset; \
        type **in_rows = (type**)input->get_rows(); \
        type **out_rows = (type**)output->get_rows(); \
        int i,j; \
	int_type tmp1, tmp2; \
	int_type SR0, SR1, SR2, SR3, SR4, SR5, SR6, SR7, SR8, SR9, SR10, SR11, SR12, SR13, SR14, SR15, SR16, SR17, SR18, SR19, SR20, SR21, SR22, SR23, SR24, SR25, SR26, SR27; \
	int_type *SC0 = new int_type[(frame_w + offset * 2) * 28]; \
	memset(SC0, 0, sizeof(int_type) * (frame_w  + offset * 2) * 28); \
	for (i = start_in; i < end_in; i++) \
	{ \
		type *in_row; \
		if (i < 0) \
	           	in_row = in_rows[0]; \
		else if (i >= frame_h) \
			in_row = in_rows[frame_h - 1]; \
		else \
			in_row = in_rows[i]; \
		SR0 = SR1 = SR2 = SR3 = SR4 = SR5 = SR6 = SR7 = SR8 = SR9 = SR10 = SR11 = SR12 = SR13 = SR14 = SR15 = SR16 = SR17 = SR18 = SR19 = SR20 = SR21 = SR22 = SR23 = SR24 = SR25 = SR26 = SR27 = 0; \
              { \
                        type *out_row; \
                        if (i >= start_out + offset && i<= end_out + offset) \
                        	out_row = out_rows[i - offset]; \
                        else \
                        	out_row = 0; \
			int_type *SC = SC0; \
			for (j = 0; j < frame_w + offset * 2; j++) \
                        { \
                        	if (j < offset) \
                           		tmp1 = in_row[0]; \
                           	else if (j >= frame_w + offset) \
                           		tmp1 = in_row[frame_w - 1]; \
                           	else \
                           		tmp1 = in_row[j - offset]; \
				tmp2 = SR0 + tmp1; \
				SR0 = tmp1; \
				tmp1 = SR1 + tmp2; \
				SR1 = tmp2; \
				tmp2 = SR2 + tmp1; \
				SR2 = tmp1; \
				tmp1 = SR3 + tmp2; \
				SR3 = tmp2; \
				tmp2 = SR4 + tmp1; \
				SR4 = tmp1; \
				tmp1 = SR5 + tmp2; \
				SR5 = tmp2; \
				tmp2 = SR6 + tmp1; \
				SR6 = tmp1; \
				tmp1 = SR7 + tmp2; \
				SR7 = tmp2; \
				tmp2 = SR8 + tmp1; \
				SR8 = tmp1; \
				tmp1 = SR9 + tmp2; \
				SR9 = tmp2; \
				tmp2 = SR10 + tmp1; \
				SR10 = tmp1; \
				tmp1 = SR11 + tmp2; \
				SR11 = tmp2; \
				tmp2 = SR12 + tmp1; \
				SR12 = tmp1; \
				tmp1 = SR13 + tmp2; \
				SR13 = tmp2; \
				tmp2 = SR14 + tmp1; \
				SR14 = tmp1; \
				tmp1 = SR15 + tmp2; \
				SR15 = tmp2; \
				tmp2 = SR16 + tmp1; \
				SR16 = tmp1; \
				tmp1 = SR17 + tmp2; \
				SR17 = tmp2; \
				tmp2 = SR18 + tmp1; \
				SR18 = tmp1; \
				tmp1 = SR19 + tmp2; \
				SR19 = tmp2; \
				tmp2 = SR20 + tmp1; \
				SR20 = tmp1; \
				tmp1 = SR21 + tmp2; \
				SR21 = tmp2; \
				tmp2 = SR22 + tmp1; \
				SR22 = tmp1; \
				tmp1 = SR23 + tmp2; \
				SR23 = tmp2; \
				tmp2 = SR24 + tmp1; \
				SR24 = tmp1; \
				tmp1 = SR25 + tmp2; \
				SR25 = tmp2; \
				tmp2 = SR26 + tmp1; \
				SR26 = tmp1; \
				tmp1 = SR27 + tmp2; \
				SR27 = tmp2; \
				tmp2 = SC[0] + tmp1; \
				SC[0] = tmp1; \
				tmp1 = SC[1] + tmp2; \
				SC[1] = tmp2; \
				tmp2 = SC[2] + tmp1; \
				SC[2] = tmp1; \
				tmp1 = SC[3] + tmp2; \
				SC[3] = tmp2; \
				tmp2 = SC[4] + tmp1; \
				SC[4] = tmp1; \
				tmp1 = SC[5] + tmp2; \
				SC[5] = tmp2; \
				tmp2 = SC[6] + tmp1; \
				SC[6] = tmp1; \
				tmp1 = SC[7] + tmp2; \
				SC[7] = tmp2; \
				tmp2 = SC[8] + tmp1; \
				SC[8] = tmp1; \
				tmp1 = SC[9] + tmp2; \
				SC[9] = tmp2; \
				tmp2 = SC[10] + tmp1; \
				SC[10] = tmp1; \
				tmp1 = SC[11] + tmp2; \
				SC[11] = tmp2; \
				tmp2 = SC[12] + tmp1; \
				SC[12] = tmp1; \
				tmp1 = SC[13] + tmp2; \
				SC[13] = tmp2; \
				tmp2 = SC[14] + tmp1; \
				SC[14] = tmp1; \
				tmp1 = SC[15] + tmp2; \
				SC[15] = tmp2; \
				tmp2 = SC[16] + tmp1; \
				SC[16] = tmp1; \
				tmp1 = SC[17] + tmp2; \
				SC[17] = tmp2; \
				tmp2 = SC[18] + tmp1; \
				SC[18] = tmp1; \
				tmp1 = SC[19] + tmp2; \
				SC[19] = tmp2; \
				tmp2 = SC[20] + tmp1; \
				SC[20] = tmp1; \
				tmp1 = SC[21] + tmp2; \
				SC[21] = tmp2; \
				tmp2 = SC[22] + tmp1; \
				SC[22] = tmp1; \
				tmp1 = SC[23] + tmp2; \
				SC[23] = tmp2; \
				tmp2 = SC[24] + tmp1; \
				SC[24] = tmp1; \
				tmp1 = SC[25] + tmp2; \
				SC[25] = tmp2; \
				tmp2 = SC[26] + tmp1; \
				SC[26] = tmp1; \
				if (j >= offset * 2) \
					if (out_row) out_row[j - offset * 2] = (((uint64_t)1 <<55) + SC[27] + tmp2) >> 56; \
				SC[27] = tmp2; \
				SC += 28;\
			} \
		} \
	} \
	delete [] SC0; \
}

/* THIS WORKS ONLY FOR ODD N >= 3 */
#define DO_FEATHER_N(type, int_type, max, N) \
{ \
	switch(input->get_color_model()) \
        { \
	        case BC_A8: \
			switch (N) \
			{ \
				case 3: \
					DO_FEATHER_3(unsigned char, uint16_t, max, 3); \
					return 1; \
					break; \
				case 5: \
					DO_FEATHER_5(unsigned char, uint16_t, max, 5); \
					return 1; \
					break; \
				case 7: \
					DO_FEATHER_7(unsigned char, uint32_t, max, 7); \
					return 1; \
					break; \
				case 9: \
					DO_FEATHER_9(unsigned char, uint32_t, max, 9); \
					return 1; \
					break; \
				case 11: \
					DO_FEATHER_11(unsigned char, uint32_t, max, 11); \
					return 1; \
					break; \
				case 13: \
					DO_FEATHER_13(unsigned char, uint32_t, max, 13); \
					return 1; \
					break; \
				case 15: \
					DO_FEATHER_15(unsigned char, uint64_t, max, 15); \
					return 1; \
					break; \
				case 17: \
					DO_FEATHER_17(unsigned char, uint64_t, max, 17); \
					return 1; \
					break; \
				case 19: \
					DO_FEATHER_19(unsigned char, uint64_t, max, 19); \
					return 1; \
					break; \
				case 21: \
					DO_FEATHER_21(unsigned char, uint64_t, max, 21); \
					return 1; \
					break; \
				case 23: \
					DO_FEATHER_23(unsigned char, uint64_t, max, 23); \
					return 1; \
					break; \
				case 25: \
					DO_FEATHER_25(unsigned char, uint64_t, max, 25); \
					return 1; \
					break; \
				case 27: \
					DO_FEATHER_27(unsigned char, uint64_t, max, 27); \
					return 1; \
					break; \
				case 29: \
					DO_FEATHER_29(unsigned char, uint64_t, max, 29); \
					return 1; \
					break; \
				default: \
					return 0; \
			} \
	        case BC_A16: \
			switch (N) \
			{ \
				case 3: \
					DO_FEATHER_3(uint16_t, uint32_t, max, 3); \
					return 1; \
					break; \
				case 5: \
					DO_FEATHER_5(uint16_t, uint32_t, max, 5); \
					return 1; \
					break; \
				case 7: \
					DO_FEATHER_7(uint16_t, uint32_t, max, 7); \
					return 1; \
					break; \
				case 9: \
					DO_FEATHER_9(uint16_t, uint32_t, max, 9); \
					return 1; \
					break; \
				case 11: \
					DO_FEATHER_11(uint16_t, uint64_t, max, 11); \
					return 1; \
					break; \
				case 13: \
					DO_FEATHER_13(uint16_t, uint64_t, max, 13); \
					return 1; \
					break; \
				case 15: \
					DO_FEATHER_15(uint16_t, uint64_t, max, 15); \
					return 1; \
					break; \
				case 17: \
					DO_FEATHER_17(uint16_t, uint64_t, max, 17); \
					return 1; \
					break; \
				case 19: \
					DO_FEATHER_19(uint16_t, uint64_t, max, 19); \
					return 1; \
					break; \
				case 21: \
					DO_FEATHER_21(uint16_t, uint64_t, max, 21); \
					return 1; \
					break; \
				case 23: \
					DO_FEATHER_23(uint16_t, uint64_t, max, 23); \
					return 1; \
					break; \
				case 25: \
					DO_FEATHER_25(uint16_t, uint64_t, max, 25); \
					return 1; \
					break; \
				default: \
					return 0; \
			} \
	} \
	return 0; \
} 
