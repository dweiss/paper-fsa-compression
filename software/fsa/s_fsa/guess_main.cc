/***	guess_main.cc	***/

/*	Copyright (C) Jan Daciuk, 1996-2004	*/

/*
   This program tries to guess a category of a word based on its ending.
   It uses a dictionary of word endings in form of an automaton.

*/

#include	<iostream>
#include	<fstream>
#include	<string.h>
#include	<stdlib.h>
#include	<new>
#include	<unistd.h>
#ifdef DMALLOC
#include	"dmalloc.h"
#endif
#include	"fsa.h"
#include	"nstr.h"
#include	"common.h"
#include	"guess.h"
#include	"fsa_version.h"

static const int	MAX_LINE_LEN = 512;	// Buffer length for input

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
 *		4	- not enough memory;
 * Remarks:	None.
 */
int
main(const int argc, const char *argv[])
{
  word_list	dict;		// dictionary file name 
  int		arg_index;	// current argument number
  word_list	inputs;		// names of input files (if any)
  const char	*lang_file = NULL; // name of file with character set
  int		guess_lexemes = TRUE; // whether dictionary contains lexemes
  int		guess_prefix = FALSE; // whether dictionary contains prefixes
  int		guess_infix = FALSE; // whether dictionary contains infixes
  int		guess_mmorph = FALSE; // whether dictionary contains
                                      // morphological descriptions 

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
#ifdef GUESS_LEXEMES
    else if (argv[arg_index][1] == 'g') {
      // Dictionary contains only categories
      guess_lexemes = FALSE;
    }
    else if (argv[arg_index][1] == 'A') {
      // Dictionary contains only categories
      guess_lexemes = FALSE;
    }
#endif
#ifdef GUESS_PREFIX
    else if (argv[arg_index][1] == 'p') {
      // Dictionary does not contain prefixes
      guess_prefix = FALSE;
    }
    else if (argv[arg_index][1] == 'P') {
      // Dictionary contains prefixes
      guess_prefix = TRUE;
    }
    else if (argv[arg_index][1] == 'I') {
      // Dictionary contains infixes
      guess_infix = TRUE;
    }
#endif
#ifdef GUESS_MMORPH
    else if (argv[arg_index][1] == 'm') {
      // guess morphological descriptions for mmorph
      guess_mmorph = TRUE;
    }
#endif
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
      // version details
#include "compile_options.h"
      return 0;
    }
    else {
      cerr << argv[0] << ": unrecognized option\n";
      return usage(argv[0]);
    }
  }//for

  if (dict.how_many() == 0) {
    cerr << argv[0] << ": at least one dictionary file must be specified\n";
    return usage(argv[0]);
  }

  if (!guess_lexemes && guess_infix) {
    guess_infix = FALSE;
    guess_prefix = TRUE;
  }

  guess_fsa fsa_dict(&dict, guess_lexemes, guess_prefix, guess_infix,
		     guess_mmorph, lang_file);
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
	fsa_dict.guess_file(io_obj);
      } while (inputs.next());
      return 0;
    }
    else {
      tr_io io_obj(&cin, cout, MAX_LINE_LEN, "",
#ifdef UTF8
		   (word_syntax_type *)
#endif
		   fsa_dict.get_syntax());
      return fsa_dict.guess_file(io_obj);
    }
  }
  else
    return fsa_dict;
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
#ifdef GUESS_LEXEMES
       << "-A\t- make the program behave as if it were compiled without "
       << "GUESS_LEXEMES\n"
       << "(i.e. do not guess lexemes)\n"
       << "-g\t- make the program behave as if it were compiled without "
       << "GUESS_LEXEMES\n"
       << "(now obsolete, same as -A)\n"
#endif
#ifdef GUESS_PREFIX
       << "-p\t- make the program behave as if it were compiled without "
       << "GUESS_PREFIX\n"
       << "(i.e. do not try to recognize prefixes). This is now the default\n"
       << "-P\t- recognize prefixes\n"
       << "-I\t- recognize infixes and prefixes\n"
#endif
#ifdef GUESS_MMORPH
       << "-m\t- guess mmorph descriptions\n"
#endif
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

/***	EOF guess_main.cc	***/
