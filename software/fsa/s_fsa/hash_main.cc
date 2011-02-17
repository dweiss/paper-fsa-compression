/***	hash_main.cc	***/

/*	Copyright (C) Jan Daciuk, 1997-2004	*/

/*

This program translates words into corresponding numbers, and numbers to words.
It implements so called perfect hashing.
The translation is done using a dictionary of words prepared with fsa_build
or fsa_ubuild. The dictionary is in a form of a binary
automaton. At least one dictionary must be specified.

Words are read from standard input, and contents written to standard
output if not specified otherwise.
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
#include	"hash.h"
#include	"fsa_version.h"

static const int	MAX_LINE_LEN = 512;

int
main(const int argc, const char *argv[]);
int
usage(const char *prog_name);
void
not_enough_memory(void);



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
 * Remarks:	None.
 */
int
main(const int argc, const char *argv[])
{
  word_list	dict;		// dictionary file name 
  word_list	inputs;		// names of input files (if any)
#ifdef FLEXIBLE
#ifdef NUMBERS
  int		arg_index;	// current argument number
  const char	*lang_file = NULL; // name of file with character set
  direction_t	direction = unspecified;
#endif
#endif

  set_new_handler(&not_enough_memory);

#ifndef NUMBERS
  cerr << argv[0] << ": This program MUST be compiled with NUMBERS" << endl;
  return 1;
#else
#ifdef FLEXIBLE
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
    else if (argv[arg_index][1] == 'N') {
      direction = words_to_numbers;
    }
    else if (argv[arg_index][1] == 'W') {
      direction = numbers_to_words;
    }
    else if (argv[arg_index][1] == 'v') {
      // details of version
#include "compile_options.h"
      return 0;
    }
    else {
      cerr << argv[0] << ": unrecognized option\n";
      return usage(argv[0]);
    }
  }//for

  if (dict.how_many() == 0) {
    cerr << "No dictionary specified" << endl;
    return usage(argv[0]);
  }

  if (direction == unspecified) {
    cerr << "Either -N or -W must be specified" << endl;
    return usage(argv[0]);
  }

  hash_fsa fsa_dict(&dict, lang_file);
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
	fsa_dict.hash_file(io_obj, direction);
      } while (inputs.next());
      return 0;
    }
    else {
      tr_io io_obj(&cin, cout, MAX_LINE_LEN, "",
#ifdef UTF8
		   (word_syntax_type *)
#endif
		   fsa_dict.get_syntax());
      return fsa_dict.hash_file(io_obj, direction);
    }
  }
  else
    return fsa_dict;
#else
  cerr << "You specified -DNUMBERS, but not -DFLEXIBLE. Specify -DFLEXIBLE\n";
  return 1;
#endif
#endif
}//main

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
       << "-i input file name (multiple files allowed)\n"
       << "\t[default: standard input]\n"
       << "-l language_file\t- file that defines characters allowed in words\n"
       << "\tand case conversions\n"
       << "\t[default: ASCII letters, standard conversions]\n"
       << "-N\ttranslate words to numbers (either this or -W must be given)\n"
       << "-W\ttranslate numbers to words (either this or -N must be given)\n"
       << "-v\tversion details\n"
       << "Standard output used for displaying results.\n"
       << "At least one dictionary must be present.\n";
  return 1;
}//usage

/***	EOF hash_main.cc	***/
