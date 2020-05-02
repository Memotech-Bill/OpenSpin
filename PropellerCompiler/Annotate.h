/* Annotate.h - Collect data for annotated binary listing */

#ifndef H_ANNOTATE
#define H_ANNOTATE

// Annotation types
enum AL_Type {atSpinCode, atSpinObj, atSpinPub, atSpinPri, atSpinPar, atSpinRes, atSpinLoc,
              atDAT, atByte, atWord, atLong, atPASM};

// Request collection of annotation data
void AL_Request (void);
// Enable annotated listing
void AL_Enable (bool bEnable);
// Open collection of data for an object
void AL_OpenObject (const char *psFile, const char *psPath, const char *pSource);
// Save Code Symbols
void AL_Symbol (AL_Type at, int posn, const char *psSymbol, int addr, int caddr = -1);
// Associate a code line with an address
void AL_AddLine (AL_Type at, int posn, int addr, int caddr = -1);
// Set a data type for a given address
void AL_SetType (AL_Type at, int addr = -1);
// Add current object to heap
void AL_AddToHeap (int nHeap);
// Restore object from heap
void AL_CopyFromHeap (int nHeap, int nIndex);
// Include object
void AL_Include (int nIndex, int nOffset, int nSize);
// Define a variable
void AL_Variable (const char *psName, int nSize, int nCount);
// Define a Sub-Object
void AL_SubObject (int posn, const char *psName, const char *psObject, int nCount);
// Output the annotated listing
void AL_Output (const unsigned char *pBinary, int nSize, const struct CompilerData *pcd, bool bPrep);

#endif
