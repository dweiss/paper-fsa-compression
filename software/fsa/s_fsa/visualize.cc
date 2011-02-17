/***	visualize.cc	***/

/*	Copyright (c) Jan Daciuk, 1999-2004	*/


#include	<iostream>
#include	<fstream>
#include	<string.h>
#include	<new>
#include	<vector>
#include	"fsa.h"
#include	"nstr.h"
#include	"common.h"
#include	"visualize.h"


/* bit operations */
int is_set(const int bit_no, const unsigned char *vector)
   { return ((vector[bit_no / 8] >> (bit_no & 7)) & 1); }
/* set particular bit */
void set_bit(const int bit_no, unsigned char *vector)
   { vector[bit_no / 8] |= (1 << (bit_no & 7)); }

vector<long int> start_states;

/* Name:	visual_fsa
 * Class:	visual_fsa (constructor).
 * Purpose:	Open dictionary files and read automata from them.
 * Parameters:	dict_names	- (i) dictionary file names.
 * Returns:	Nothing.
 * Remarks:	At least one dictionary file must be read.
 */
visual_fsa::visual_fsa(word_list *dict_names, const int compressed_dictionary)
  : fsa(dict_names), compressed(compressed_dictionary), visited(NULL)
{
}//visual_fsa::visual_fsa




/* Name:	create_graphs
 * Class:	visual_fsa
 * Purpose:	Creates graphs for all automata (all dictionaries).
 * Parameters:	None.
 * Returns:	TRUE if OK, FALSE otherwise.
 * Remarks:	None.
 */
int
visual_fsa::create_graphs(void)
{
  dict_list	*dict;
  fsa_arc_ptr	*dummy = NULL;
  fsa_arc_ptr	start_node, next_node;
  int		g;

  dictionary.reset(); g = 0;
  for (dict = &dictionary; dict->item(); dict->next()) {
    set_dictionary(dict->item());
    no_of_arcs = dict->item()->no_of_arcs;
//#ifdef FLEXIBLE
//    arc_size = dummy->size;
//#else
    arc_size = 1;
//#endif
    if (arc_size == 1)
      compressed = 1;		// the automaton has variable arc length
    if (compressed) {
      visited = new unsigned char[no_of_arcs / 8 + 1];
      memset(visited, 0, no_of_arcs / 8 + 1);
    }
    cout << "graph: {" << endl
	 << "  title: \"g" << g << "\"" << endl
	 << "  orientation: left_to_right"
	 << "  display_edge_labels: yes"
	 << "  splines: yes" << endl
	 << "  manhattan_edges: yes" << endl
	 << "  node.shape: ellipse" << endl << endl;
    // last node
    cout << "  node: {" << endl
	 << "    title: \"n0\"" << endl
	 << "  }" << endl << endl;
    start_node = dummy->first_node(current_dict);
    next_node = start_node.set_next_node(current_dict);
    current_offset = next_node.arc - current_dict;
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
    sparse_create_nodes();
    for (unsigned int k = 0; k < start_states.size(); k++) {
      create_node(current_dict + start_states[k]
#ifdef NUMBERS
		  + dummy->entryl
#endif
		  );
    }
    if (compressed)
      memset(visited, 0, no_of_arcs / 8 + 1); // clear marks
    current_offset = next_node.arc - current_dict;
    sparse_create_edges();
    for (unsigned int kk = 0; kk < start_states.size(); kk++) {
      create_edges(current_dict + start_states[kk]
#ifdef NUMBERS
		   + dummy->entryl
#endif
		   );
    }
#else //!(FLEXIBLE&STOPBIT&SPARSE)
    create_node(start_node.set_next_node(current_dict));
    if (compressed)
      memset(visited, 0, no_of_arcs / 8 + 1); // clear marks
    current_offset = next_node.arc - current_dict;
    create_edges(start_node.set_next_node(current_dict));
#endif
    cout << "}" << endl;
    if (compressed)
      delete [] visited;
    g++;
  }
  return TRUE;
}//visual_fsa::create_graphs

#ifdef FLEXIBLE
#ifdef NUMBERS
/* Name:	words_in_node
 * Class:	visual_fsa
 * Purpose:	Returns the number of different words (word suffixes)
 *		in the given node.
 * Parameters:	start		- (i) parent of the node to be examined.
 * Returns:	Number of different word suffixes in the given node.
 * Remarks:	None.
 */
int
visual_fsa::words_in_node(fsa_arc_ptr start)
{
  fsa_arc_ptr next_node = start.set_next_node(current_dict);

  return (start.get_goto() ?
    bytes2int((unsigned char *)next_node.arc - next_node.entryl,
	      next_node.entryl)
      : 0);
}//visual_fsa::words_in_node
#endif
#endif

/* Name:	create_node
 * Class:	visual_fsa
 * Purpose:	Creates a description of a node for one automaton.
 * Parameters:	start		- (i) look at that node.
 * Returns:	TRUE.
 * Remarks:	None.
 */
int
visual_fsa::create_node(fsa_arc_ptr start)
{
  fsa_arc_ptr	next_node = start;
  fsa_arc_ptr	next_start;
#ifdef NUMBERS
  fsa_arc_ptr	*dummy;
#endif

  if (start.get_goto() == 0 || start.arc == current_dict
#ifdef NUMBERS
      + dummy->entryl
#endif
      )
    return 0;


  // Handle only states that were not handled before
  if (compressed ?
      !is_set((next_node.arc - current_dict) / arc_size, visited)
      : next_node.arc - current_dict >= current_offset) {

    // Print node
    cout << " node: {" << endl
	 << "   title: \"n" << next_node.arc - current_dict << "\"" << endl;
#ifdef NUMBERS
    if (dummy->entryl) {
      // print the cardinality of the right language
      cout << "   info1: \"" << words_in_node(start) << "\"" << endl;
    }
#endif
    cout << " }" << endl << endl;

    // Increase current offset
    next_start = next_node;
    next_node = start.set_next_node(current_dict);
//    for (int i = 0; i < kids; i++, ++next_start)
    forallnodes(i)
      ;
    current_offset = next_node.arc - current_dict;
    next_node = next_start;
    if (compressed)
      set_bit((next_node.arc - current_dict) / arc_size, visited);

    // Print child nodes
//    for (int i = 0; i < kids; i++, ++next_node) {
    forallnodes(j) {
      create_node(next_node.set_next_node(current_dict));
    }
  }
  return TRUE;
}//visual_fsa::create_node

/* Name:	create_edges
 * Class:	visual_fsa
 * Purpose:	Creates descritpions of all edges going out from this node.
 * Parameters:	start		- look at that node.
 * Returns:	TRUE.
 * Remarks:	None.
 */
int
visual_fsa::create_edges(fsa_arc_ptr start)
{
  fsa_arc_ptr	next_node = start;
  fsa_arc_ptr	start_node;
#ifdef NUMBERS
  fsa_arc_ptr	*dummy;
#endif

  if (start.get_goto() == 0 || start.arc == current_dict
#ifdef NUMBERS
      + dummy->entryl
#endif
      )
    return 0;

  // Handle only states that were not handled before
  if (compressed ?
      !is_set((next_node.arc - current_dict) / arc_size, visited)
      : next_node.arc - current_dict >= current_offset) {

    start_node = next_node;
    forallnodes(i) {
      cout << "  edge: {" << endl
	   << "    sourcename: \"n" << start_node.arc - current_dict << "\""
	   << endl
	   << "    targetname: \"n"
	   << (next_node.get_goto() ?
	       next_node.set_next_node(current_dict) - current_dict
	       : 0) << "\""
	   << endl
	   << "    label: \"" << next_node.get_letter()
	   << (next_node.is_final() ? "!" : "")
	   << "\"" << endl;
      if (next_node.is_final()) {
	cout << "    color: red" << endl
	     << "    thickness: 4" << endl;
      }
      cout << "  }" << endl << endl;
    }
    current_offset = next_node.arc - current_dict;
    if (compressed)
      set_bit((start_node.arc - current_dict) / arc_size, visited);

    // Print edges of children
    next_node = start.set_next_node(current_dict);
    forallnodes(j) {
      create_edges(next_node.set_next_node(current_dict));
    }
  }
  return TRUE;
}//visual_fsa::create_edges

#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
/* Name:	sparse_create_nodes
 * Class:	visual_fsa
 * Purpose:	Creates descriptions of states in the sparse matrix.
 * Parameters:	None.
 * Returns:	Nothing.
 * Remarks:	This function sets the global variable start_states.
 *		The variable specifies starting states in annotations.
 *		This is necessary as the start state of the automaton
 *		no longer lies in annotations, so we get to annotations
 *		from several different places.
 */
void
visual_fsa::sparse_create_nodes(void)
{
//   unsigned char minchar = sparse_vect->get_minchar();
//   unsigned char maxchar = sparse_vect->get_maxchar();
//  long int no_of_states = sparse_vect->size() + maxchar - minchar + 1;
  long int no_of_states = sparse_vect->size() -
    sparse_vect->get_alphabet_size();
  long int next = 0L;
  for (long int n = sparse_vect->get_first(); n < no_of_states; n++) {
    bool found = false;
    char cc = ' ';
//     for (unsigned char i = minchar; i <= maxchar; i++) {
//       cc = (char)i;
    const char *a6t = sparse_vect->get_alphabet() + 1;
    for (int i = 1; i < sparse_vect->get_alphabet_size(); i++) {
      cc = *a6t++;
      if ((next = sparse_vect->get_target(n, cc)) != -1L) {
	found = true;
	break;
      }
    }
    if (found) {
      cout << " node {" << endl
	   << "   title: \"s" << n << "\"" << endl
	   << " }" << endl << endl;
      if (cc == ANNOT_SEPARATOR && next) {
	start_states.push_back(next);
      }
    }
  }
}//visual_fsa::sparse_create_nodes

/* Name:	sparse_create_edges
 * Class:	visual_fsa
 * Purpose:	Creates descriptions of edges in the sparse matrix.
 * Parameters:	None.
 * Returns:	Nothing.
 * Remarks:	Transitions labeled with the annotation separator
 *		lead outside of the sparse matrix.
 */
void
visual_fsa::sparse_create_edges(void)
{
//   unsigned char minchar = sparse_vect->get_minchar();
//   unsigned char maxchar = sparse_vect->get_maxchar();
//   long int no_of_states = sparse_vect->size() + maxchar - minchar + 1;
  long int no_of_states = sparse_vect->size() -
    sparse_vect->get_alphabet_size();
  long int next = 0L;
  for (long int n = sparse_vect->get_first(); n < no_of_states; n++) {
//     for (unsigned char i = minchar; i <= maxchar; i++) {
//       char cc = (char)i;
    const char *a6t = sparse_vect->get_alphabet() + 1;
    for (int i = 1; i < sparse_vect->get_alphabet_size(); i++) {
      char cc = *a6t++;
      if ((next = sparse_vect->get_target(n, cc)) != -1L) {
	cout << "  edge {" << endl
	     << "    sourcename: \"s" << n << "\"" << endl
	     << "    targetname: \""
	     << (cc == ANNOT_SEPARATOR ? "n" : "s") << "\"" << endl
	     << "    label: \"" << cc << "\"" << endl
#ifdef NUMBERS
#endif
	     << "  }" << endl << endl;
      }
    }
  }
}//visual_fsa::sparse_create_edges
#endif //FLEXIBLE&STOPBIT&SPARSE

/***	EOF visualize.cc	***/
