#ifndef HUFFMAN_H
#define HUFFMAN_H

/*
 * huffman tables ... recalcualted to work with my optimzed
 * decoder scheme (MH)
 * 
 * probably we could save a few bytes of memory, because the 
 * smaller tables are often the part of a bigger table
 */

struct newhuff 
{
  unsigned int linbits;
  short *table;
};

extern short mpeg3_tab0[1];

extern short mpeg3_tab1[7];

extern short mpeg3_tab2[17];

extern short mpeg3_tab3[17];

extern short mpeg3_tab5[31];

extern short mpeg3_tab6[31];

extern short mpeg3_tab7[71];

extern short mpeg3_tab8[71];

extern short mpeg3_tab9[71];

extern short mpeg3_tab10[127];

extern short mpeg3_tab11[127];

extern short mpeg3_tab12[127];

extern short mpeg3_tab13[511];

extern short mpeg3_tab15[511];

extern short mpeg3_tab16[511];

extern short mpeg3_tab24[511];

extern short mpeg3_tab_c0[31];

extern short mpeg3_tab_c1[31];



extern struct newhuff mpeg3_ht[32];

extern struct newhuff mpeg3_htc[2];


#endif
