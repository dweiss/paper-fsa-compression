/***	visual_main.cc	***/

/*	Copyright (C) Jan Daciuk, 1999-2004	*/

/*

  This program visualizes automata (dictionaries) as graphs,
  or rather produces data for vcg program that performs the visualization.

  Dictionaries are automata produced by fsa_build or fsa_ubuild.

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
#include	"visualize.h"
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
  int		compressed = 0;	// whether the dictionary produced with -O

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
#ifdef STOPBIT
    else if (argv[arg_index][1] == 'O') {
      compressed = TRUE;
    }
#endif
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


  visual_fsa fsa_dict(&dict, compressed);
  if (!fsa_dict) {
    return fsa_dict.create_graphs();
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
       << "-i input file name\n"
       << "\t[default: standard input]\n"
#ifdef STOPBIT
       << "-O\t\t- (necessary for) dictionary produced with -O\n"
       << "\t\t\tnote that -O uses much more memory\n"
#endif
       << "-v\tversion details\n"
       << "Results printed on standard output\n"
       << "Only non compressed dictionaries can be handled!\n"
       << "(i.e. those produced without -O)\n";
  return 1;
}//usage

/***	EOF visual_main.cc	***/
