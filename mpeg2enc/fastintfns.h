/* fast int primitives. min,max,abs,samesign
 *
 * WARNING: Assumes 2's complement arithmetic.
 *
 */

#define fabsshift ((8*sizeof(unsigned int))-1)
#ifdef P6_CPU
static __inline__ int intmax( register int x, register int y )
{
	asm( "cmpl %1, %0\n"
	     "cmovl %1, %0\n"
         : "+r" (x) :  "r" (y)
       );
	return x;
}

static __inline__ int intmin( register int x, register int y )
{
	asm( "cmpl %1, %0\n"
	     "cmovg %1, %0\n"
         : "+r" (x) :  "rm" (y)
       );
	return x;
}

static __inline__ int intabs( register int x )
{
	register int neg = -x;
	asm( "cmpl %1, %0\n"
	     "cmovl %1, %0\n"
         : "+r" (x) :  "r" (neg)
       );
	return x;
}
#else

static __inline__ int intabs(int x)
{
	return ((x)-(((unsigned int)(x))>>fabsshift)) ^ ((x)>>fabsshift);
}

static __inline__ int intmax(int x, int y)
{
	return (((x-y)>>fabsshift) & y) |  ((~((x-y)>>fabsshift)) & x);
}

static __inline__ int intmin(int x,int y)
{
	return (((y-x)>>fabsshift) & y) |  ((~((y-x)>>fabsshift)) & x);
}

#endif

#define signmask(x) (((int)x)>>fabsshift)
static __inline__ int intsamesign(int x, int y)
{
	return (y+(signmask(x) & -(y<<1)));
}
#undef signmask
#undef fabsshift

