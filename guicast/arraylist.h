#ifndef ARRAYLIST_H
#define ARRAYLIST_H

// designed for lists of track numbers

template<class TYPE>
class ArrayList
{
public:
	ArrayList();
	virtual ~ArrayList();

	TYPE append(TYPE value);
	TYPE append();
	TYPE insert(TYPE value, int number);

// allocate
	void allocate(int total);
// remove last pointer from end
	void remove();          
// remove last pointer and object from end
	void remove_object();          
// remove pointer to object from list
	void remove(TYPE value);       
// remove object and pointer to it from list
	void remove_object(TYPE value);     
// remove object and pointer to it from list
	void remove_object_number(int number);     
// remove pointer to item numbered
	void remove_number(int number);
// Return number of first object matching argument
	int number_of(TYPE object);
	void remove_all();
// Remove pointer and objects for each array entry
	void remove_all_objects();
	TYPE last();
	void set_array_delete();

	void sort();

	TYPE* values;
	int total;

private:
	int available;
	int array_delete;
};

template<class TYPE>
ArrayList<TYPE>::ArrayList()
{
	total = 0;
	available = 1;
	array_delete = 0;
	values = new TYPE[available];
}


template<class TYPE>
ArrayList<TYPE>::~ArrayList()
{
// Just remove the pointer
	delete [] values;
}

template<class TYPE>
void ArrayList<TYPE>::set_array_delete()
{
   array_delete = 1;
}


template<class TYPE>
void ArrayList<TYPE>::allocate(int total)
{
	if(total > available)
	{
		available = total;
		TYPE* newvalues = new TYPE[available];
		for(int i = 0; i < total; i++) newvalues[i] = values[i];
		delete [] values;
		values = newvalues;
	}
}

template<class TYPE>
TYPE ArrayList<TYPE>::append(TYPE value)            // add to end of list
{
	if(total + 1 > available) 
	{
		available *= 2;
		TYPE* newvalues = new TYPE[available];
		for(int i = 0; i < total; i++) newvalues[i] = values[i];
		delete [] values;
		values = newvalues;
	}
	
	values[total++] = value;
	return value;
}

template<class TYPE>
TYPE ArrayList<TYPE>::append()            // add to end of list
{
	if(total + 1 > available)
	{
		available *= 2;
		TYPE* newvalues = new TYPE[available];
		for(int i = 0; i < total; i++) newvalues[i] = values[i];
		delete [] values;
		values = newvalues;
	}
	total++;

	return values[total - 1];
}

template<class TYPE>
TYPE ArrayList<TYPE>::insert(TYPE value, int number)
{
	append(0);
	for(int i = total - 1; i > number; i--)
	{
		values[i] = values[i - 1];
	}
	values[number] = value;
}

template<class TYPE>
void ArrayList<TYPE>::remove(TYPE value)                   // remove value from anywhere in list
{
	int in, out;

	for(in = 0, out = 0; in < total;)
	{
		if(values[in] != value) values[out++] = values[in++];
		else 
		{
			in++; 
		}
	}
	total = out;
}

template<class TYPE>
TYPE ArrayList<TYPE>::last()                   // last element in list
{
	return values[total - 1];
}



template<class TYPE>
void ArrayList<TYPE>::remove_object(TYPE value)                   // remove value from anywhere in list
{
	remove(value);
	if (array_delete) 
		delete [] value;
	else 
		delete value;
}

template<class TYPE>
void ArrayList<TYPE>::remove_object_number(int number)
{
	if(number < total)
	{
		if (array_delete) 
			delete [] values[number];
		else
			delete values[number];
		remove_number(number);
	}
	else
		fprintf(stderr, "ArrayList<TYPE>::remove_object_number: number %d out of range %s.\n", number, total);
}


template<class TYPE>
void ArrayList<TYPE>::remove_object()                   // remove value from anywhere in list
{
	if(total)
	{
		if (array_delete) 
			delete [] values[total - 1];
		else 
			delete values[total - 1];
		remove();
	}
	else
		fprintf(stderr, "ArrayList<TYPE>::remove_object: array is 0 length.\n");
}



template<class TYPE>
void ArrayList<TYPE>::remove()
{
	total--;
}

// remove pointer from anywhere in list
template<class TYPE>
void ArrayList<TYPE>::remove_number(int number)                   
{
	int in, out;
	for(in = 0, out = 0; in < total;)
	{
		if(in != number)
			values[out++] = values[in++];
		else
// need to delete it here
			in++;       
	}
	total = out;
}

template<class TYPE>
void ArrayList<TYPE>::remove_all_objects()
{
//printf("ArrayList<TYPE>::remove_all_objects 1 %d\n", total);
	for(int i = 0; i < total; i++)
	{
		if(array_delete)
			delete [] values[i];
		else
			delete values[i];
	}
	total = 0;
}

template<class TYPE>
void ArrayList<TYPE>::remove_all()
{
	total = 0;
}

// sort from least to greatest value
template<class TYPE>
void ArrayList<TYPE>::sort()
{
	int result = 1;
	TYPE temp;

	while(result)
	{
		result = 0;
		for(int i = 0, j = 1; j < total; i++, j++)
		{
			if(values[j] < values[i])
			{
				temp = values[i];
				values[i] = values[j];
				values[j] = temp;
				result = 1;
			}
		}
	}
}

template<class TYPE>
int ArrayList<TYPE>::number_of(TYPE object)
{
	for(int i = 0; i < total; i++)
	{
		if(values[i] == object) return i;
	}
	return 0;
}


#endif
