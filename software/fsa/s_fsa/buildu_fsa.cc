/***	buildu_fsa	***/

/*	Copyright (C) Jan Daciuk, 1999-2004	*/

/*
  This file contains funtions particular to fsa_ubuild.
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
#include	"unode.h"
#include	"nstr.h"
#include	"nindex.h"
#include	"fsa_version.h"
#include	"build_fsa.h"


/* holds information about prefix of word already in automaton */
struct prefix {
  node		*end_node;	// last node of automaton constaining word
  node		**path;		// path of all nodes in the prefix
  int		length;		// length of prefix
  int		first;		// first confluence (reentrant) state
  int		max_length;	// allocated length of path
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
  arc_node	*offspr;


  if (length >= common_prefix->max_length) {
    // allocate more space
    node **new_path = new node *[common_prefix->max_length += 128];
    memcpy(new_path, common_prefix->path,
	   (common_prefix->max_length - 128) * sizeof(node *));
    if (common_prefix->max_length > 128)
      delete common_prefix->path;
    common_prefix->path = new_path;
  }
  common_prefix->path[length] = start_node;
  for (int i = 0; i < start_node->get_no_of_kids(); i++) {
    offspr = start_node->get_children() + i;
    if (offspr->letter == *word) {
      if (offspr->child) {
	int l = find_common_prefix(offspr->child, word + 1, common_prefix,
				   length + 1);
	if (start_node->hit_node(0) > 1) {
	  common_prefix->first = length;
	}
	return l;
      }
      else {
	break;
      }
    }
  }
  if (start_node->hit_node(0) > 1) {
    common_prefix->first = length;
  }
  common_prefix->end_node = start_node;
  return common_prefix->length = length;
}//find_common_prefix

/* Name:	already_there
 * Class:	none.
 * Purpose:	Checks whether a word is already in the automaton.
 * Parameters:	word		- (i) word to be checked;
 *		common_prefix	- (i) common prefix of the word
 *					and all words already in fsa.
 * Returns:	TRUE if the words is in the automaton, FALSE otherwise.
 * Remarks:	Since the prefix is calculated first, we don't need
 *		to check the number of children of the node before
 *		the last one in the common prefix path - an arc with
 *		the appropriate letter must be there.
 */
int
already_there(const char *word, prefix *common_prefix)
{
  int l = common_prefix->length - 1;
  if (l < 0)
    return FALSE;
  arc_node *a = (common_prefix->path[l])->get_children();
  for (; a->letter != word[l]; a++)
    ;
  return (a->is_final && (word[l+1] == '\0'));
}//already_there

/* Name:	clone
 * Class:	None.
 * Purpose:	Provide a copy of the argument node, and update hit count
 *		of children accordingly.
 * Parameters:	n	- (i) node to be copied.
 * Returns:	A copy of the argument.
 * Remarks:	As a new node is created with the same arcs as the argument
 *		node, pointing to the same nodes as arcs of the argument node,
 *		so the hit count of the target nodes of those arcs must be
 *		increased as well!
 */
node *
clone(node *n)
{
  node *nn = new node(n);
  arc_node *a = n->get_children();
  for (int i = 0; i < n->get_no_of_kids(); i++, a++)
    if (a->child)
      a->child->hit_node();
  return nn;
}

/* Name:	build_fsa
 * Class:	automaton
 * Purpose:	Reads a list of words and builds a final state
 *		automaton that recognizes them.
 * Parameters:	infile	- (i) file to be read.
 * Returns:	TRUE if automaton built, FALSE otherwise.
 * Remarks:	Words do not need to be sorted. Duplicates are allowed.
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
  node		*prev_node;
  node		*next_node;
  int		node_modified;
  int		i;
  char		junk;
#ifdef PROGRESS
  int		line_no = 0;
#endif

  word_buffer = new char[WORD_BUFFER_LENGTH];
  word = &word_buffer[0];
  common_prefix.max_length = 0;
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
    if (*word == '\0')
      continue;

#ifdef DEBUG
    cerr << "`" << word << "'\n";
#endif
#ifdef PROGRESS
    if ((line_no++ & 0x03FF) == 0)
      cerr << (line_no - 1) << " lines processed" << endl;
#endif
    // the rest is word - common_prefix
    common_prefix.first = 0;
    rest = word + find_common_prefix(root, word, &common_prefix, 0);
    if (already_there(word, &common_prefix))
      continue;

    last_node = common_prefix.end_node;
    int len = strlen(word);
#ifdef DEBUG
    cerr << "Prefix len: " << common_prefix.length << ", rest: `" << rest
         << "'\n";
#endif


    if (common_prefix.first) {
      // There is a node in the common_prefix path that has hit count greater
      // than one, i.e. a confluence or reentrant state - one that is pointed
      // to by more than one node.
      // Since modifying anything in the common prefix path after that node
      // means also modifying other parts of the transducer, copies
      // of all nodes in the common prefix path from that node
      // must be created

      new_node = clone(last_node);
      prev_node = next_node = new_node;

      // To avoid creating loops, the first node before the first
      // reentrant node must be unregistered
      if (common_prefix.first > 1)
	unregister_node(common_prefix.path[common_prefix.first - 1]);

      // If the rest of the word (after prefix) exists, make a chain of nodes
      // with its consecutive letters as labels of the linking transitions,
      // and attach it to the last node found in prefix.
      if (*rest) {
	new_node = next_node->add_postfix(rest);
      }
      // the postfix may have been attached earlier in path
      // so that part must be cloned as well
      // we deal with it by moving the first node pointer earlier
      for (i = common_prefix.first - 1; i > 0; --i)
	if (common_prefix.path[i]->hit_node(0) > 1)
	  common_prefix.first = i;

      // now extract one branch from a few that have been compressed into one
      for (i = common_prefix.length - 1; i >= common_prefix.first; --i) {
	// note that next_node is not registered yet,
	// as it is only a copy, and its hit count is 0
	// comp_or_reg should take care of hit_count
	new_node = comp_or_reg(next_node);
	next_node = new_node;

	// create a new node
	prev_node = clone(common_prefix.path[i]);

	// attach next_node to it
	// note that prev_node is only a copy of an existing node,
	// so there is no need to unregister it
	mod_child(prev_node, next_node, word[i], i == len - 1, TRUE);
	next_node = prev_node;
      }//for

      // Modify the first node with hit count > 1
      // note that now i = common_prefix.first - 1
      prev_node = common_prefix.path[i];
      new_node = comp_or_reg(next_node);
      next_node = new_node;
      // the node will always be modified here
      node_modified = TRUE;
      if (i) {
	// root is never registered
	unregister_node(prev_node);
	prev_node->hit_node(- prev_node->hit_node(0));
      }
      mod_child(prev_node, next_node, word[i], i == len - 1, node_modified);
      next_node = prev_node;

      // Now the copy of the first confluence (reentrant) node is modified,
      // the node immediately preceding it - is not (yet),
      // i.e. it will need mod_child to redirect the arc,
      // and the previous node is not unregistered yet,
      // and it must be unregistered (unless it is the root)
    }//if path contains a reentrant node
    else {
      // This is probably an early stage in the building process.
      // There is reentrant node in the common prefix path.
      // This means there is no need to create copies of nodes

      i = common_prefix.length;
      next_node = prev_node = last_node;
      node_modified = TRUE;
      if (*rest) {
	if (i) {
	  // this may be root
	  unregister_node(prev_node);
	  prev_node->hit_node(- prev_node->hit_node(0));
	}
	// If the rest of  the word (after prefix) exists, make a chain
	// of nodes with consecutive letters as labels of corresponding
	// transitions, and attach it to the last node found in prefix.
	next_node = prev_node->add_postfix(rest);
      }//if there is a postfix to add
      else {
	// there is nothing to add, but we may have to modify
	// the final attribute of an arc
	--i;
	prev_node = common_prefix.path[i];
	if (i) {
	  unregister_node(prev_node);
	  prev_node->hit_node(- prev_node->hit_node(0));
	}
	mod_child(prev_node, next_node, word[i], i == len - 1);
      }
    }

    // However, nodes in the path are already registered, so those
    // of them that are modified should be withdrawn from the register
    // and reregistered. In principle, this applies to the last node
    // in the common prefix path. However, by changing that node,
    // a node that is isomorphic to an already registered one can be created,
    // which implies replacement rather than registering, which in turn
    // forces a change in the preceding node, and so on.
    // This also affects the nodes before the first reentrant one
    // (the one with hit count > 1)
    // Life is brutal
    while (--i > 0 && node_modified) {
      next_node = prev_node;
      prev_node = common_prefix.path[i];
      new_node = comp_or_reg(next_node);
      if ((node_modified = (new_node != next_node))) {
	next_node = new_node;
	unregister_node(prev_node);
	prev_node->hit_node(- prev_node->hit_node(0));
      }
      mod_child(prev_node, next_node, word[i], i == len - 1, node_modified);
    }

    // Now we may need to modify root
    if (node_modified && i == 0) {
      next_node = prev_node;
      prev_node = common_prefix.path[i];
      new_node = comp_or_reg(next_node);
      mod_child(prev_node, new_node, word[i], i == len - 1,
		new_node != next_node);
    }
  }//while

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
       << "-X\tmake index a tergo (for category guessing)\n"
#endif
#ifdef NUMBERS
       << "-N\tnumber entries (perfect hashing)\n"
#endif
#ifdef WEIGHTED
       << "-W\tweight arcs (for probabilities in guessing)\n"
#endif
       << "-v\tversion details\n"
       << "Example: " << prog_name << " -i word_list -O > dict1.fsa\n";
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
  char  FILLER = '_';
  
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
      else if (strcmp(argv[1], "-v") == 0) {
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
    remove_annot_pointers(autom.get_root()); // !!!! some states get 0 hit_count
#endif
    // nodes must have arc_no set to -1
    mark_inner(autom.get_root(), -1);
  }
#endif //A_TERGO

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
