/* Annotate.h - Collect data for annotated binary listing */

#ifndef H_ANNOTATE
#define H_ANNOTATE

// Destinations for listing
enum AL_Mode {amNone, amConsole, amOutput, amFile};

// Annotation types
enum AL_Type {atSpinCode, atSpinObj, atDAT, atByte, atWord, atLong, atPASM};

// Request collection of annotation data
void AL_Request (AL_Mode, const char *psFile);
// Enable annotated listing
void AL_Enable (bool bEnable);
// Open collection of data for an object
void AL_OpenObject (const char *psFile, const struct CompilerData *pcd, struct preprocess *preproc);
// Save Routine entry locations
void AL_Routine (int posn);
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
// Enter an Object Distillation Record
void AL_Distill (int start, int length, int reloc);
// Output the annotated listing
void AL_Output (const unsigned char *pBinary, int nSize, const struct CompilerData *pcd);

#endif
