/***	accent_main.cc	***/

/*	Copyright (C) Jan Daciuk, 1996-2004	*/

/*

This program looks for words from standard input, and tries to find
morphological information for that words in speficied dictionaries.

Synopsis:
fsa_accent options

options are:
[-d dictionary]...
-a accent_file

where dictionary is a file containing the dictionary in a form of a binary
automaton. At least one dictionay must be specified.
Accent file contains relations between the accented characters, and their latin
equivalents.

Words are read from standard input, and results written to standard
output.

*/

#include	<iostream>
#include	<fstream>
#include	<string.h>
#include	<stdlib.h>
#include	<new>
#include	<unistd.h>
#include	"fsa.h"
#include	"nstr.h"
#include	"common.h"
#include	"accent.h"
#include	"fsa_version.h"

static const int	Buf_len = 256;	// Buffer length for accent file
static const int	MAX_LINE_LEN = 512;	// Buffer length for input
#ifdef UTF8
const int		acc_tab_len = 512;	// Accent table length
const unsigned int	MAX_DSTR = 128;		// Max # chars with diacritics
						// for one without them
#endif

using namespace std;

int
main(const int argc, const char *argv[]);
int
usage(const char *prog_name);
void
not_enough_memory(void);
accent_tabs *
make_accents_table(const char *file_name);




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
  cerr << "Not enough memory for the automaton\n";
  exit(4);
}//not_enough_memory

/* Name:	main
 * Class:	None.
 * Purpose:	Launches the program.
 * Parameters:	argc		- (i) number of program arguments;
 *		argv		- (i) program arguments;
 * Returns:	Program exit code:
 *		0	- OK;
 *		1	- invalid options;
 *		2	- dictionary file could not be opened;
 *		4	- not enough memory;
 *		5	- accent file non-existant or cannot be used.
 * Remarks:	None.
 */
int
main(const int argc, const char *argv[])
{
  word_list	dict;		// dictionary file name 
  int		arg_index;	// current argument number
  const char	*accent_file_name = NULL;// relations between characters
  accent_tabs	*accents;	// accent equivalence table
  word_list	inputs;		// names of input files (if any)
  const char 	*lang_file = NULL; // name of file with character set

  set_new_handler(&not_enough_memory);

  for (arg_index = 1; arg_index < argc; arg_index++) {
    if (argv[arg_index][0] != '-')
      // not an option
      return usage(argv[0]);
    if (argv[arg_index][1] == 'd') {
      // dictionary file name
      if (++arg_index >= argc)
	return usage(argv[0]);
      dict.insert(argv[arg_index]);
    }
    else if (argv[arg_index][1] == 'a') {
      // file that contains relations between accented characters and their
      // latin equivalents
      if (++arg_index >= argc)
	return usage(argv[0]);
      accent_file_name = argv[arg_index];
    }
    else if (argv[arg_index][1] == 'i') {
      // input file name
      if (++arg_index >= argc)
	return usage(argv[0]);
      inputs.insert(argv[arg_index]);
    }
    else if (argv[arg_index][1] == 'l') {
      // language file name
      if (++arg_index >= argc)
	return usage(argv[0]);
      lang_file = argv[arg_index];
    }
    else if (argv[arg_index][1] == 'v') {
#include "compile_options.h"
      return 0;
    }
    else {
      cerr << argv[0] << ": unrecognized option\n";
      return usage(argv[0]);
    }
  }//for

  if (accent_file_name == NULL) {
    cerr << "Accent file name not specified\n";
    return usage(argv[0]);
  }

  if (dict.how_many() == 0) {
    cerr << argv[0] << ": at least one dictionary file must be specified\n";
    return usage(argv[0]);
  }

  if ((accents = make_accents_table(accent_file_name)) == NULL)
    // could not make accent table (message already printed in funtion)
    return 5;

  accent_fsa fsa_dict(&dict, lang_file);
  if (!fsa_dict) {
    if (inputs.how_many()) {
      inputs.reset();
      do {
	ifstream iff(inputs.item());
	tr_io io_obj(&iff, cout, MAX_LINE_LEN, inputs.item(),
#ifdef UTF8
		     (word_syntax_type *)
#endif
		     fsa_dict.get_syntax());
	fsa_dict.accent_file(io_obj, accents);
      } while (inputs.next());
      return 0;
    }
    else {
      tr_io io_obj(&cin, cout, MAX_LINE_LEN, "",
#ifdef UTF8
		   (word_syntax_type *)
#endif
		   fsa_dict.get_syntax());
      return fsa_dict.accent_file(io_obj, accents);
    }
  }
  else
    return fsa_dict;
}//main

#if defined(UTF8) && !(defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE))
/* Name:	fill_diacritics
 * Class:	None.
 * Purpose:	Fills the accent table with a tree of UTF character strings
 *		formed from characters that have the same graphical skeleton,
 *		but differ by diacritics.
 * Parameters:	strings_to_fit	- (i) a vector with strings representing
 *					each a UTF8 character string of
 *					a UTF8 character with diacritics;
 *		first_char	- (i) first string in strings_to_fit
 *					to be examined;
 *		last_char	- (i) last string in strings_to_fit
 *					to be examined;
 *		char_index	- (i) which character in strings to examine;
 *		origin		- (i) where to start fitting a node.
 * Returns:	The start of the node.
 * Remarks:	The UTF8 characters are strings of 8-bit characters.
 *		They may have common prefixes.
 *		The vector strings_to_fit is already sorted.
 *		It is assumed that the first char_index 8-bit characters
 *		of those strings are already processed, and they already
 *		form a tree in word_syntax.
 *		The first thing to do is to find what are the distinct
 *		bytes at char_index position of the strings. In other words,
 *		what are the labels on branches going out from the node
 *		to be constructed.
 */
int
fill_diacritics(accent_tab_type *acc_tab,
		unsigned char **strings_to_fit,
		unsigned int first_char, unsigned int last_char,
		unsigned int char_index, unsigned int &origin)
{
  unsigned int alsp = 0;
  unsigned char als[256];
  unsigned char prev;
  unsigned int acc_tab_current_l = acc_tab_len;
  for (unsigned int rp = first_char; rp <= last_char; ) {
    // Put into als the set of characters on branches of the current
    // subtree of the alphabet strings
    prev = als[alsp++] = strings_to_fit[rp++][char_index];
    while (rp <= last_char && strings_to_fit[rp][char_index] == prev) {
      rp++;
    }
  }
  // Fit the set inside the sparse vector
  unsigned int b = origin;
  bool found_fit = false;
  do {
    found_fit = true;
    if (b + als[alsp-1] >= acc_tab_current_l) {
      // Reallocate memory
      accent_tab_type *new_acc = new accent_tab_type[2 * acc_tab_current_l];
      // Copy old contents
      memcpy(new_acc, acc_tab, acc_tab_current_l * sizeof(accent_tab_type));
      // Clear new entries
      memset((char *)new_acc + acc_tab_current_l * sizeof(accent_tab_type),
	     0,  acc_tab_current_l * sizeof(accent_tab_type));
      // Set new accent table, delete old memory
      delete [] acc_tab;
      acc_tab = new_acc; new_acc = NULL;
      acc_tab_current_l *= 2;	// we have now so much of it
    }
    // Find a space that isn't blocked by other nodes
    for (unsigned int j = 0; j < alsp; j++) {
      if (acc_tab[b + als[j]].chr != '\0') {
	found_fit = false;
	b++;
	break;
      }
    }
  } while (!found_fit);
  // ---------------------------------------------------------------------------
  // Reserve the branches for the current node
  for (unsigned int j = 0; j < alsp; j++) {
    acc_tab[b + als[j]].chr = als[j];
  }
  origin = b + 1;
  // Create subtrees
  unsigned int i = first_char;
  int k = i;
  for (unsigned int j = 0; j < alsp; j++) {
    // Find which strings in the range share the same character at char_index
    // position
    k = i;
    while (i <= last_char && strings_to_fit[i][char_index] == als[j]) {
      i++;
    }
    if (strings_to_fit[k][char_index+1] &&
	utf8t(strings_to_fit[k][char_index+1]) == CONTBYTE) {
      // Create one subtree
      acc_tab[b + als[j]].follow = fill_diacritics(acc_tab, strings_to_fit,
						   k, i - 1, char_index + 1,
						   origin);
    }
    else {
      // This is a leaf (the string ends here)
      acc_tab[b + als[j]].follow = -1;
    }
  }
  return b;
}//fill_diacritics

/* Name:	fill_acc_tab
 * Class:	None.
 * Purpose:	Fills the accent table with a tree of UTF8 character byte
 *		strings.
 * Parameters:	acc_tab		- (i/o) the accent table to fill;
 *		strings_to_fit	- (i) a vector with strings representing
 *					one UTF8 character without diacritics,
 *					spaces and a string of UTF8 characters
 *					(byte strings) with diacritics.
 *		first_char	- (i) first string in strings_to_fit
 *					to be examined;
 *		last_char	- (i) last string in strings_to_fit
 *					to be examined;
 *		char_index	- (i) which character in strings to examine;
 *		origin		- (i) where to start fitting a node.
 * Returns:	The start of the node.
 * Remarks:	The UTF8 characters are strings of 8-bit characters.
 *		They may have common prefixes.
 *		The vector strings_to_fit is already sorted.
 *		It is assumed that the first char_index 8-bit characters
 *		of those strings are already processed, and they already
 *		form a tree in word_syntax.
 *		The first thing to do is to find what are the distinct
 *		bytes at char_index position of the strings. In other words,
 *		what are the labels on branches going out from the node
 *		to be constructed.
 *		It is assumed that for i=first_char..last_char
 *		strings_to_fit[i][char_index] != ' '.
 */
int
fill_acc_tab(accent_tab_type *acc_tab,
	     unsigned char **strings_to_fit,
	     unsigned int first_char, unsigned int last_char,
	     unsigned int char_index, unsigned int &origin)
{
  unsigned int alsp = 0;
  unsigned char als[256];
  unsigned char prev;
  unsigned int acc_tab_current_l = acc_tab_len;
  for (unsigned int rp = first_char; rp <= last_char; ) {
    // Put into als the set of characters on branches of the current
    // subtree of the alphabet strings
    prev = als[alsp++] = strings_to_fit[rp++][char_index];
    while (rp <= last_char && strings_to_fit[rp][char_index] == prev) {
      rp++;
    }
  }
  // Fit the set inside the sparse vector
  unsigned int b = origin;
  bool found_fit = false;
  do {
    found_fit = true;
    if (b + als[alsp-1] >= acc_tab_current_l) {
      // Reallocate memory
      accent_tab_type *new_acc = new accent_tab_type[2 * acc_tab_current_l];
      // Copy old contents
      memcpy(new_acc, acc_tab, acc_tab_current_l * sizeof(accent_tab_type));
      // Clear new entries
      memset((char *)new_acc + acc_tab_current_l * sizeof(accent_tab_type),
	     0,  acc_tab_current_l * sizeof(accent_tab_type));
      // Set new accent table, delete old memory
      delete [] acc_tab;
      acc_tab = new_acc; new_acc = NULL;
      acc_tab_current_l *= 2;	// we have now so much of it
    }
    // Find a space that isn't blocked by other nodes
    for (unsigned int j = 0; j < alsp; j++) {
      if (acc_tab[b + als[j]].chr != '\0') {
	found_fit = false;
	b++;
	break;
      }
    }
  } while (!found_fit);
  // ---------------------------------------------------------------------------
  // Reserve the branches for the current node
  for (unsigned int j = 0; j < alsp; j++) {
    acc_tab[b + als[j]].chr = als[j];
  }
  origin = b + 1;
  // Create subtrees
  unsigned int i = first_char;
  int k = i;
  for (unsigned int j = 0; j < alsp; j++) {
    // Find which strings in the range share the same character at char_index
    // position
    k = i;
    while (i <= last_char && strings_to_fit[i][char_index] == als[j]) {
      i++;
    }
    if (strings_to_fit[k][char_index+1] != ' ' &&
	strings_to_fit[k][char_index+1]) {
      // Create one subtree
      acc_tab[b + als[j]].follow = fill_acc_tab(acc_tab, strings_to_fit,
						k, i - 1, char_index + 1,
						origin);
    }
    else if (strings_to_fit[k][char_index+1]) {
      // This is a leaf of unaccented UTF8 character
      // Here first_char = last_char or there is an error in accent file
      // The next character on the input should be a space
      // The space is followed by a series of accented UTF8 characters
      acc_tab[b + als[j]].follow = -1;
      unsigned int ci = char_index + 1;
      // Skip spaces
      while (strings_to_fit[k][ci] == ' ') ci++;
      // Create subtrees for diacritics
      // We cannot process the characters with diacritics one by one
      // as placing the tree in the sparse matrix requires
      // that all labels of arcs going out from a node must be known
      // We need pointers to all UTF8 character strings
      unsigned char *dstr[MAX_DSTR];
      unsigned int dp = 0;
      for (unsigned char *d = strings_to_fit[k] + ci; *d; ) {
	if (dp < MAX_DSTR) {
	  dstr[dp++] = d;
	  d += utf8len(*d);
	}
	else {
	  std::cerr << "fsa_accent: too many characters with diacritics\n"
		    << "for a character without them. Increase MAX_DSTR\n"
		    << "in accent_main.cc and recompile.\n";
	  exit(5);
	}
      }
      qsort(dstr, dp, sizeof(unsigned char *), my_cmp_str_ndx);
      acc_tab[b + als[j]].repl = fill_diacritics(acc_tab, dstr, 0, dp - 1, 0,
						 origin);
    }
    else {
      std::cerr << "fsa_accent: error in accent file format\n";
      exit(5);
    }
  }
  return b;
}//fill_acc_tab
#endif

/* Name:	make_accents_table
 * Class:	None.
 * Purpose:	Read file with accents and prepare accent table.
 * Parameters:	a_file		- (i) name of file with accent descriptions.
 * Returns:	Accent table.
 * Remarks:	The format of the accent file is as follows:
 *			The first character in the first line is a comment
 *			character. Each line that begins with that character
 *			is a comment.
 *			Note: it is usually `#' or ';'.
 *
 *			The first character of non-comment lines is a character
 *			without diacritics. Then, after one or more spaces,
 *			follow characters with diacritics. There are no spaces
 *			between them; they (spaces) indicate the end
 *			of definition.
 *			Characters are represented by themselves, the file
 *			is binary. HT can be defined as accented character.
 *		The format of the accent table:
 *			The table contains 256 characters of codes 0-255.
 *
 *			If a character contains a diacritic, it is represented
 *			by the equivalent latin character.
 *
 *			All other characters are represented by themselves.
 */
accent_tabs *
make_accents_table(const char *file_name)
{
#if defined(UTF8) && !(defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE))
  const int	max_line_buf =	128;
#else
  const int	acc_tab_len =	256;
#endif
  int		i;
  char		comment_char;
  char		junk;
  unsigned char	buffer[Buf_len];
#if defined(UTF8) && !(defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE))
  unsigned char	**line_buf;
  unsigned int	line_buf_len;
  unsigned int	lines = 0;
#else
  char		accented;
#endif

  // Open accent file
  ifstream acc_f(file_name, ios::in /*| ios::nocreate */);
  if (acc_f.bad()) {
    cerr << "Cannot open accent file `" << file_name << "'\n";
    return NULL;
  }
  // Allocate memory for the accent table
#if defined(UTF8) && !(defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE))
  line_buf = new unsigned char *[line_buf_len = max_line_buf];
  accent_tab_type *acc_tab = new accent_tab_type[acc_tab_len];
#else
  char *acc_tab = new char[acc_tab_len];
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
  char **cheq = new char *[acc_tab_len];
#endif
#endif
  // Initialize the accent table
  for (i = 0; i < acc_tab_len; i++) {
#if defined(UTF8) && !(defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE))
    acc_tab[i].chr = '\0'; acc_tab[i].follow = -1; acc_tab[i].repl = -1;
#else
    acc_tab[i] = i;
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
    cheq[i] = NULL;
#endif
#endif
  }
  // Read the first line to set the comment character
  if (acc_f.get((char *)buffer, Buf_len, '\n'))
    comment_char = buffer[0];
  else {
    delete [] acc_tab;
    acc_tab = NULL;
    return NULL;
  }
  acc_f.get(junk);
  // Read subsequent lines
  while (acc_f.get((char *)buffer, Buf_len, '\n')) {
    acc_f.get(junk);
#if defined(UTF8) && !(defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE))
    // We must collect those first, unaccented characters
    // before we can stored them in the sparsed matrix
    if (buffer[0] != comment_char) {
      if (lines >= line_buf_len) {
	// Allocate more memory
	unsigned char **new_lines = new unsigned char *[line_buf_len * 2];
	// Copy the old contents to the new location
	for (unsigned int z = 0; z < line_buf_len; z++) {
	  new_lines[z] = line_buf[z];
	}
	// Swap pointers
	delete [] line_buf; line_buf = new_lines;
      }
      line_buf[lines++] = (unsigned char *)nstrdup((const char *)buffer);
    }
#else
    if ((accented = buffer[0]) != comment_char) {
      // Skip blanks between the character with diacritics
      // and characters that have it as a base, but contain diacritics
      for (i = 1; buffer[i] == ' '; i++)
	;
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
      // Note: must check that - not the whole buffer
      int lb = strlen((char *)buffer);
      unsigned char xc = (unsigned char)accented;
      cheq[xc] = new char[lb + 2];
      strcpy(cheq[xc] + 1, (char *)buffer);
      cheq[xc][0] = xc;
#endif
      for (; buffer[i] != '\0' && buffer[i] != ' '; i++) {
	// Assign a diacritic-less character to its diacritic-rich equivalent
	acc_tab[buffer[i]] = accented;
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
	cheq[buffer[i]] = cheq[xc];
#endif
      }
    }
#endif //UTF8
  }
#if defined(UTF8) && !(defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE))
  // We now have meaningful lines stored  in line_buf
  // Each line contains one UTF8 character without diactrictics
  // some spaces and more characters with diacritics.
  // The lines are not sorted - let's sort them
  qsort(line_buf, lines, sizeof(char *), my_cmp_str_ndx);
  unsigned int base = 0;
  fill_acc_tab(acc_tab, line_buf, 0, lines - 1, 0, base);
  accent_tabs *acc_struct = acc_tab;
#else
  accent_tabs *acc_struct = new accent_tabs;
  acc_struct->accents = acc_tab;
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
  acc_struct->eqchs = cheq;
#else
  acc_struct->eqchs = NULL;
#endif
#endif
  return acc_struct;
}//make_accents_table
      

/* Name:	usage
 * Class:	None.
 * Purpose:	Prints program synopsis.
 * Parameters:	prog_name	- (i) program name.
 * Returns:	1.
 * Remarks:	None.
 */
int
usage(const char *prog_name)
{
  cerr << "Usage:\n" << prog_name << " [options]...\n"
       << "Options:\n"
       << "-d dictionary\t- automaton file (multiple files allowed)\n"
       << "-a accent_file\t- file with relations between characters\n"
       << "-i input file name (multiple files allowed)\n"
       << "\t[default: standard input]\n"
       << "-l language_file\t- file that defines characters allowed in words\n"
       << "\tand case conversions\n"
       << "\t[default: ASCII letters, standard conversions]\n"
       << "-v version details\n"
       << "Standard output is used for displaying results.\n"
       << "At least one dictionary must be present.\n";
  return 1;
}//usage

/***	EOF accent_main.cc	***/
