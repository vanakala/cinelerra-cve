#include "funcprotos.h"
#include "quicktime.h"

void quicktime_read_movi(quicktime_t *file, quicktime_atom_t *parent_atom)
{
	file->mdat.atom.size = parent_atom->size;
// Relative to start of the movi string
	file->mdat.atom.start = parent_atom->start + 8;
	quicktime_atom_skip(file, parent_atom);
}

void quicktime_write_movi(quicktime_t *file)
{
	quicktime_atom_write_header(file, &file->mdat.atom, "LIST");
	quicktime_write_char32(file, "movi");
}



