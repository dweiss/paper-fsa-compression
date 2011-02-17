/***	prefix_main.cc	***/

/*	Copyright (C) Jan Daciuk, 1996-2004	*/

/*

This program display contents of a dictionary.

Synopsis:
fsa_prefix [-a] -d dictionary [-d dictionary]...
fsa_prefix -v

where dictionary is a file containing the dictionary in a form of a binary
automaton. At least one dictionay must be specified.

Words are read from standard input, and contents written to standard
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
#include	"prefix.h"
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
  int		arg_index;	// current argument number
  word_list	inputs;		// names of input files (if any)
  const char	*lang_file = NULL; // name of file with character set
  bool		dump_all = false;

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
    else if (argv[arg_index][1] == 'a') {
      // dump All contents
      dump_all = true;
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

  prefix_fsa fsa_dict(&dict, lang_file);
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
	if (dump_all) {
	  fsa_dict.complete_prefix("", io_obj);
	}
	else {
	  fsa_dict.complete_file_words(io_obj);
	}
      } while (inputs.next());
      return 0;
    }
    else {
      tr_io io_obj(&cin, cout, MAX_LINE_LEN, "",
#ifdef UTF8
		   (word_syntax_type *)
#endif
		   fsa_dict.get_syntax());
      if (dump_all) {
	return (fsa_dict.complete_prefix("", io_obj) == 0);
      }
      else {
	return fsa_dict.complete_file_words(io_obj);
      }
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
       << "-i input file name (multiple files allowed)\n"
       << "\t[default: standard input]\n"
       << "-l language_file\t- file that defines characters allowed in words\n"
       << "\tand case conversions\n"
       << "\t[default: ASCII letters, standard conversions]\n"
       << "-v\tversion details\n"
       << "Standard output used for displaying results.\n"
       << "At least one dictionary must be present.\n";
  return 1;
}//usage

/***	EOF prefix_main.cc	***/
