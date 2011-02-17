/***	dump.cc	***/

/*
  A utility for printing the contents of an automaton.
  In contrast to fsa_prefix, this program prints information
  about nodes and arcs, and their constituents.
*/

#include	<iostream>
#include	<fstream>
#include	<string.h>
#include	<new>

#define		TRUE	1
#define		FALSE	0

int		global_gtl;

using namespace std;

struct signature {		/* dictionary file signature */
  char          sig[4];         /* automaton identifier (magic number) */
  char          ver;            /* automaton type number */
  char          filler;         /* char used as filler */
  char          annot_sep;      /* char that separates annotations from lex */
  char          gtl;            /* length of go_to field */
};/*struct signature */

/* Name:	bytes2int
 * Class:	None.
 * Purpose:	Convert a sequence of bytes to integer.
 * Parameters:	bytes	- (i) sequence of bytes,
 *		n	- (i) length of the sequence.
 * Returns:	Equivalent integer.
 * Remarks:	Needed for portability of automata.
 */
inline unsigned int
bytes2int(const unsigned char *bytes, const int n)
{
  unsigned int r = 0;
  int i;
  for (i = n - 1; i >= 0; --i) {
    r <<= 8; r |= bytes[i];
  }
  return r;
}

/* Name:	usage
 * Class:	None.
 * Purpose:	Explains invocation syntax.
 * Parameters:	name	- (i) program name.
 * Returns:	Nothing.
 * Remarks:	The description is sent to standard error.
 */
static void
usage(const char *name)
{
  cerr << name << " prints contents of an automaton arc-wise" << endl
       << "Synopsis: " << name << "automaton [>output_file]" << endl;
}

// Globals - only for sparse matrix representation
char min_char, max_char;	// smallest and greatest label
int  sparse_gtl;		// pointer length (in bytes) for sparse vector


/* Name:	print_sig
 * Class:	None.
 * Parameters:	inp		- (i) input stream,
 *		dict_file_name	- (i) name of input file.
 * Returns:	Version number of the automaton.
 * Remarks:	None.
 */
int
print_sig(ifstream &inp, const char *dict_file_name)
{
  signature	sig_arc;
  unsigned int	tmp;

  if (!inp.read((char *)&sig_arc, sizeof(sig_arc))) {
    cerr << "Cannot read signature" << endl;
    exit(1);
  }
  if (strncmp(sig_arc.sig, "\\fsa", (size_t)4)) {
    cerr << "Invalid dictionary file (bad magic number): " << dict_file_name
      << endl;
    return(FALSE);
  }
  tmp = sig_arc.ver;
  cout << "Automaton version: " << dec << tmp << endl;
  cout << "Dictionary was build:" << endl;
  switch (sig_arc.ver) {
  case 0:
    cout << "without FLEXIBLE," << endl << "without LARGE_DICTIONARIES,"
	 << endl
	 << "without STOPBIT," << endl << "without NEXTBIT" << endl;
    cout << "Version not supported by the tool. Exiting." << endl;
    exit(4);
    break;
  case '\x80':
    cout << "without FLEXIBLE," << endl << "with LARGE DICTIONARIES," << endl
	 << "without STOPBIT," << endl << "without NEXTBIT" << endl;
    cout << "Version not supported by the tool. Exiting." << endl;
    exit(4);
    break;
  case 1:
    cout << "with FLEXIBLE," << endl << "without LARGE DICTIONARIES," << endl
	 << "without STOPBIT," << endl << "without NEXTBIT" << endl;
    cout << "Version not supported by the tool. Exiting." << endl;
    exit(4);
    break;
  case 2:
    cout << "with FLEXIBLE," << endl << "without LARGE_DICTIONARIES," << endl
	 << "without STOPBIT," << endl << "with NEXTBIT" << endl;
    cout << "Version not supported by the tool. Exiting." << endl;
    exit(4);
    break;
  case 4:
    cout << "with FLEXIBLE," << endl << "without LARGE_DICTIONARIES," << endl
	 << "with STOPBIT," << "without NEXTBIT," << endl
	 << "without TAILS," << endl << "without SPARSE" << endl;
    break;
  case 5:
    cout << "with FLEXIBLE," << endl << "without LARGE_DICTIONARIES," << endl
	 << "with STOPBIT," << endl << "with NEXTBIT," << endl
	 << "without TAILS," << endl << "without SPARSE" << endl;
    break;
  case 6:
    cout << "with FLEXIBLE," << endl << "without LARGE_DICTIONARIES," << endl
	 << "with STOPBIT," << endl << "without NEXTBIT," << endl
	 << "with TAILS," << endl << "without SPARSE" << endl;
  case 7:
    cout << "with FLEXIBLE," << endl << "without LARGE_DICTIONARIES," << endl
	 << "with STOPBIT," << endl << "with NEXTBIT," << endl
	 << "with TAILS" << endl << "without SPARSE" << endl;
    break;
  case 8:
    cout << "with FLEXIBLE," << endl << "without LARGE_DICTIONARIES," << endl
	 << "with STOPBIT," << endl << "with NEXTBIT," << endl
	 << "without TAILS," << endl << "with WEIGHTED and -W in build"
	 << endl;
    break;
  case 9:
    cout << "with FLEXIBLE," << endl << "without LARGE_DICTIONARIES," << endl
	 << "with STOPBIT," << "without NEXTBIT," << endl
	 << "without TAILS," << endl << "with SPARSE" << endl;
    break;
  case 10:
    cout << "with FLEXIBLE," << endl << "without LARGE_DICTIONARIES," << endl
	 << "with STOPBIT," << endl << "with NEXTBIT," << endl
	 << "without TAILS," << endl << "with SPARSE" << endl;
    break;
  case 11:
    cout << "with FLEXIBLE," << endl << "without LARGE_DICTIONARIES," << endl
	 << "with STOPBIT," << endl << "without NEXTBIT," << endl
	 << "with TAILS," << endl << "with SPARSE" << endl;
    break;
  case 12:
    cout << "with FLEXIBLE," << endl << "without LARGE_DICTIONARIES," << endl
	 << "with STOPBIT," << endl << "with NEXTBIT," << endl
	 << "with TAILS," << endl << "with SPARSE" << endl;
    break;
  default:
    cout << "with yet unknown compile options (upgrade your software)"
	 << endl;
    exit(3);
  }
  cout << "Filler is `" << sig_arc.filler << "'" << endl
       << "Annotation separator is `" << sig_arc.annot_sep << "'" << endl;
  global_gtl = (unsigned char)(sig_arc.gtl);
  cout << "Goto length is: " <<  (global_gtl & 0x0f) << endl;
  if (global_gtl & 0xf0) {
    cout << "Entry length is: " << (global_gtl >> 4) << endl;
  }
  return sig_arc.ver;
}

/* Name:	print_arcs
 * Class:	None.
 * Purpose:	Prints arcs of the automaton.
 * Parameters:	inp	- (i) input stream,
 *		ver	- (i) automaton version,
 *		fsize	- (i) automaton file size,
 *		gtl	- (i) length of the goto field,
 *		entryl	- (i) length of the perfect hasing counter.
 * Returns:	Nothing.
 * Remarks:	None.
 */
void
print_arcs(ifstream &inp, const int ver, const long fsize)
{
  unsigned int tmp;
  int autom_size = fsize - 8;
  int flags_shift[] = {0, 0, 1, 0, 2, 3, 3, 4, 3, 2, 3, 3, 4};
  int gtl = global_gtl & 0xf;
  int entryl = (global_gtl >> 4) & 0xf;
  int stopf, nextf, tailf, finf;
  unsigned int gotof;
  const char *dict = new char[autom_size];
  
  // Read the automaton
  if (!(inp.read((char *)dict, autom_size))) {
    cerr << "Cannot read dictionary file";
    exit(2);
  }
  stopf = 0;
  long int start_loc = 0L;
  int alphabet_size = 0;
  const char *alphabet = NULL;
  unsigned char *char_num = NULL;
  if (ver > 8) {
    sparse_gtl = bytes2int((const unsigned char *)dict, 1);
//    min_char = dict[1];
//    max_char = dict[2];
//    int ml = (unsigned char)min_char;
    cout << "Pointer length in the sparse vector is: " << sparse_gtl
	 << endl;
//    cout << "Minimal label is `" << char(dict[1]) << "' ("
//	   << ml << ")" << endl;
//    ml = (unsigned char)max_char;
//    cout << "Maximal label is `" << char(dict[2]) << "' (" << ml
//	   << ")" << endl;
//    int mtl = (unsigned char)max_char - (unsigned char)min_char;
//    long int svec_entries = bytes2int((const unsigned char *)dict + 3,
//				      sparse_gtl + 1) - mtl;
//    cout << "Sparse vector has " << svec_entries << " entries." << endl;
//    long int annot_size = bytes2int((const unsigned char *)dict +
//				    sparse_gtl + 3,
//				    sparse_gtl);
//    cout << "Size of annotations is " << annot_size << " bytes." << endl;
    //const char *svl = dict + 2 * sparse_gtl + 3;
    long int svec_entries = bytes2int((const unsigned char *)dict + 1,
				      sparse_gtl + 1);
    alphabet_size = (unsigned char)(dict[sparse_gtl + 2]);
    svec_entries -= alphabet_size;
    alphabet = dict + sparse_gtl + 3;
    cout << "Alphabet:" << endl;
    for (int ai = 1; ai < alphabet_size; ai++) {
      cout << "[" << ai << "] `" << alphabet[ai] << "'" << endl;
    }
    cout << "Sparse matrix:" << endl;
    char_num = (unsigned char *)dict + sparse_gtl + 3 + alphabet_size;
    const char *svl = dict + sparse_gtl + alphabet_size + 259;
    //const char *svl = dict + sparse_gtl + 4;
    for (long int svi = 0; svi < svec_entries; svi++) {
      bool state_found = false;
      const char *tp = svl;
      for (int svti = 1; svti < alphabet_size; svti++) {
	if (*tp == alphabet[svti]) {
	  if (!state_found) {
	    state_found = true;
	    cout << "[" << svi << "] State" << endl;
	  }
	  cout << "  [" << svti << "] `" << *tp << "' ";
	  if (entryl) {
	    cout << "#" << bytes2int((const unsigned char *)tp + 1, entryl)
		 << " ";
	  }
	  int final = (tp[1 + entryl] & 1);
	  if (final) {
	    cout << ",f ";
	  }
	  long int gotof = bytes2int((const unsigned char *)tp + 1 + entryl,
				     sparse_gtl) >> 1;
	  cout << "-> " << gotof << endl;
	  cout << "  -----" << endl;
	}
	tp += 1 + entryl + sparse_gtl;
      }
      svl += 1 + entryl + sparse_gtl;
    }
    start_loc = svl - dict + alphabet_size * (1 + entryl + sparse_gtl);
  }
  for (long int curr_loc = start_loc; curr_loc < autom_size; ) {
    if (curr_loc - start_loc < (2 * (1 + gtl)))
      stopf = 1;

    // Handle numbers
    if (entryl && stopf) {
      int hash_count = bytes2int((const unsigned char *)dict + curr_loc,
				 entryl);
      for (int j = 0; j < entryl - 1; j++) {
	tmp = dict[curr_loc];
	cout << "[" << dec << curr_loc - start_loc << "] " << hex << tmp
	     << endl;
	curr_loc++;
      }
      tmp = dict[curr_loc];
      cout << "[" << dec << curr_loc - start_loc << "] " << hex << tmp
	   << " #v:" << dec << hash_count << endl;
      curr_loc++;
    }
    // Process transitions
    if ((ver >= 4 && ver <= 8) || (ver > 8)) {
      // Version compiled with STOPBIT; label is first
      cout << "[" << dec << curr_loc - start_loc << "] " << hex <<
	(((dict[curr_loc] & 0x7f) >= ' ') ? dict[curr_loc] : '?');
      if ((dict[curr_loc] & 0x7f) >= ' ') {
	unsigned char hc = dict[curr_loc];
	unsigned int hl = hc;
	cout << " (" << hex << hl << ")";
      }
      cout << endl;
      curr_loc++;
      if (ver == 8) {
	// Version with weight
	tmp = dict[curr_loc];
	cout << "[" << dec << curr_loc - start_loc << "] /" << dec << tmp
	     << endl;
	curr_loc++;
      }
      gotof = bytes2int((const unsigned char *)dict + curr_loc, gtl) >>
	flags_shift[ver];
      stopf = nextf = finf = tailf = 0;
      if (ver == 5 || ver == 7 || ver == 8 || ver == 10 || ver == 12) {
	nextf = ((dict[curr_loc] & 4) != 0);
      }
      stopf = ((dict[curr_loc] & 2) != 0);
      finf = ((dict[curr_loc] & 1) != 0);
      if (ver == 6 || ver == 11)
	tailf = ((dict[curr_loc] & 4) != 0);
      else if (ver == 7 || ver == 12)
	tailf = ((dict[curr_loc] & 8) != 0);
      // Print goto field with flags
      if (!nextf) {
	for (int i = 0; i < gtl - 1; i++) {
	  tmp = dict[curr_loc] & 0xff;
	  cout << "[" << dec << curr_loc - start_loc << "] " << hex << tmp
	       << endl;
	  curr_loc++;
	}
      }
      tmp = dict[curr_loc] & 0xff;
      cout << "[" << dec << curr_loc - start_loc << "] " << hex << tmp
	   << " -> (";
      if (nextf)
	cout << "*";
      else
	cout << dec << gotof;
      if (tailf)
	cout << ",t";
      if (nextf)
	cout << ",n";
      if (stopf)
	cout << ",s";
      if (finf)
	cout << ",f";
      cout << ")" << endl;
      curr_loc++;
      if (tailf) {
	int tailp = bytes2int((const unsigned char *)dict + curr_loc, gtl) >>
	  flags_shift[ver];
	for (int k = 0; k < gtl - 1; k++) {
	  tmp = dict[curr_loc];
	  cout << "[" << dec << curr_loc - start_loc << "] " << hex << tmp
	       << endl;
	  curr_loc++;
	}
	tmp = dict[curr_loc];
	cout << "[" << dec << curr_loc - start_loc << "] " << hex << tmp
	     << " >" << dec << tailp << endl;
	curr_loc++;
      }
      if (stopf) {
	cout << "=====" << endl;
      }
      else {
	cout << "-----" << endl;
      }
    }//if version with STOPBIT
  }//for all bytes
}//print_arcs

/* Name:	not_enough_memory
 * Class:	None.
 * Purpose:	Inform the user that there is not enough memory to continue
 *		and finish the program.
 * Parameters:	None.
 * Returns:	Nothing.
 * Remarks:	None.
 */
void
not_enough_memory(void)
{
  cerr << "Not enough memory\n";
  exit(5);
}//not_enough_memory


/* Name:	main
 * Class:	None.
 * Purpose:	Run the program.
 * Parameters:	argc	- (i) number of program parameters,
 *		argv	- (i) parameters.
 * Returns:	Exit code.
 * Remarks:	None.
 */
int
main(const int argc, const char *argv[])
{
  const char *dict_file_name;
  set_new_handler(&not_enough_memory);

  // handle options
  if (argc != 2) {
    usage(argv[0]);
    return 7;
  }
  else {
    dict_file_name = argv[1];
  }
  
  
  ifstream dict(dict_file_name, ios::in | /* ios::nocreate | */ ios::ate);
  if (dict.bad()) {
    cerr << "Cannot open dictionary file " << dict_file_name << "\n";
    return(FALSE);
  }
  // There is a bug in libstdc++ distributed in rpms.
  // This is a workaround (thanks to Arnaud Adant <arnaud.adant@supelec.fr>
  // for pointing this out).
  if (!dict.seekg(0,ios::end)) {
    cerr << "Seek on dictionary file failed. File is "
         << dict_file_name << "\n";
    return FALSE;
  }
  streampos file_size = dict.tellg();
  if (!dict.seekg(0L)) {
    cerr << "Seek on dictionary file failed." << endl;
    return 6;
  }
  int version = print_sig(dict, dict_file_name);
  print_arcs(dict, version, file_size);
  return 0;
}
