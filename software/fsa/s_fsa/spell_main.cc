/***	spell_main.cc	***/

/*	Copyright (C) Jan Daciuk, 1996-2004	*/

/*

This program corrects spelling mistakes (when possible) by proposing
a list of replacements for words not found in a dictionary.

Synopsis:
fsa_spell [-d dictionary]... [-e edit_distance] [-d dictionary]...

where dictionary is a file containing the dictionary in a form of a binary
automaton, and edit_distance is a number that says how many basic editing
operations may be performed to transform an input string into a word
that is present in the dictionary. Default edit distance is 1.
At least one dictionay must be specified.

Words are read from standard input, and replacements written to standard
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
#include	"spell.h"
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
  word_list	dict;		// dictionary file names
  int		distance = 1;	// edit distance
  int		arg_index;	// current argument number
  word_list	inputs;		// names of input files (if any)
  const char	*lang_file = NULL; // name of file with character set
  const char	*chclass_file = NULL; // name of file with character classes
  bool		force = false;	// force research of replacements candidates

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
    else if (argv[arg_index][1] == 'e') {
      // edit distance
      if (++arg_index >= argc)
	return usage(argv[0]);
      distance = atoi(argv[arg_index]);
      if (distance < 0 || distance > Max_edit_distance) {
	cerr << "You're kidding. Edit distance must be from 0 to "
	  << Max_edit_distance << endl;
	distance = 1;
      }
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
    else if (argv[arg_index][1] == 'r') {
      // character class file name
#ifdef CHCLASS
      if (++arg_index >= argc)
	return usage(argv[0]);
      chclass_file = argv[arg_index];
#else
      cerr << "Recompile with CHCLASS compile option to use -r here" << endl;
#endif
    }
    else if (argv[arg_index][1] == 'v') {
      // details of version
#include "compile_options.h"
      return 0;
    }
    else if (argv[arg_index][1] == 'f') {
      force = true;
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

  spell_fsa fsa_dict(&dict, distance, chclass_file, lang_file);
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
	fsa_dict.spell_file(distance, force, io_obj);
      } while (inputs.next());
      return 0;
    }
    else {
      tr_io io_obj(&cin, cout, MAX_LINE_LEN, "",
#ifdef UTF8
		   (word_syntax_type *)
#endif
		   fsa_dict.get_syntax());
      return fsa_dict.spell_file(distance, force, io_obj);
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
       << "-d dictionary\t\t- automaton file (multiple files allowed)\n"
       << "-e edit_distance\t- max number of basic editing operations\n"
       << "\t\t\t  [deafult: 1]\n"
       << "-i input_file\t\t- input file name (multiple files allowed)\n"
       << "\t\t\t  [default: standard input]\n"
       << "-l language_file\t- file that defines characters allowed in words\n"
       << "\t\t\t  and case conversions\n"
       << "\t\t\t  [default: ASCII letters, standard conversions]\n"
       << "-r char_class_file\t- file that specifies which pairs of "
       << "characters\n"
       << "\t\t\t  can be treated as as single characters\n"
       << "\t\t\t  [default: no pairs treated as single characters]\n"
       << "-f\t\t\t- force search for replacement candidates\n"
       << "-v version details\n"
       << "Standard output is used for displaying results.\n"
       << "At least one dictionary must be present.\n";
  return 1;
}//usage

/***	EOF spell_main.cc	***/
