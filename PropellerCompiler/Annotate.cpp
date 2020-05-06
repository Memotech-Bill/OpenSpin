/* Annotate.h - Collect data for annotated binary listing */

#include "Annotate.h"
#include "PropellerCompiler.h"
#include "CompileSpin.h"
#include "textconvert.h"
#include "preprocess.h"

#include <stdio.h>
#include <string.h>
#include <vector>
#include <map>
#include <string>
#include <algorithm>

// Forward definition:
class AL_Object;

// Detail a line of source code

class AL_SourceLine
    {
public:
    AL_SourceLine(AL_Type at = atSpinCode, int addr = 0, int caddr = -1);   // Constructor
    ~AL_SourceLine () {}                                                    // Destructor
    void SetType (AL_Type at, int addr = -1);                               // Change data type
    AL_Type Type (int index = 0) const { return m_types.at (index); }       // Get base type
    int Addr (void) const { return m_addr; }                                // Get binary address
    int CogAddr (void) const { return m_caddr; }                            // Get cog address
    int NumTypes (void) const { return m_types.size (); }                   // Number of types
    void Changes (std::vector<int> &offsets) const;                         // Changes of type
private:
    std::map<int, AL_Type> m_types;
    int m_addr;
    int m_caddr;
    };

// Class for annotation details for a given address
class AL_Point
    {
public:
    AL_Point (const AL_Object *pobj = NULL) { m_pobj = pobj; }  // Construction
    virtual ~AL_Point () {}                                     // Destructor
    const AL_Object *Object (void) { return m_pobj; }           // Get source object
    void AddLine (const AL_Object *obj, int posn);              // Add a source line location
    int Sort (void);                                            // Sort source lines and return count
    int GetLine (int index) const;                              // Get location of source line

private:
    const AL_Object *m_pobj;                                    // Source object
    std::vector<int> m_lines;                                   // Source lines related to this address
    };

// Details of object references
struct AL_ObjRef
    {
    AL_ObjRef (void) { m_name = ""; m_pobj = NULL; m_count = 0; }   // Default constructor
    AL_ObjRef (const char *psName, const AL_Object *pobj, int nCount)
        { m_name = psName; m_pobj = pobj; m_count = nCount; }       // Constructor
    ~AL_ObjRef () {}                                                // Destructor
    std::string m_name;                                             // Name of object variable
    const AL_Object *m_pobj;                                        // Pointer to object details
    int m_count;                                                    // Size of array
    };

// Object distillation records
struct AL_DistRec
    {
    AL_DistRec (int start = 0, int end = 0, int reloc = 0)
        { m_start = start; m_end = end; m_reloc = reloc; }    // Constructor
    int m_start;
    int m_end;
    int m_reloc;
    };

// Class to collect details of an Object
class AL_Object
    {
public:
    AL_Object (const char *psFile, const char *psPath);             // Constructor
    ~AL_Object () {}                                                // Destructor
    const std::string *File (void) const { return &m_sFile; }       // Get source file name
    const std::string *Path (void) const { return &m_sPath; }       // Get path to source file
    void *PreProc (void) const { return m_ppstate; }                // Pre-processor define state
    void Restart (void);                                            // Restart compilation
    void AddLine (AL_Type at, int posn, int addr, int caddr);       // Add location of a source line
    void SetType (AL_Type, int addr);                               // Set a type change for last source line
    bool GetOnHeap (void) const { return m_bOnHeap; }               // Return OnHeap flag
    void SetOnHeap (bool bOnHeap) { m_bOnHeap = bOnHeap; }          // Set OnHeap flag
    void Include (AL_Object *pobj, int nOffset);                    // Include a sub-object from heap
    void AddRoutine (int posn) { m_routines.push_back (posn); }     // Add a routine definition to the object
    void AddVariable (const char *psName, int nSize, int nCount);   // Add a global variable
    void AddSubObject (int posn, const char *psName,
        const AL_Object *, int nCount);                             // Add a sub-object reference
    void AddDistill (int start, int length, int reloc);             // Add a distillation record
    int DistillAddr (int addr);                                     // Translate distilled address
    void PlaceLines (const AL_Object *pobj, int nOffset);           // Place source lines at addresses
    void ListVariables (const std::string *psName, int &addr) const;// List variable locations
    void Output (const unsigned char *pBinary, int nSize,
        const struct CompilerData *pcd);                            // Output listing
    
private:
    std::string m_sFile;                                            // Source file of object
    std::string m_sPath;                                            // Path of source file
    int m_posnLast;                                                 // Position of last source line
    void *m_ppstate;                                                // Preprocessor state
    std::map<int, AL_SourceLine> m_lines;                           // Source lines for the object
    std::map<int, AL_Point> m_points;                               // Address information for object
    bool m_bOnHeap;                                                 // True if object is on heap
    std::vector<std::pair<AL_Object *, int> > m_include;            // Objects included from heap
    std::vector<int> m_routines;                                    // Source locations for routines
    std::vector<std::pair<std::string, int> > m_variables[3];       // Add global variables
    std::map<int, AL_ObjRef> m_subobjs;                             // Sub-object references
    std::vector<AL_DistRec> m_distill;                              // Object distillation records
    bool m_bFixup;                                                  // True if distillation fixup required
    };

// Global data:
static bool g_bSelected = false;                    // Annotated output requested
static bool g_bEnable = false;                      // Collection of data enabled
static AL_Object * g_alobj = NULL;                  // Current object
static const char *g_pSource = NULL;                // Pointer to source in compiler
static std::map<int, AL_Object *> g_alheap;         // Object heap
static std::map<int, AL_Object *> g_objs;           // Objects on obj_data
static const AL_Object *g_pobjFile = NULL;          // Object of currently loaded source file
static char *g_pFileSrc = NULL;                     // Source code loaded from file
static struct preprocess *g_preprocessor = NULL;    // File preprocessor data
static std::string g_sListFile;                     // Name of list file (or "-" for stdout)
static FILE *g_pfilList = NULL;                     // List output file stream

// Global functions:
// Request collection of annotation data
void AL_Request (AL_Mode mode, const char *psName)
    {
    const char *psExt;
    switch (mode)
        {
        case amConsole:
            g_sListFile = "-";
            g_bSelected = true;
            break;
        case amOutput:
            psExt = strrchr (psName, '.');
            if ( psExt ) g_sListFile = std::string (psName, psExt - psName);
            else g_sListFile = psName;
            g_sListFile += ".lst";
            g_bSelected = true;
            break;
        case amFile:
            g_sListFile = psName;
            g_bSelected = true;
            break;
        default:
            g_bSelected = false;
            break;
        }
    }

// Enable collection of annotation data
void AL_Enable (bool bEnable)
    {
    if (bEnable) g_bEnable = g_bSelected;
    else g_bEnable = false;
    }

// Open annotation data for a given object
void AL_OpenObject (const char *psFile, const struct CompilerData *pcd, struct preprocess *preproc)
    {
    if ( g_bEnable )
        {
        g_preprocessor = preproc;
        if (( g_alobj == NULL ) || ( *g_alobj->File () != psFile ))
            {
            if ( ( g_alobj != NULL ) && ( ! g_alobj->GetOnHeap () ) ) delete g_alobj;
            g_alobj = new AL_Object (psFile, pcd->current_file_path);
            }
        g_pSource = pcd->source;
        }
    }

// Associate a code line with an address
void AL_AddLine (AL_Type at, int posn, int addr, int caddr)
    {
    if ( ( g_bEnable ) && ( g_alobj != NULL ) )
        {
        // Find beginning of line
        while (posn > 0)
            {
            if ( g_pSource[posn - 1] == '\r' ) break;
            --posn;
            }
        // Add it
        g_alobj->AddLine (at, posn, addr, caddr);
        }
    }

// Set a data type for a given address
void AL_SetType (AL_Type at, int addr)
    {
    if ( ( g_bEnable ) && ( g_alobj != NULL ) )
        {
        g_alobj->SetType (at, addr);
        }
    }

// Save Code Symbols
void AL_Routine (int posn)
    {
    if ( ( g_bEnable ) && ( g_alobj != NULL ) )
        {
        // Find beginning of line
        while (posn > 0)
            {
            if ( g_pSource[posn - 1] == '\r' ) break;
            --posn;
            }
        // Add it
        g_alobj->AddRoutine (posn);
        }
    }

// Add an Object Variable
void AL_Variable (const char *psName, int nSize, int nCount)
    {
    if ( ( g_bEnable ) && ( g_alobj != NULL ) )
        {
        g_alobj->AddVariable (psName, nSize, nCount);
        }
    }

// Add a Sub-Object reference
void AL_SubObject (int posn, const char *psName, const char *psObject, int nCount)
    {
    if ( ( g_bEnable ) && ( g_alobj != NULL ) )
        {
        std::string sFile = psObject;
        sFile += ".spin";
        AL_Object *pobj = NULL;
        std::map<int, AL_Object *>::iterator it;
        for (it = g_alheap.begin (); it != g_alheap.end (); ++it)
            {
            if ( *(it->second->File ()) == sFile )
                {
                pobj = it->second;
                break;
                }
            }
        if ( pobj != NULL ) g_alobj->AddSubObject (posn, psName, pobj, nCount);
        }
    }

// Add current object to heap
void AL_AddToHeap (int nHeap)
    {
    if ( g_bEnable )
        {
        g_alobj->SetOnHeap(true);
        g_alheap[nHeap] = g_alobj;
        }
    }

// Restore object from heap
void AL_CopyFromHeap (int nHeap, int nIndex)
    {
    if ( g_bEnable )
        {
        g_objs[nIndex] = g_alheap[nHeap];
        }
    }

// Include object
void AL_Include (int nIndex, int nOffset, int nSize)
    {
    if ( g_bEnable )
        {
        g_alobj->Include (g_objs[nIndex], nOffset);
        }
    }

// Define a distillation block
void AL_Distill (int start, int length, int reloc)
    {
    if ( g_bEnable )
        {
        g_alobj->AddDistill (start, start + length, reloc - start);
        }
    }

// Free all allocated memory
void AL_Reset (void)
    {
    if ( ( g_alobj != NULL ) && ( ! g_alobj->GetOnHeap () ) ) delete g_alobj;
    g_alobj = NULL;
    std::map<int, AL_Object *>::iterator it;
    for (it = g_alheap.begin (); it != g_alheap.end (); ++it)
        delete it->second;
    g_alheap.clear ();
    g_objs.clear ();
    g_bSelected = false;
    g_bEnable = false;
    g_pSource = NULL;
    g_pobjFile = NULL;
    if (g_pFileSrc != NULL) free (g_pFileSrc);
    g_pFileSrc = NULL;
    g_preprocessor = NULL;
    g_sListFile.clear ();
    g_pfilList = NULL;
    }

// Output the annotated listing
void AL_Output (const unsigned char *pBinary, int nSize, const struct CompilerData *pcd)
    {
    if ( g_bEnable )
        {
        if ( g_sListFile == "-" )
            {
            g_pfilList = stdout;
            g_alobj->Output (pBinary, nSize, pcd);
            }
        else
            {
            g_pfilList = fopen (g_sListFile.c_str (), "w");
            if ( g_pfilList != NULL )
                {
                g_alobj->Output (pBinary, nSize, pcd);
                fclose (g_pfilList);
                }
            }
        }
    AL_Reset ();
    }

// Load source file - Unfortunately GetPASCIISource has a fixed destination,
// so we have to duplicate the functionality here
static bool AL_LoadSource (const AL_Object *pobj)
    {
    if ( pobj == g_pobjFile ) return true;
    g_pobjFile = NULL;
    if ( g_pFileSrc != NULL )
        {
        free (g_pFileSrc);
        g_pFileSrc = NULL;
        }
    // read in file to temp buffer and convert to PASCII
    FILE *pfil = fopen (pobj->Path ()->c_str (), "r");
    if ( pfil == NULL ) return false;
    fseek (pfil, 0, SEEK_END);
    int nLength = ftell (pfil);
    fseek (pfil, 0, SEEK_SET);
    char *praw = (char *) malloc (nLength + 1);
    if ( praw == NULL )
        {
        fclose (pfil);
        return false;
        }
    int nRead = fread (praw, 1, nLength, pfil);
    fclose (pfil);
    if ( nRead != nLength )
        {
        free (praw);
        return false;
        }
    praw[nLength] = '\0';
    char *psrc = praw;
    if ( pobj->PreProc () )
        {
        memoryfile mfile;
        mfile.buffer = praw;
        mfile.length = nLength;
        mfile.readoffset = 0;
        pp_restore_define_state (g_preprocessor, pobj->PreProc ());
        pp_push_file_struct (g_preprocessor, &mfile, pobj->File ()->c_str ());
        pp_run (g_preprocessor);
        psrc = pp_finish (g_preprocessor);
        nLength = (int) strlen (psrc);
        if (nLength > 0)
            {
            free (praw);
            praw = NULL;
            }
        else
            {
            free (psrc);
            psrc = praw;
            }
        }
    bool bResult = false;
    g_pFileSrc = (char *) calloc (nLength + 1, 1);
    if ( g_pFileSrc != NULL )
        bResult = UnicodeToPASCII(psrc, nLength, g_pFileSrc, g_preprocessor != NULL);
    free (psrc);
    if (bResult)
        {
        g_pobjFile = pobj;
        fprintf (g_pfilList, "                       File \"%s\"\n", pobj->File ()->c_str ());
        }
    return bResult;
    }

// Print a line of source code
static void AL_Print (int iCh)
    {
    while ((g_pFileSrc[iCh] != '\r') && (g_pFileSrc[iCh] != '\0'))
        {
        fprintf (g_pfilList, "%c", g_pFileSrc[iCh]);
        ++iCh;
        }
    fprintf (g_pfilList, "\n");
    }

// Get a little-endian word from a byte array
static unsigned int AL_Word (const unsigned char *pdata)
    {
    unsigned int b0 = pdata[0];
    unsigned int b1 = pdata[1];
    return ( b1 << 8 ) | b0;
    }

// Get a little-endian long from a byte array
static unsigned int AL_Long (const unsigned char *pdata)
    {
    unsigned int b0 = pdata[0];
    unsigned int b1 = pdata[1];
    unsigned int b2 = pdata[2];
    unsigned int b3 = pdata[3];
    return ( b3 << 24 ) | ( b2 << 16 ) | ( b1 << 8 ) | b0;
    }

// Get a big-endian word from a byte array
static unsigned int AL_BWord (const unsigned char *pdata)
    {
    unsigned int b0 = pdata[0];
    unsigned int b1 = pdata[1];
    return ( b0 << 8 ) | b1;
    }

// Get a 3 byte big-endian value from a byte array
static unsigned int AL_BInt24 (const unsigned char *pdata)
    {
    unsigned int b0 = pdata[0];
    unsigned int b1 = pdata[1];
    unsigned int b2 = pdata[2];
    return ( b0 << 16 ) | ( b1 << 8 ) | b2;
    }

// Get a big-endian long from a byte array
static unsigned int AL_BLong (const unsigned char *pdata)
    {
    unsigned int b0 = pdata[0];
    unsigned int b1 = pdata[1];
    unsigned int b2 = pdata[2];
    unsigned int b3 = pdata[3];
    return ( b0 << 24 ) | ( b1 << 16 ) | ( b2 << 8 ) | b3;
    }

// Get an address from a byte array
unsigned int AL_Address (const unsigned char *pdata, bool &bWord)
    {
    unsigned int ll;
    unsigned char bl = pdata[0];
    if ( bl & 0x80 )
        {
        ll = bl;
        ll = ( ll << 8 ) | pdata[1];
        fprintf (g_pfilList, " %02X %02X", bl, pdata[1]);
        if ( ! (bl & 0x40) ) ll &= 0x3FFF;
        bWord = true;
        }
    else
        {
        fprintf (g_pfilList, " %02X", bl);
        ll = bl;
        if ( bl & 0x40 ) ll |= 0xFF30;
        bWord = false;
        }
    return ll;
    }

// Decode Spin Bytecode
// Based upon information at: https://github.com/rosco-pc/propeller-wiki/wiki/Spin-Byte-Code
// And in SpinBytecodeDoc_600_260C_007F.spin
// from http://forums.parallax.com/discussion/111684/spin-bytecode
static void AL_ByteCode (const unsigned char *pBinary, int addr, int addrNext)
    {
    struct ByteCode
        {
        const char *psName;
        enum {
            op_None,
            op_Obj_Call_Pair,
            op_Packed_Literal,
            op_Byte_Literal,
            op_Word_Literal,
            op_Near_Long_Literal,
            op_Long_Literal,
            op_Memory_Opcode,
            op_Effect,
            op_Signed_Offset,
            op_Unsigned_Offset,
            op_Unsigned_Effected_Offset
            } op;
        } codes[] = {
            {"Frame_Call_Return", ByteCode::op_None},	// 00
            {"Frame_Call_Noreturn", ByteCode::op_None},	// 01
            {"Frame_Call_Abort", ByteCode::op_None},	// 02
            {"Frame_Call_Trashabort", ByteCode::op_None},	// 03
            {"Branch", ByteCode::op_Signed_Offset},	// 04
            {"Call", ByteCode::op_Byte_Literal},	// 05
            {"Objcall", ByteCode::op_Obj_Call_Pair},	// 06
            {"Objcall_Indexed", ByteCode::op_Obj_Call_Pair},	// 07
            {"Loop_Start", ByteCode::op_Signed_Offset},	// 08
            {"Loop_Continue", ByteCode::op_Signed_Offset},	// 09
            {"Jump_If_False", ByteCode::op_Signed_Offset},	// 0A
            {"Jump_If_True", ByteCode::op_Signed_Offset},	// 0B
            {"Jump_From_Stack", ByteCode::op_None},	// 0C
            {"Compare_Case", ByteCode::op_Signed_Offset},	// 0D
            {"Compare_Case_Range", ByteCode::op_Signed_Offset},	// 0E
            {"Look_Abort", ByteCode::op_None},	// 0F
            {"Lookup_Compare", ByteCode::op_None},	// 10
            {"Lookdown_Compare", ByteCode::op_None},	// 11
            {"Lookuprange_Compare", ByteCode::op_None},	// 12
            {"Lookdownrange_Compare", ByteCode::op_None},	// 13
            {"Quit", ByteCode::op_None},	// 14
            {"Mark_Interpreted", ByteCode::op_None},	// 15
            {"Strsize", ByteCode::op_None},	// 16
            {"Strcomp", ByteCode::op_None},	// 17
            {"Bytefill", ByteCode::op_None},	// 18
            {"Wordfill", ByteCode::op_None},	// 19
            {"Longfill", ByteCode::op_None},	// 1A
            {"Waitpeq", ByteCode::op_None},	// 1B
            {"Bytemove", ByteCode::op_None},	// 1C
            {"Wordmove", ByteCode::op_None},	// 1D
            {"Longmove", ByteCode::op_None},	// 1E
            {"Waitpne", ByteCode::op_None},	// 1F
            {"Clkset", ByteCode::op_None},	// 20
            {"Cogstop", ByteCode::op_None},	// 21
            {"Lockret", ByteCode::op_None},	// 22
            {"Waitcnt", ByteCode::op_None},	// 23
            {"Read_Indexed_Spr", ByteCode::op_None},	// 24
            {"Write_Indexed_Spr", ByteCode::op_None},	// 25
            {"Effect_Indexed_Spr", ByteCode::op_Effect},	// 26
            {"Waitvid", ByteCode::op_None},	// 27
            {"Coginit_Returns", ByteCode::op_None},	// 28
            {"Locknew_Returns", ByteCode::op_None},	// 29
            {"Lockset_Returns", ByteCode::op_None},	// 2A
            {"Lockclr_Returns", ByteCode::op_None},	// 2B
            {"Coginit", ByteCode::op_None},	// 2C
            {"Locknew", ByteCode::op_None},	// 2D
            {"Lockset", ByteCode::op_None},	// 2E
            {"Lockclr", ByteCode::op_None},	// 2F
            {"Abort", ByteCode::op_None},	// 30
            {"Abort_With_Return", ByteCode::op_None},	// 31
            {"Return", ByteCode::op_None},	// 32
            {"Pop_Return", ByteCode::op_None},	// 33
            {"Push_Neg1", ByteCode::op_None},	// 34
            {"Push_0", ByteCode::op_None},	// 35
            {"Push_1", ByteCode::op_None},	// 36
            {"Push_Packed_Lit", ByteCode::op_Packed_Literal},	// 37
            {"Push_Byte_Lit", ByteCode::op_Byte_Literal},	// 38
            {"Push_Word_Lit", ByteCode::op_Word_Literal},	// 39
            {"Push_Mid_Lit", ByteCode::op_Near_Long_Literal},	// 3A
            {"Push_Long_Lit", ByteCode::op_Long_Literal},	// 3B
            {"Unused_Op_$3C", ByteCode::op_None},	// 3C
            {"Indexed_Mem_Op", ByteCode::op_Memory_Opcode},	// 3D
            {"Indexed_Range_Mem_Op", ByteCode::op_Memory_Opcode},	// 3E
            {"Memory_Op", ByteCode::op_Memory_Opcode},	// 3F
            {"Push_VariableMem_Long_0", ByteCode::op_None},	// 40
            {"Pop_VariableMem_Long_0", ByteCode::op_None},	// 41
            {"Effect_VariableMem_Long_0", ByteCode::op_Effect},	// 42
            {"Reference_VariableMem_Long_0", ByteCode::op_None},	// 43
            {"Push_VariableMem_Long_1", ByteCode::op_None},	// 44
            {"Pop_VariableMem_Long_1", ByteCode::op_None},	// 45
            {"Effect_VariableMem_Long_1", ByteCode::op_Effect},	// 46
            {"Reference_VariableMem_Long_1", ByteCode::op_None},	// 47
            {"Push_VariableMem_Long_2", ByteCode::op_None},	// 48
            {"Pop_VariableMem_Long_2", ByteCode::op_None},	// 49
            {"Effect_VariableMem_Long_2", ByteCode::op_Effect},	// 4A
            {"Reference_VariableMem_Long_2", ByteCode::op_None},	// 4B
            {"Push_VariableMem_Long_3", ByteCode::op_None},	// 4C
            {"Pop_VariableMem_Long_3", ByteCode::op_None},	// 4D
            {"Effect_VariableMem_Long_3", ByteCode::op_Effect},	// 4E
            {"Reference_VariableMem_Long_3", ByteCode::op_None},	// 4F
            {"Push_VariableMem_Long_4", ByteCode::op_None},	// 50
            {"Pop_VariableMem_Long_4", ByteCode::op_None},	// 51
            {"Effect_VariableMem_Long_4", ByteCode::op_Effect},	// 52
            {"Reference_VariableMem_Long_4", ByteCode::op_None},	// 53
            {"Push_VariableMem_Long_5", ByteCode::op_None},	// 54
            {"Pop_VariableMem_Long_5", ByteCode::op_None},	// 55
            {"Effect_VariableMem_Long_5", ByteCode::op_Effect},	// 56
            {"Reference_VariableMem_Long_5", ByteCode::op_None},	// 57
            {"Push_VariableMem_Long_6", ByteCode::op_None},	// 58
            {"Pop_VariableMem_Long_6", ByteCode::op_None},	// 59
            {"Effect_VariableMem_Long_6", ByteCode::op_Effect},	// 5A
            {"Reference_VariableMem_Long_6", ByteCode::op_None},	// 5B
            {"Push_VariableMem_Long_7", ByteCode::op_None},	// 5C
            {"Pop_VariableMem_Long_7", ByteCode::op_None},	// 5D
            {"Effect_VariableMem_Long_7", ByteCode::op_Effect},	// 5E
            {"Reference_VariableMem_Long_7", ByteCode::op_None},	// 5F
            {"Push_LocalMem_Long_0", ByteCode::op_None},	// 60
            {"Pop_LocalMem_Long_0", ByteCode::op_None},	// 61
            {"Effect_LocalMem_Long_0", ByteCode::op_Effect},	// 62
            {"Reference_LocalMem_Long_0", ByteCode::op_None},	// 63
            {"Push_LocalMem_Long_1", ByteCode::op_None},	// 64
            {"Pop_LocalMem_Long_1", ByteCode::op_None},	// 65
            {"Effect_LocalMem_Long_1", ByteCode::op_Effect},	// 66
            {"Reference_LocalMem_Long_1", ByteCode::op_None},	// 67
            {"Push_LocalMem_Long_2", ByteCode::op_None},	// 68
            {"Pop_LocalMem_Long_2", ByteCode::op_None},	// 69
            {"Effect_LocalMem_Long_2", ByteCode::op_Effect},	// 6A
            {"Reference_LocalMem_Long_2", ByteCode::op_None},	// 6B
            {"Push_LocalMem_Long_3", ByteCode::op_None},	// 6C
            {"Pop_LocalMem_Long_3", ByteCode::op_None},	// 6D
            {"Effect_LocalMem_Long_3", ByteCode::op_Effect},	// 6E
            {"Reference_LocalMem_Long_3", ByteCode::op_None},	// 6F
            {"Push_LocalMem_Long_4", ByteCode::op_None},	// 70
            {"Pop_LocalMem_Long_4", ByteCode::op_None},	// 71
            {"Effect_LocalMem_Long_4", ByteCode::op_Effect},	// 72
            {"Reference_LocalMem_Long_4", ByteCode::op_None},	// 73
            {"Push_LocalMem_Long_5", ByteCode::op_None},	// 74
            {"Pop_LocalMem_Long_5", ByteCode::op_None},	// 75
            {"Effect_LocalMem_Long_5", ByteCode::op_Effect},	// 76
            {"Reference_LocalMem_Long_5", ByteCode::op_None},	// 77
            {"Push_LocalMem_Long_6", ByteCode::op_None},	// 78
            {"Pop_LocalMem_Long_6", ByteCode::op_None},	// 79
            {"Effect_LocalMem_Long_6", ByteCode::op_Effect},	// 7A
            {"Reference_LocalMem_Long_6", ByteCode::op_None},	// 7B
            {"Push_LocalMem_Long_7", ByteCode::op_None},	// 7C
            {"Pop_LocalMem_Long_7", ByteCode::op_None},	// 7D
            {"Effect_LocalMem_Long_7", ByteCode::op_Effect},	// 7E
            {"Reference_LocalMem_Long_7", ByteCode::op_None},	// 7F
            {"Push_MainMem_Byte", ByteCode::op_None},	// 80
            {"Pop_MainMem_Byte", ByteCode::op_None},	// 81
            {"Effect_MainMem_Byte", ByteCode::op_Effect},	// 82
            {"Reference_MainMem_Byte", ByteCode::op_None},	// 83
            {"Push_ObjectMem_Byte", ByteCode::op_Unsigned_Offset},	// 84
            {"Pop_ObjectMem_Byte", ByteCode::op_Unsigned_Offset},	// 85
            {"Effect_ObjectMem_Byte", ByteCode::op_Unsigned_Effected_Offset},	// 86
            {"Reference_ObjectMem_Byte", ByteCode::op_Unsigned_Offset},	// 87
            {"Push_VariableMem_Byte", ByteCode::op_Unsigned_Offset},	// 88
            {"Pop_VariableMem_Byte", ByteCode::op_Unsigned_Offset},	// 89
            {"Effect_VariableMem_Byte", ByteCode::op_Unsigned_Effected_Offset},	// 8A
            {"Reference_VariableMem_Byte", ByteCode::op_Unsigned_Offset},	// 8B
            {"Push_LocalMem_Byte", ByteCode::op_Unsigned_Offset},	// 8C
            {"Pop_LocalMem_Byte", ByteCode::op_Unsigned_Offset},	// 8D
            {"Effect_LocalMem_Byte", ByteCode::op_Unsigned_Effected_Offset},	// 8E
            {"Reference_LocalMem_Byte", ByteCode::op_Unsigned_Offset},	// 8F
            {"Push_Indexed_MainMem_Byte", ByteCode::op_None},	// 90
            {"Pop_Indexed_MainMem_Byte", ByteCode::op_None},	// 91
            {"Effect_Indexed_MainMem_Byte", ByteCode::op_Effect},	// 92
            {"Reference_Indexed_MainMem_Byte", ByteCode::op_None},	// 93
            {"Push_Indexed_ObjectMem_Byte", ByteCode::op_Unsigned_Offset},	// 94
            {"Pop_Indexed_ObjectMem_Byte", ByteCode::op_Unsigned_Offset},	// 95
            {"Effect_Indexed_ObjectMem_Byte", ByteCode::op_Unsigned_Effected_Offset},	// 96
            {"Reference_Indexed_ObjectMem_Byte", ByteCode::op_Unsigned_Offset},	// 97
            {"Push_Indexed_VariableMem_Byte", ByteCode::op_Unsigned_Offset},	// 98
            {"Pop_Indexed_VariableMem_Byte", ByteCode::op_Unsigned_Offset},	// 99
            {"Effect_Indexed_VariableMem_Byte", ByteCode::op_Unsigned_Effected_Offset},	// 9A
            {"Reference_Indexed_VariableMem_Byte", ByteCode::op_Unsigned_Offset},	// 9B
            {"Push_Indexed_LocalMem_Byte", ByteCode::op_Unsigned_Offset},	// 9C
            {"Pop_Indexed_LocalMem_Byte", ByteCode::op_Unsigned_Offset},	// 9D
            {"Effect_Indexed_LocalMem_Byte", ByteCode::op_Unsigned_Effected_Offset},	// 9E
            {"Reference_Indexed_LocalMem_Byte", ByteCode::op_Unsigned_Offset},	// 9F
            {"Push_MainMem_Word", ByteCode::op_None},	// A0
            {"Pop_MainMem_Word", ByteCode::op_None},	// A1
            {"Effect_MainMem_Word", ByteCode::op_Effect},	// A2
            {"Reference_MainMem_Word", ByteCode::op_None},	// A3
            {"Push_ObjectMem_Word", ByteCode::op_Unsigned_Offset},	// A4
            {"Pop_ObjectMem_Word", ByteCode::op_Unsigned_Offset},	// A5
            {"Effect_ObjectMem_Word", ByteCode::op_Unsigned_Effected_Offset},	// A6
            {"Reference_ObjectMem_Word", ByteCode::op_Unsigned_Offset},	// A7
            {"Push_VariableMem_Word", ByteCode::op_Unsigned_Offset},	// A8
            {"Pop_VariableMem_Word", ByteCode::op_Unsigned_Offset},	// A9
            {"Effect_VariableMem_Word", ByteCode::op_Unsigned_Effected_Offset},	// AA
            {"Reference_VariableMem_Word", ByteCode::op_Unsigned_Offset},	// AB
            {"Push_LocalMem_Word", ByteCode::op_Unsigned_Offset},	// AC
            {"Pop_LocalMem_Word", ByteCode::op_Unsigned_Offset},	// AD
            {"Effect_LocalMem_Word", ByteCode::op_Unsigned_Effected_Offset},	// AE
            {"Reference_LocalMem_Word", ByteCode::op_Unsigned_Offset},	// AF
            {"Push_Indexed_MainMem_Word", ByteCode::op_None},	// B0
            {"Pop_Indexed_MainMem_Word", ByteCode::op_None},	// B1
            {"Effect_Indexed_MainMem_Word", ByteCode::op_Effect},	// B2
            {"Reference_Indexed_MainMem_Word", ByteCode::op_None},	// B3
            {"Push_Indexed_ObjectMem_Word", ByteCode::op_Unsigned_Offset},	// B4
            {"Pop_Indexed_ObjectMem_Word", ByteCode::op_Unsigned_Offset},	// B5
            {"Effect_Indexed_ObjectMem_Word", ByteCode::op_Unsigned_Effected_Offset},	// B6
            {"Reference_Indexed_ObjectMem_Word", ByteCode::op_Unsigned_Offset},	// B7
            {"Push_Indexed_VariableMem_Word", ByteCode::op_Unsigned_Offset},	// B8
            {"Pop_Indexed_VariableMem_Word", ByteCode::op_Unsigned_Offset},	// B9
            {"Effect_Indexed_VariableMem_Word", ByteCode::op_Unsigned_Effected_Offset},	// BA
            {"Reference_Indexed_VariableMem_Word", ByteCode::op_Unsigned_Offset},	// BB
            {"Push_Indexed_LocalMem_Word", ByteCode::op_Unsigned_Offset},	// BC
            {"Pop_Indexed_LocalMem_Word", ByteCode::op_Unsigned_Offset},	// BD
            {"Effect_Indexed_LocalMem_Word", ByteCode::op_Unsigned_Effected_Offset},	// BE
            {"Reference_Indexed_LocalMem_Word", ByteCode::op_Unsigned_Offset},	// BF
            {"Push_MainMem_Long", ByteCode::op_None},	// C0
            {"Pop_MainMem_Long", ByteCode::op_None},	// C1
            {"Effect_MainMem_Long", ByteCode::op_Effect},	// C2
            {"Reference_MainMem_Long", ByteCode::op_None},	// C3
            {"Push_ObjectMem_Long", ByteCode::op_Unsigned_Offset},	// C4
            {"Pop_ObjectMem_Long", ByteCode::op_Unsigned_Offset},	// C5
            {"Effect_ObjectMem_Long", ByteCode::op_Unsigned_Effected_Offset},	// C6
            {"Reference_ObjectMem_Long", ByteCode::op_Unsigned_Offset},	// C7
            {"Push_VariableMem_Long", ByteCode::op_Unsigned_Offset},	// C8
            {"Pop_VariableMem_Long", ByteCode::op_Unsigned_Offset},	// C9
            {"Effect_VariableMem_Long", ByteCode::op_Unsigned_Effected_Offset},	// CA
            {"Reference_VariableMem_Long", ByteCode::op_Unsigned_Offset},	// CB
            {"Push_LocalMem_Long", ByteCode::op_Unsigned_Offset},	// CC
            {"Pop_LocalMem_Long", ByteCode::op_Unsigned_Offset},	// CD
            {"Effect_LocalMem_Long", ByteCode::op_Unsigned_Effected_Offset},	// CE
            {"Reference_LocalMem_Long", ByteCode::op_Unsigned_Offset},	// CF
            {"Push_Indexed_MainMem_Long", ByteCode::op_None},	// D0
            {"Pop_Indexed_MainMem_Long", ByteCode::op_None},	// D1
            {"Effect_Indexed_MainMem_Long", ByteCode::op_Effect},	// D2
            {"Reference_Indexed_MainMem_Long", ByteCode::op_None},	// D3
            {"Push_Indexed_ObjectMem_Long", ByteCode::op_Unsigned_Offset},	// D4
            {"Pop_Indexed_ObjectMem_Long", ByteCode::op_Unsigned_Offset},	// D5
            {"Effect_Indexed_ObjectMem_Long", ByteCode::op_Unsigned_Effected_Offset},	// D6
            {"Reference_Indexed_ObjectMem_Long", ByteCode::op_Unsigned_Offset},	// D7
            {"Push_Indexed_VariableMem_Long", ByteCode::op_Unsigned_Offset},	// D8
            {"Pop_Indexed_VariableMem_Long", ByteCode::op_Unsigned_Offset},	// D9
            {"Effect_Indexed_VariableMem_Long", ByteCode::op_Unsigned_Effected_Offset},	// DA
            {"Reference_Indexed_VariableMem_Long", ByteCode::op_Unsigned_Offset},	// DB
            {"Push_Indexed_LocalMem_Long", ByteCode::op_Unsigned_Offset},	// DC
            {"Pop_Indexed_LocalMem_Long", ByteCode::op_Unsigned_Offset},	// DD
            {"Effect_Indexed_LocalMem_Long", ByteCode::op_Unsigned_Effected_Offset},	// DE
            {"Reference_Indexed_LocalMem_Long", ByteCode::op_Unsigned_Offset},	// DF
            {"Rotate_Right", ByteCode::op_None},	// E0
            {"Rotate_Left", ByteCode::op_None},	// E1
            {"Shift_Right", ByteCode::op_None},	// E2
            {"Shift_Left", ByteCode::op_None},	// E3
            {"Limit_Min", ByteCode::op_None},	// E4
            {"Limit_Max", ByteCode::op_None},	// E5
            {"Negate", ByteCode::op_None},	// E6
            {"Complement", ByteCode::op_None},	// E7
            {"Bit_And", ByteCode::op_None},	// E8
            {"Absolute_Value", ByteCode::op_None},	// E9
            {"Bit_Or", ByteCode::op_None},	// EA
            {"Bit_Xor", ByteCode::op_None},	// EB
            {"Add", ByteCode::op_None},	// EC
            {"Subtract", ByteCode::op_None},	// ED
            {"Arith_Shift_Right", ByteCode::op_None},	// EE
            {"Bit_Reverse", ByteCode::op_None},	// EF
            {"Logical_And", ByteCode::op_None},	// F0
            {"Encode", ByteCode::op_None},	// F1
            {"Logical_Or", ByteCode::op_None},	// F2
            {"Decode", ByteCode::op_None},	// F3
            {"Multiply", ByteCode::op_None},	// F4
            {"Multiply_Hi", ByteCode::op_None},	// F5
            {"Divide", ByteCode::op_None},	// F6
            {"Modulo", ByteCode::op_None},	// F7
            {"Square_Root", ByteCode::op_None},	// F8
            {"Less", ByteCode::op_None},	// F9
            {"Greater", ByteCode::op_None},	// FA
            {"Not_Equal", ByteCode::op_None},	// FB
            {"Equal", ByteCode::op_None},	// FC
            {"Less_Equal", ByteCode::op_None},	// FD
            {"Greater_Equal", ByteCode::op_None},	// FE
            {"Logical_Not", ByteCode::op_None}	// FF
        };

    while (addr < addrNext )
        {
        unsigned char bc = pBinary[addr];
        fprintf (g_pfilList, "%04X     %02X", addr, bc);
        std::string sArgs = "";
        int nCol = 11;
        char sAddr[12];
        unsigned char bl;
        unsigned int ll;
        bool bWord = false;
        switch (codes[bc].op)
            {
            case ByteCode::op_None:
                break;
            case ByteCode::op_Obj_Call_Pair:
                bl = pBinary[++addr];
                fprintf (g_pfilList, " %02X", bl);
                nCol += 3;
                sprintf (sAddr, ", $%02X", bl);
                sArgs += sAddr;
                bl = pBinary[++addr];
                fprintf (g_pfilList, " %02X", bl);
                nCol += 3;
                sprintf (sAddr, ", $%02X", bl);
                sArgs += sAddr;
                break;
            case ByteCode::op_Packed_Literal:
                bl = pBinary[++addr];
                ll = 2 << ( bl & 0x1F );
                if ( bl & 0x20 ) --ll;
                if ( bl & 0x40 ) ll = ~ll;
                fprintf (g_pfilList, " %02X", bl);
                nCol += 3;
                sprintf (sAddr, ", $%08X", ll);
                sArgs += sAddr;
                break;
            case ByteCode::op_Byte_Literal:
                bl = pBinary[++addr];
                fprintf (g_pfilList, " %02X", bl);
                nCol += 3;
                sprintf (sAddr, ", $%02X", bl);
                sArgs += sAddr;
                break;
            case ByteCode::op_Word_Literal:
                ll = AL_BWord (&pBinary[++addr]);
                ++addr;
                fprintf (g_pfilList, " %04X", ll);
                nCol += 5;
                sprintf (sAddr, ", $%04X", ll);
                sArgs += sAddr;
                break;
            case ByteCode::op_Near_Long_Literal:
                ll = AL_BInt24 (&pBinary[++addr]);
                addr += 2;
                fprintf (g_pfilList, " %06X", ll);
                nCol += 7;
                sprintf (sAddr, ", $%06X", ll);
                sArgs += sAddr;
                break;
            case ByteCode::op_Long_Literal:
                ll = AL_BLong (&pBinary[++addr]);
                addr += 3;
                fprintf (g_pfilList, " %08X", ll);
                nCol += 9;
                sprintf (sAddr, ", $%08X", ll);
                sArgs += sAddr;
                break;
            case ByteCode::op_Signed_Offset:
            case ByteCode::op_Unsigned_Offset:
            case ByteCode::op_Memory_Opcode:
                ll = AL_Address (&pBinary[++addr], bWord);
                if ( bWord )
                    {
                    ++addr;
                    nCol += 3;
                    }
                nCol += 3;
                sprintf (sAddr, ", $%04X", ll);
                sArgs += sAddr;
                break;
            case ByteCode::op_Unsigned_Effected_Offset:
                ll = AL_Address (&pBinary[++addr], bWord);
                if ( bWord )
                    {
                    ++addr;
                    nCol += 3;
                    }
                nCol += 3;
                sprintf (sAddr, ", $%04X", ll);
                sArgs += sAddr;
                // Deliberately falls through to next case
            case ByteCode::op_Effect:
                bl = pBinary[++addr];
                fprintf (g_pfilList, " %02X", bl);
                nCol += 3;
                if ( bl & 0x40 )
                    {
                    sArgs += ", ";
                    sArgs += codes[(bl & 0x1F) + 0xE0].psName;
                    if ( ! (bl & 0x20) ) sArgs += ", swap";
                    }
                else if ( bl & 0x20 )
                    {
                    switch (bl & 0x18)
                        {
                        case 0x00: sArgs += ", pre-inc"; break;
                        case 0x08: sArgs += ", post-inc"; break;
                        case 0x10: sArgs += ", pre-dec"; break;
                        case 0x18: sArgs += ", post-dec"; break;
                        }
                    switch (bl & 0x06)
                        {
                        case 0x00: sArgs += ", bit"; break;
                        case 0x02: sArgs += ", byte"; break;
                        case 0x04: sArgs += ", word"; break;
                        case 0x06: sArgs += ", long"; break;
                        }
                    }
                else
                    {
                    switch (bl & 0x1C)
                        {
                        case 0x00:
                        case 0x04:
                            if (bl & 0x06)
                                {
                                sArgs += ", repeat-var loop";
                                if (bl & 0x04) sArgs += ", pop step";
                                ll = AL_Address (&pBinary[++addr], bWord);
                                if ( bWord )
                                    {
                                    ++addr;
                                    nCol += 3;
                                    }
                                nCol += 3;
                                sprintf (sAddr, ", $%04X", ll);
                                sArgs += sAddr;
                                }
                            else
                                {
                                sArgs += ", write";
                                }
                            break;
                        case 0x08: sArgs += ", random forward"; break;
                        case 0x0C: sArgs += ", random reverse"; break;
                        case 0x10: sArgs += ", sign-extend byte"; break;
                        case 0x14: sArgs += ", sign-extend word"; break;
                        case 0x18: sArgs += ", post-clear"; break;
                        case 0x1C: sArgs += ", post-set"; break;
                        }
                    if (bl & 0x80) sArgs += ", push";
                    }
                break;
            }
        while (nCol < 22)
            {
            fprintf (g_pfilList, " ");
            ++nCol;
            }
        fprintf (g_pfilList, " ; %s%s\n", codes[bc].psName, sArgs.c_str ());
        ++addr;
        }
    }

// List a section of binary data
static void AL_Dump (const unsigned char *pBinary, AL_Type at, int addr, int addrNext, int caddr)
    {
    if ( at == atSpinCode )
        {
        AL_ByteCode (pBinary, addr, addrNext);
        return;
        }
    if ( addr < addrNext )
        {
        if ((caddr >= 0) && (caddr < 0x800) && ((caddr & 0x03) == 0))
            fprintf (g_pfilList, "%04X %03X", addr, caddr >> 2);
        else
            fprintf (g_pfilList, "%04X    ", addr);
        }
    int nByte = 0;
    while (addr < addrNext)
        {
        if (( at == atLong ) && ( addr + 4 > addrNext )) at = atWord;
        if (( at == atWord ) && ( addr + 2 > addrNext )) at = atByte;
        if ( at == atLong )
            {
            fprintf (g_pfilList, " %08X", AL_Long (&pBinary[addr]));
            addr += 4;
            if ( caddr >= 0 ) caddr += 4;
            }
        else if ( at == atWord )
            {
            fprintf (g_pfilList, " %04X", AL_Word (&pBinary[addr]));
            addr += 2;
            if ( caddr >= 0 ) caddr += 2;
            }
        else
            {
            fprintf (g_pfilList, " %02X", pBinary[addr]);
            ++addr;
            if ( caddr >= 0 ) ++caddr;
            }
        if (( ++nByte >= 16 ) && (addr < addrNext))
            {
            if ((caddr >= 0) && (caddr < 0x800) && ((caddr & 0x03) == 0))
                fprintf (g_pfilList, "\n%04X %03X", addr, caddr >> 2);
            else
                fprintf (g_pfilList, "\n%04X    ", addr);
            nByte = 0;
            }
        }
    fprintf (g_pfilList, "\n");
    }

// AL_SourceLine Methods:

// Constructor
AL_SourceLine::AL_SourceLine(AL_Type at, int addr, int caddr)
    {
    m_addr = addr;
    m_caddr = caddr;
    m_types[0] = at;
    }

// Change data type
void AL_SourceLine::SetType(AL_Type at, int addr)
    {
    if ( addr < m_addr ) m_types[0] = at;
    else m_types[addr - m_addr] = at;
    }

// Changes of type
void AL_SourceLine::Changes (std::vector<int> &offsets) const
    {
    std::map<int, AL_Type>::const_iterator it;
    for (it = m_types.begin (); it != m_types.end (); ++it)
        {
        offsets.push_back (it->first);
        }
    std::sort (offsets.begin (), offsets.end ());
    }

// AL_Point Methods:

// Add a source line location
void AL_Point::AddLine (const AL_Object *pobj, int posn)
    {
    m_pobj = pobj;
    std::vector<int>::iterator it;
    for (it = m_lines.begin(); it != m_lines.end(); ++it)
        {
        if ( *it == posn ) return;
        }
    m_lines.push_back(posn);
    }

// Sort source lines and return count
int AL_Point::Sort (void)
    {
    std::sort (m_lines.begin (), m_lines.end ());
    return m_lines.size ();
    }

// Get location of source line
int AL_Point::GetLine (int index) const
    {
    return m_lines[index];
    }

// AL_Object Methods:

// Constructor
AL_Object::AL_Object (const char *psFile, const char *psPath)
    {
    m_sFile = psFile;
    m_sPath = psPath;
    m_bOnHeap = false;
    if (g_preprocessor) m_ppstate = pp_get_define_state(g_preprocessor);
    else m_ppstate = NULL;
    m_bFixup = false;
    }

// Add location of a source line
void AL_Object::AddLine (AL_Type at, int posn, int addr, int caddr)
    {
    m_lines[posn] = AL_SourceLine (at, addr, caddr);
    m_posnLast = posn;
    }

// Add a type change to the last source line
void AL_Object::SetType (AL_Type at, int addr)
    {
    m_lines[m_posnLast].SetType (at, addr);
    }

// Include a sub-object from heap
void AL_Object::Include (AL_Object *pobj, int nOffset)
    {
    m_include.push_back (std::pair<AL_Object *, int>(pobj, nOffset));
    }

// Define an Object Global Variable
void AL_Object::AddVariable (const char *psName, int nSize, int nCount)
    {
    m_variables[nSize].push_back (std::pair<std::string, int> (std::string (psName), nCount));
    }

// Add a Sub-Object reference
void AL_Object::AddSubObject (int posn, const char *psName, const AL_Object *pobj, int nCount)
    {
    m_subobjs[posn] = AL_ObjRef (psName, pobj, nCount);
    }

// Add an Object Distillation record
void AL_Object::AddDistill (int start, int length, int reloc)
    {
    if ( ( ! m_bFixup ) && ( m_distill.size () > 0 ) ) m_bFixup = ( start != m_distill.back ().m_end );
    m_distill.push_back (AL_DistRec (start, length, reloc));
    }

// Translate an address following distillation
int AL_Object::DistillAddr (int addr)
    {
    if ( ! m_bFixup ) return addr;
    std::vector<AL_DistRec>::iterator it;
    for (it = m_distill.begin (); it != m_distill.end (); ++it)
        {
        if ( ( addr >= it->m_start ) && ( addr < it->m_end ) ) return addr + it->m_reloc;
        }
    return -1;
    }

// Associate source lines with addresses in the binary
void AL_Object::PlaceLines (const AL_Object *pobj, int nOffset)
    {
    std::map<int, AL_SourceLine>::const_iterator it;
    int addr = DistillAddr (nOffset);
    if ( addr >= 0 )
        {
        m_points[addr] = AL_Point (pobj);
        for (it = pobj->m_lines.begin (); it != pobj->m_lines.end (); ++it)
            {
            addr = DistillAddr (it->second.Addr () + nOffset);
            if ( addr >= 0 ) m_points[addr].AddLine (pobj, it->first);
            }
        std::vector<std::pair<AL_Object *, int> >::const_iterator it2;
        for (it2 = pobj->m_include.begin (); it2 != pobj->m_include.end (); ++it2)
            {
            PlaceLines (it2->first, it2->second + nOffset);
            }
        }
    }

// Output variable locations
void AL_Object::ListVariables (const std::string *psName, int &addr) const
    {
    if ( m_variables[0].size () + m_variables[1].size () + m_variables[2].size () > 0 )
        {
        const char *psSize[] = { "byte", "word", "long" };
        fprintf (g_pfilList, "                       Variables for %s (%s)\n", psName->c_str (), m_sFile.c_str ());
        addr = ( addr + 3 ) & 0xFFFC;
        int addrBase = addr;
        std::vector<std::pair<std::string, int> >::const_iterator it;
        for (int iSize = 2; iSize >= 0; --iSize)
            {
            for (it = m_variables[iSize].begin (); it != m_variables[iSize].end (); ++it)
                {
                fprintf (g_pfilList, "%04X     %04X          %s %s", addr, addr - addrBase, psSize[iSize],
                    it->first.c_str ());
                if ( it->second > 1 ) fprintf (g_pfilList, "[%d]\n", it->second);
                else fprintf (g_pfilList, "\n");
                addr += ( it->second ) << iSize;
                }
            }
        }
    std::vector<int> order;
    std::map<int, AL_ObjRef>::const_iterator it2;
    for (it2 = m_subobjs.begin (); it2 != m_subobjs.end (); ++it2)
        {
        order.push_back (it2->first);
        }
    std::sort (order.begin (), order.end ());
    std::vector<int>::const_iterator it3;
    for (it3 = order.begin (); it3 != order.end (); ++it3)
        {
        const AL_ObjRef &objref = m_subobjs.at(*it3);
        if ( objref.m_count > 1 )
            {
            for (int i = 0; i < objref.m_count; ++i)
                {
                char sDim[12];
                sprintf (sDim, "[%d]", i);
                std::string sName = *psName + "." + objref.m_name + sDim;
                objref.m_pobj->ListVariables (&sName, addr);
                }
            }
        else
            {
            std::string sName = *psName + "." + objref.m_name;
            objref.m_pobj->ListVariables (&sName, addr);
            }
        }
    }

// Output the annotated listing
void AL_Object::Output (const unsigned char *pBinary, int nSize, const struct CompilerData *pcd)
    {
    std::vector<int>adlist;
    std::map<int, AL_Point>::iterator it;
    const int iShift = 0x10;
    PlaceLines (this, 0);
    for (it = m_points.begin (); it != m_points.end (); ++it)
        {
        adlist.push_back (it->first + iShift);
        }
    std::sort (adlist.begin (), adlist.end ());
    int addr = 0;
    unsigned int iFreq = *((unsigned int *)(&pBinary[addr]));
    fprintf (g_pfilList, "%04X     %08X      Frequency %d Hz\n", 0, iFreq, iFreq);
    addr += 4;
    unsigned char bclk = pBinary[addr];
    std::string sClock = "Clock mode: ";
    if ( bclk & 0x20 )
        {
        static const char *psSrc[] = {"XInput", "Xtal1", "Xtal2", "Xtal3" };
        sClock += psSrc[(bclk & 0x18) >> 3];
        if ( ( bclk & 0x40 ) && ( ( bclk & 0x07 ) >= 0x03 ) )
            {
            char sPLL[10];
            sprintf (sPLL, " + Pll%dX", 1 << ( ( bclk & 0x07 ) - 0x03 ));
            sClock += sPLL;
            }
        }
    else
        {
        sClock = ( bclk & 0x01 ) ? "RCSlow" : "RCFast";
        }
    fprintf (g_pfilList, "%04X     %02X            %s\n", addr, bclk, sClock.c_str ());
    ++addr;
    fprintf (g_pfilList, "%04X     %02X            Check Sum\n", addr, pBinary[addr]);
    ++addr;
    static const char *psTop[] = { "Base of Program", "Base of Variables", "Base of Stack",
                                   "Initial Program Counter", "Initial Stack Pointer" };
    for (int i = 0; i < 5; ++i)
        {
        fprintf (g_pfilList, "%04X     %04X          %s\n", addr, AL_Word(&pBinary[addr]), psTop[i]);
        addr += 2;
        }
    size_t iPoint = 0;
    int addrNext = adlist[iPoint];
    int caddr = -1;
    AL_Type at = atByte;
    while (addr < nSize)
        {
        if ( addr < addrNext )
            {
            AL_Dump (pBinary, at, addr, addrNext, caddr);
            addr = addrNext;
            }
        if ( iPoint < adlist.size () )
            {
            int posn;
            char sCog[4];
            AL_Point &point = m_points[addr - iShift];
            const AL_Object *pobj = point.Object ();
            AL_LoadSource (pobj);
            int nLine = point.Sort ();
            if ( nLine > 0 )
                {
                std::vector<int> order;
                for (int i = 0; i < nLine; ++i)
                    {
                    posn = point.GetLine (i);
                    const AL_SourceLine &line = pobj->m_lines.at (posn);
                    at = line.Type ();
                    if ( at >= atDAT ) order.push_back (posn);
                    }
                for (int i = 0; i < nLine; ++i)
                    {
                    posn = point.GetLine (i);
                    const AL_SourceLine &line = pobj->m_lines.at (posn);
                    at = line.Type ();
                    if ( at < atDAT ) order.push_back (posn);
                    }
                for (int i = 0; i < nLine - 1; ++i)
                    {
                    posn = order[i];
                    const AL_SourceLine &line = pobj->m_lines.at (posn);
                    at = line.Type ();
                    caddr = line.CogAddr ();
                    if (( at == atPASM ) && ( caddr >= 0 ) && ( caddr < 0x800 ) && ((caddr & 0x03) == 0))
                        fprintf (g_pfilList, "%04X %03X               ", addr, caddr >> 2);
                    else
                        fprintf (g_pfilList, "%04X                   ", addr);
                    AL_Print (posn);
                    }
                posn = order[nLine - 1];
                const AL_SourceLine &line = pobj->m_lines.at (posn);
                at = line.Type ();
                caddr = line.CogAddr ();
                if (( caddr >= 0 ) && ( caddr < 0x800 ) && ((caddr & 0x03) == 0))
                    sprintf (sCog, "%03X", caddr >> 2);
                else
                    strcpy (sCog, "   ");
                switch (at)
                    {
                    case atPASM:
                    case atLong:
                        fprintf (g_pfilList, "%04X %s %08X      ", addr, sCog, AL_Long (&pBinary[addr]));
                        addr += 4;
                        if (caddr >= 0) caddr += 4;
                        break;
                    case atSpinObj:
                        fprintf (g_pfilList, "%04X %s %04X %04X     ", addr, sCog, AL_Word (&pBinary[addr]),
                            AL_Word (&pBinary[addr+2]));
                        addr += 4;
                        if (caddr >= 0) caddr += 4;
                        at = atWord;
                        break;
                    case atWord:
                        fprintf (g_pfilList, "%04X %s %04X          ", addr, sCog, AL_Word (&pBinary[addr]));
                        addr += 2;
                        if (caddr >= 0) caddr += 2;
                        break;
                    case atByte:
                        fprintf (g_pfilList, "%04X %s %02X            ", addr, sCog, pBinary[addr]);
                        ++addr;
                        if (caddr >= 0) ++caddr;
                        break;
                    default:
                        fprintf (g_pfilList, "%04X %s               ", addr, sCog);
                        break;
                    }
                AL_Print (posn);
                if ( line.NumTypes () > 1 )
                    {
                    std::vector<int> changes;
                    line.Changes (changes);
                    for (size_t i = 0; i < changes.size () - 1; ++i)
                        {
                        AL_Dump (pBinary, line.Type (changes[i]), addr + changes[i], addr + changes[i+1], caddr);
                        }
                    at = line.Type (changes[changes.size () - 1]);
                    addr += changes[changes.size () - 1];
                    }
                }
            else
                {
                fprintf (g_pfilList, "%04X     %04X %04X     Link to Next Object\n",
                    addr, AL_Word (&pBinary[addr]), AL_Word (&pBinary[addr+2]));
                addr += 4;
                std::vector<int>::const_iterator it;
                for (it = pobj->m_routines.begin (); it != pobj->m_routines.end (); ++it)
                    {
                    fprintf (g_pfilList, "%04X     %04X %04X     Link to ",
                        addr, AL_Word (&pBinary[addr]), AL_Word (&pBinary[addr+2]));
                    AL_Print (*it);
                    addr += 4;
                    }
                }
            }
        do
            {
            if ( ++iPoint < adlist.size () ) addrNext = adlist[iPoint];
            else addrNext = nSize;
            }
        while (( addrNext < addr ) && ( addrNext < nSize ));
        }
    addr =  AL_Word(&pBinary[8]);
    std::string sName = "TOP";
    ListVariables (&sName, addr);
    addr = ( addr + 3 ) & 0xFFFC;
    fprintf (g_pfilList, "%04X                   Reserved 8 bytes.\n", addr);
    addr = AL_Word(&pBinary[10]);
    fprintf (g_pfilList, "%04X                   Base of stack.\n", addr);
    addr += pcd->stack_requirement;
    fprintf (g_pfilList, "%04X                   Top of stack.\n", addr);
    }
