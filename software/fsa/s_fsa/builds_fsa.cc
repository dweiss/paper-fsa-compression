/***	builds_fsa	***/

/*	Copyright (C) Jan Daciuk, 1999-2004	*/

/*
  This file contains functions particular to fsa_build.
*/

#include	<iostream>
#include	<fstream>
#include	<stddef.h>
#include	<string.h>
#include	<stdlib.h>
#include	<new>
#include	<unistd.h>
#include	"fsa.h"
#include	"nnode.h"
#include	"nstr.h"
#include	"nindex.h"
#include	"fsa_version.h"
#include	"build_fsa.h"


/* holds information about prefix of word already in automaton */
struct prefix {
  node		*end_node;	// last node of automaton constaining word
  int		length;		// length of prefix
  int		arc_exists;	// if there is an arc from end_node to empty
                                // node labelled with appropriate letter
};/*prefix*/

/* Name:	find_common_prefix
 * Class:	None.
 * Purpose:	Find how many letters of the word just read are already in
 *		the automaton. Find the node that contains the last letter.
 * Parameter:	start_node	- (i) where to start in the automaton;
 *		word		- (i) word whose common prefix is sought;
 *		common_prefix	- (o) information about the prefix;
 *		length		- (i) how many characters of word were
 *					already considered.
 * Returns:	common_prefix length.
 * Remarks:	I had to add `length' parameter, beacuse that was the only
 *		way to avoid using a global variable (I hate doing that).
 */
int
find_common_prefix(node *start_node,
		   const char *word,
		   prefix *common_prefix,
		   const int length)
{
  int		modif = 0;
  arc_node	*offspr;

  for (int i = 0; i < start_node->get_no_of_kids(); i++) {
    offspr = start_node->get_children() + i;
    if (offspr->letter == *word) {
      if (offspr->child) {
	return find_common_prefix(offspr->child, word + 1, common_prefix,
				  length + 1);
      }
      else {
	modif = 1;
	break;
      }
    }
  }
  common_prefix->end_node = start_node;
  return common_prefix->length = length /*+ modif*/;
}//find_common_prefix


/* Name:	build_fsa
 * Class:	automaton
 * Purpose:	Reads a list of sorted words and builds a final state
 *		automaton that recognizes them.
 * Parameters:	infile	- (i) file to be read.
 * Returns:	TRUE if automaton built, FALSE otherwise.
 * Remarks:	Words need to be sorted. Duplicates are not allowed.
 */
int
automaton::build_fsa(istream &infile)
{
  static char	*word_buffer;
  int		allocated = WORD_BUFFER_LENGTH;
  char		*word;
  char		*rest;
  prefix	common_prefix;
  node		*last_node = NULL;
  node		*new_node;
  char		junk;
#ifdef PROGRESS
  int		line_no = 0;
#endif

  word_buffer = new char[WORD_BUFFER_LENGTH];
  word = &word_buffer[0];
  while (infile.get(word, allocated, '\n')) {
    infile.get(junk);				// eat '\n'
    while (junk != '\n') {
      // line was too long, allocate more space
      word = grow_string(word_buffer, allocated, WORD_BUFFER_LENGTH);
      word[allocated - WORD_BUFFER_LENGTH - 1] = junk;
      if (!(infile.get(word + allocated - WORD_BUFFER_LENGTH,
		       WORD_BUFFER_LENGTH, '\n'))) {
#ifdef DEBUG
	cerr << "get() failed! good()=" << infile.good()
	     << ", fail()=" << infile.fail()
	     << ", bad()=" << infile.bad()
	     << ", eof()=" << infile.eof() << "\n";
#endif
	// This horrible code is provided as an attempt to counteract recent
	// absurd I/O library behaviour
	if (!(infile.bad()) && !(infile.eof())) {
	  // Last "junk" was probably the last character of the line.
	  // Were we to omit this, the next get() would fail!!!
	  infile.clear();
	  infile.get(junk);	// There should still be EOLN in the buffer!
	}
	break;
      }
      infile.get(junk);
    }
#ifdef DEBUG
    cerr << "`" << word << "'\n";
#endif
#ifdef PROGRESS
    if ((line_no++ & 0x03FF) == 0)
      cerr << (line_no - 1) << " lines processed" << endl;
#endif
    // the rest is word - common_prefix
    rest = word + find_common_prefix(root, word, &common_prefix, 0);
    last_node = common_prefix.end_node;
#ifdef DEBUG
    cerr << "Prefix len: " << common_prefix.length << ", rest: `" << rest
         << "'\n";
#endif

    if (last_node->fertile()) {
      // We are retreating in the graph. This means that the last child
      // and possibly its children of the last node in prefix are
      // to be either registered (if there is no isomorphic subgraph
      // in the automaton, or deleted (i.e. replaced by pointers to
      // subgraphs already registered).
#ifdef DEBUG
      cerr << "About to call compress_or_register\n";
#endif
      common_prefix.end_node->compress_or_register();
    }

    // If the rest of word (after prefix) exists make a chain of nodes
    // with its consecutive letters, and attach it to the last node
    // found in prefix.
    if (*rest) {
      new_node = common_prefix.end_node->add_postfix(rest);
    }
  }//while

  root->compress_or_register();
#ifdef PROGRESS
  cerr << line_no << " lines processed. Input read." << endl;
#endif
#ifdef DEBUG
  show_index(PRIM_INDEX);
#endif
  return TRUE;
}//build_fsa


/* Name:	usage
 * Class:	None.
 * Purpose:	Prints program synopsis.
 * Parameters:	prog_name	- (i) program name.
 * Returns:	Nothing.
 * Remarks:	None.
 */
void
usage(const char *prog_name)
{
  cerr << prog_name
       << " builds an automaton. Synopsis:\n"
       << prog_name << " [options]\nOptions:\n-O\toptimization\n"
       << "-i input_file\t input file name (or else use stdin)\n"
       << "-o output_file\t output file name (or else use stdout)\n"
       << "-A as\tannotation separator character (use with -X)\n"
       << "\t\t[default:+]\n"
       << "-F fc\tfiller character (use with -X)\n"
       << "\t\t[default:_]\n"
#ifdef A_TERGO
       << "-X\tmake index a tergo (for guessing)\n"
#ifdef GENERALIZE
       << "-P\tprefix mode (for guessing)\n"
#endif
#endif //A_TERGO
#ifdef NUMBERS
       << "-N\tnumber entries (perfect hashing)\n"
#endif
#ifdef WEIGHTED
       << "-W\tweight arcs (for probabilities in guessing)\n"
#endif
       << "-v\tversion details\n"
       << "Example:\nsort -u word_list | "
       << prog_name << " -O > dict1.fsa\n";
}//usage

/* Name:	main
 * Class:	None.
 * Purpose:	Launches the program.
 * Parameters:	argc	- (i) number of program arguments;
 *		argv	- (i) program arguments.
 * Returns:	0	- if OK;
 *		1	- if invalid options;
 *		2	- if automaton could not be built;
 *		3	- if automaton could not be written to a file;
 *		4	- if there is not enough memory to build the automaton.
 * Remarks:	None.
 */
int
main(const int argc, const char *argv[])
{
  int	optimize = FALSE;
  int	make_index = FALSE;	// whether to create an index a tergo
  const char *input_file_name = NULL;
  const char *output_file_name = NULL;
#ifdef NUMBERS
  int	make_numbers = FALSE;
#endif
#ifdef WEIGHTED
  int	weighted = FALSE;
#endif //WEIGHTED
#ifdef GENERALIZE
  int 	prefix_mode = FALSE;
#endif //GENERALIZE
  char	FILLER = '_';
  
  if (argc >= 2) {
    for (int i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-O") == 0) {
	optimize = TRUE;
      }
      else if (strcmp(argv[i], "-X") == 0) {
	make_index = TRUE;
      }
      else if (strcmp(argv[i], "-A") == 0) {
	if (++i < argc)
	  ANNOT_SEPARATOR = *argv[i];
	else {
	  cerr << "Invalid annotation separator\n";
	  usage(argv[0]);
	  return 1;
	}
      }
      else if (strcmp(argv[i], "-i") == 0) {
	if (input_file_name) {
	  cerr << argv[0] << ": Multiple input files not supported" << endl;
	  cerr << "Ignoring earlier inputs" << endl;
	}
	if (++i < argc)
	  input_file_name = argv[i];
	else {
	  cerr << argv[0] << ": -i without file name" << endl;
	  usage(argv[0]);
	  return 1;
	}
      }
      else if (strcmp(argv[i], "-o") == 0) {
	if (output_file_name) {
	  cerr << argv[0] << ": Multiple output files not supported" << endl;
	  cerr << "Ignoring earlier output" << endl;
	}
	if (++i < argc)
	  output_file_name = argv[i];
	else {
	  cerr << argv[0] << ": -o without file name" << endl;
	  usage(argv[0]);
	  return 1;
	}
      }
      else if (strcmp(argv[i], "-F") == 0) {
	if (++i < argc) {
	  FILLER = (char)(argv[i][0]);
	}
	else {
	  cerr << argv[0] << ": -F without a character" << endl;
	  usage(argv[0]);
	  return 1;
	}
      }
#ifdef NUMBERS
      else if (strcmp(argv[i], "-N") == 0) {
	make_numbers = TRUE;
      }
#endif
#ifdef WEIGHTED
      else if (strcmp(argv[i], "-W") == 0) {
	weighted = TRUE;
      }
#endif //WEIGHTED
#if defined(A_TERGO) && defined(GENERALIZE)
      else if (strcmp(argv[i], "-P") == 0) {
	prefix_mode = TRUE;
      }
#endif //A_TERGO,WEIGHTED
      else if (strcmp(argv[i], "-v") == 0) {
#include "compile_options.h"
	return 0;
      }
      else {
	usage(argv[0]);
	return 1;
      }
    }//for
  }//if

#ifdef NUMBERS
  if (make_numbers && optimize) {
    cerr << "-N and -O cannot be specified together. Turning -O off" << endl;
    optimize = FALSE;
  }
#endif
  set_new_handler(&not_enough_memory);

  automaton autom;
  autom.FILLER = FILLER;
  if (input_file_name) {
    ifstream inpf(input_file_name);
    if (!inpf) {
      cerr << argv[0] << ": Could not open input file " << input_file_name
	   << endl;
      usage(argv[0]);
      exit(1);
    }
    if (!autom.build_fsa(inpf)) {
      cerr << argv[0] << ": Could not build the automaton" << endl;
      return 2;
    }
  }
  else if (!autom.build_fsa(cin)) {
    cerr << argv[0] << ": Could not build the automaton\n";
    return 2;
  }

#ifdef A_TERGO
  if (make_index) {
#ifdef PROGRESS
    cerr << "Pruning arcs" << endl;
#endif
    find_or_register(autom.get_root(), PRIM_INDEX, TRUE);
#ifdef WEIGHTED
    if (weighted) {
      weight_arcs(autom.get_root());
    }
#endif //WEIGHTED
#ifdef GENERALIZE
    collect_annotations(autom.get_root(), prefix_mode);
#endif
    autom.set_root(reduce_inner(autom.get_root()));
#ifdef GENERALIZE
    new_gen(autom.get_root(), prefix_mode);
    remove_annot_pointers(autom.get_root());
#endif
    // nodes must have arc_no set to -1
    mark_inner(autom.get_root(), -1);
  }
#endif

  if (optimize)
    share_arcs(autom.get_root());

  if (output_file_name) {
    ofstream outf(output_file_name, ios::binary); // the flag for M$ bug
    if (!outf) {
      cerr << argv[0] << ": Could not create file " << output_file_name
	   << endl;
      usage(argv[0]);
      return 1;
    }
#ifdef NUMBERS
    if (autom.write_fsa(outf, make_numbers))
#else
    if (autom.write_fsa(outf))
#endif  
      return 0;		// OK
  }
#ifdef NUMBERS
  else if (autom.write_fsa(cout, make_numbers))
#else
  else if (autom.write_fsa(cout))
#endif
    return 0;		// OK

  cerr << argv[0] << ": Could not write the automaton\n";
  return 3;
}//main

/***	EOF builds_fsa	***/
