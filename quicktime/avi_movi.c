#include "funcprotos.h"
#include "quicktime.h"

void quicktime_delete_movi(quicktime_t *file, quicktime_movi_t *movi)
{
	int i;
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		if(movi->ix[i]) quicktime_delete_ix(movi->ix[i]);
	}
}

void quicktime_init_movi(quicktime_t *file, quicktime_riff_t *riff)
{
	int i;
	quicktime_riff_t *first_riff = file->riff[0];
	quicktime_movi_t *movi = &riff->movi;

	quicktime_atom_write_header(file, &movi->atom, "LIST");
	quicktime_write_char32(file, "movi");

// Initialize partial indexes and relative positions for ix entries
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		quicktime_strl_t *strl = first_riff->hdrl.strl[i];
		quicktime_trak_t *trak = file->moov.trak[i];
		quicktime_ix_t *ix = 
			movi->ix[i] = 
			quicktime_new_ix(file, trak, strl);
	}
}

void quicktime_read_movi(quicktime_t *file, 
	quicktime_atom_t *parent_atom,
	quicktime_movi_t *movi)
{
	movi->atom.size = parent_atom->size;
// Relative to start of the movi string
	movi->atom.start = parent_atom->start + 8;
	quicktime_atom_skip(file, parent_atom);
}

void quicktime_finalize_movi(quicktime_t *file, quicktime_movi_t *movi)
{
	int i;
// Pad movi to get an even number of bytes
	char temp[2] = { 0, 0 };
	quicktime_write_data(file, 
		temp, 
		(quicktime_position(file) - movi->atom.start) % 2);

	for(i = 0; i < file->moov.total_tracks; i++)
	{
		quicktime_ix_t *ix = movi->ix[i];
// Write partial indexes and update super index
		quicktime_write_ix(file, ix, i);
	}

	quicktime_atom_write_footer(file, &movi->atom);
}






