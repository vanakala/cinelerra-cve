
typedef struct _Vector *Vector;

Vector NewVector( size_t recsize );
void *VectorAppend( Vector v, void *rec );
void VectorRewind( Vector v );
void *VectorNext( Vector v );
void *VectorLookAhead( Vector v, int lookahead );



