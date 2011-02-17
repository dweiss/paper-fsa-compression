/***	prefix.cc	***/

/*	Copyright (C) by Jan Daciuk, 1996-2004.	*/

#include	<iostream>
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	"fsa.h"
#include	"common.h"
#include	"prefix.h"
#include	"nstr.h"


/* Name:	prefix_fsa
 * Class:	prefix_fsa
 * Purpose:	Initialize class (constructor).
 * Parameters:	dict_list       - (i) list of dictionary (fsa) names;
 *		language_file	- (i) file with character set info.
 * Returns:     Nothing (constructor).
 * Remarks:     Only to launch fsa contructor.
 */
prefix_fsa::prefix_fsa(word_list *dict_list, const char *language_file)
: fsa(dict_list, language_file)
{
}//prefix_fsa::prefix_fsa


/* Name:        complete_file_words
 * Class:       prefix_fsa
 * Purpose:     Provide a list of all possible completions of words in a file.
 * Parameters:  io_obj          - (i/o) where to read words, and where
 *                                      to print them.
 * Returns:     Exit code.
 * Remarks:     The format of output is:
 *              surface_form WORD_SEP lexical_form
 *              where lexical form contains annotations.
 */
int
prefix_fsa::complete_file_words(tr_io &io_obj)
{
  char          *word;
  int		allocated;	// memory allocated for word

  word = new char[allocated=Max_word_len];
  while (get_word(io_obj, word, allocated, Max_word_len)) {
    if (!complete_prefix(word, io_obj))
      io_obj.print_not_found();
  }
  return state;
}//prefix_fsa::complete_file_words



/* Name:	complete_prefix
 * Class:	prefix_fsa
 * Purpose:	Finds all words that begin with a given prefix
 *		in all dictionaries.
 * Parameters:	word_prefix	- (i) prefix of the word;
 *		io_obj		- (o) where to print results.
 * Returns:	Number of completions found.
 * Remarks:	Empty prefix ("") lists contents of all automata.
 */
int
prefix_fsa::complete_prefix(const char *word_prefix, tr_io &io_obj)
{
  dict_list	*dict;
  int		compl_found = 0;
#if !(defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE))
  fsa_arc_ptr	*dummy = NULL;
#endif

  dictionary.reset();
  for (dict = &dictionary; dict->item(); dict->next()) {
    set_dictionary(dict->item());
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
    compl_found += sparse_compl_prefix(word_prefix, io_obj, 0,
				       sparse_vect->get_first());
#else
    fsa_arc_ptr nxtnode = dummy->first_node(current_dict);
    compl_found +=
      compl_prefix(word_prefix, io_obj, 0,
		   nxtnode.set_next_node(current_dict));
#endif
  }
  return compl_found;
}//prefix_fsa::complete_prefix


#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
/* Name:	sparse_compl_prefix
 * Class:	prefix_fsa
 * Purpose:	Provides all possible completions of a given prefix in the
 *		current automaton.
 * Parameters:	word_prefix	- (i) prefix to find;
 *		io_obj		- (o) where to print results;
 *		depth		- (i) length of prefix already found;
 *		start		- (i) look at children of that node.
 * Returns:	Number of completions found.
 * Remarks:	None.
 */
int
prefix_fsa::sparse_compl_prefix(const char *word_prefix, tr_io &io_obj,
				const int depth, const long start)
{
  int		curr_depth = depth;
  int		compl_found = 0;
  bool 		found = false;
  long 		current = start;
  long		next;
  fsa_arc_ptr	*dummy;

  do {
    found = false;
    if (current == 0L)
      return 0;

    if (*word_prefix == '\0')
      compl_found += sparse_compl_rest(io_obj, depth, start);

    if (depth + 1 >= cand_alloc)
      grow_string(candidate, cand_alloc, Max_word_len);

//     for (unsigned char i = sparse_vect->get_minchar();
// 	 i <= sparse_vect->get_maxchar(); i++) {
//      char cc = (char)i;
    const char *a6t = sparse_vect->get_alphabet() + 1;
    for (int i = 1; i < sparse_vect->get_alphabet_size(); i++) {
      char cc = *a6t++;
      if ((next = sparse_vect->get_target(current, cc)) != -1L) {
	candidate[curr_depth + 1] = '\0';
	if (*word_prefix == cc) {
#ifndef SHOW_FILLERS
	  if (cc != FILLER)
#endif
	    candidate[curr_depth++] = cc;
	  if (word_prefix[1] == '\0') {
	    if (sparse_vect->is_final(current, cc)) {
	      replacements.insert(candidate);
	      io_obj.print_repls(&replacements);
	      replacements.empty_list();
	      compl_found++;
	    }
	    if (cc == ANNOT_SEPARATOR) {
	      // Move to annotations
	      compl_found +=
		compl_rest(io_obj, curr_depth,
			   current_dict + next
#ifdef NUMBERS
			   + dummy->entryl
#endif
			   );
	    }
	    else {
	      // Stay in sparse vector
	      compl_found += sparse_compl_rest(io_obj, curr_depth, next);
	    }
	  }
	  else {
	    found = true;
	    word_prefix++;
	    current = next;
	  }
	  break;
	}
	else {
	  if (cc == FILLER) {
	    found = true;
	    current = next;
	    break;
	  }
	}
      }
    }
  } while (found);
  return compl_found;
}//prefix_fsa::sparse_compl_prefix
#endif

/* Name:	compl_prefix
 * Class:	prefix_fsa
 * Purpose:	Provides all possible completions of a given prefix in the
 *		current automaton.
 * Parameters:	word_prefix	- (i) prefix to find;
 *		io_obj		- (o) where to print results;
 *		depth		- (i) length of prefix already found;
 *		start		- (i) look at children of that node.
 * Returns:	Number of completions found.
 * Remarks:	None.
 */
int
prefix_fsa::compl_prefix(const char *word_prefix, tr_io &io_obj,
			 const int depth, fsa_arc_ptr start)
{
  int		curr_depth  = depth;
  int		compl_found = 0;
  fsa_arc_ptr	next_node = start;
  bool found = false;
  do {
    found = false;
    if (start.get_goto() == 0)
      return 0;

    if (*word_prefix == '\0')
      compl_found += compl_rest(io_obj, depth, start);

    if (depth + 1 >= cand_alloc)
      grow_string(candidate, cand_alloc, Max_word_len);

    forallnodes(i) {
      candidate[curr_depth + 1] = '\0';
      if (*word_prefix == next_node.get_letter()) {
#ifndef SHOW_FILLERS
	if (next_node.get_letter() != FILLER)
#endif
	  candidate[curr_depth++] = next_node.get_letter();
	if (word_prefix[1] == '\0') {
	  if (next_node.is_final()) {
	    replacements.insert(candidate);
	    io_obj.print_repls(&replacements);
	    replacements.empty_list();
	    compl_found++;
	  }
	  compl_found += compl_rest(io_obj, curr_depth,
				    next_node.set_next_node(current_dict));
	}
	else {
	  found = true;
	  word_prefix++;
	  start = next_node;
	}
	break;
      }
      else {
	if (next_node.get_letter() == FILLER) {
	  compl_found += compl_prefix(word_prefix + 1, io_obj, curr_depth,
				      next_node.set_next_node(current_dict));
// 	  found = true;
// 	  start = next_node;
// 	  break;
	}
      }
    }
    next_node = start.set_next_node(current_dict);
  } while (found);
  return compl_found;
}//prefix_fsa::compl_prefix


#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
/* Name:	sparse_compl_rest
 * Class:	prefix_fsa
 * Purpose:	Finds all completions of a given prefix.
 * Parameters:	depth		- (i) number of characters already in form;
 *		start		- (i) look at children of that node.
 * Returns:	Number of completions found.
 * Remarks:	This is invoked from comp_prefix, when prefix has been found.
 */
int
prefix_fsa::sparse_compl_rest(tr_io &io_obj, const int depth, const long start)
{
  int		curr_depth;
  int		compl_found = 0;
  long		next;
  fsa_arc_ptr	*dummy;

  if (start == 0L)
    return 0;

  if (depth > MAX_NOT_CYCLE) {
    cerr << "Possible cycle detected. Exiting." << endl;
    exit(5);
  }
  if (depth + 1 >= cand_alloc)
    grow_string(candidate, cand_alloc, Max_word_len);

//   for (unsigned char i = sparse_vect->get_minchar();
//        i <= sparse_vect->get_maxchar(); i++) {
//     char cc = (char)i;
  const char *a6t = sparse_vect->get_alphabet() + 1;
  for (int i = 1; i < sparse_vect->get_alphabet_size(); i++) {
    char cc = *a6t++;
    if ((next = sparse_vect->get_target(start, cc)) != -1L) {
      curr_depth = depth;
#ifndef SHOW_FILLERS
      if (cc != FILLER)
#endif
	candidate[curr_depth++] = cc;
      candidate[curr_depth] = '\0';
      if (sparse_vect->is_final(start, cc)) {
#ifdef DUMP_ALL
	io_obj.print_line(candidate);
#else
	replacements.insert(candidate);
	io_obj.print_repls(&replacements);
	replacements.empty_list();
#endif
	compl_found++;
      }
      if (cc == ANNOT_SEPARATOR) {
	// Move to annotations
	compl_found += compl_rest(io_obj, curr_depth, current_dict + next
#ifdef NUMBERS
				  + dummy->entryl
#endif
				  );
      }
      else {
	// Stay in sparse vector
	compl_found += sparse_compl_rest(io_obj, curr_depth, next);
      }
    }
  }
  return compl_found;
}//prefix_fsa::sparse_compl_rest
#endif

/* Name:	compl_rest
 * Class:	prefix_fsa
 * Purpose:	Finds all completions of a given prefix.
 * Parameters:	depth		- (i) number of characters already in form;
 *		start		- (i) look at children of that node.
 * Returns:	Number of completions found.
 * Remarks:	This is invoked from comp_prefix, when prefix has been found.
 */
int
prefix_fsa::compl_rest(tr_io &io_obj, const int depth, fsa_arc_ptr start)
{
  fsa_arc_ptr	next_node = start;
  int		curr_depth;
  int		compl_found = 0;
#ifdef NUMBERS
  fsa_arc_ptr	*dummy;
#endif

  if (start.arc == current_dict
#ifdef NUMBERS
      + dummy->entryl
#endif
      )	// the NULL state
    return 0;

  if (depth > MAX_NOT_CYCLE) {
    cerr << "Possible cycle detected. Exiting." << endl;
    exit(5);
  }
  if (depth + 1 >= cand_alloc)
    grow_string(candidate, cand_alloc, Max_word_len);

  forallnodes(i) {
    curr_depth = depth;
#ifndef SHOW_FILLERS
    if (next_node.get_letter() != FILLER)
#endif
      candidate[curr_depth++] = next_node.get_letter();
    candidate[curr_depth] = '\0';
    if (next_node.is_final()) {
#ifdef DUMP_ALL
      io_obj.print_line(candidate);
#else
      replacements.insert(candidate);
      io_obj.print_repls(&replacements);
      replacements.empty_list();
#endif
      compl_found++;
    }
    compl_found += compl_rest(io_obj, curr_depth,
			      next_node.set_next_node(current_dict));
  }
  return compl_found;
}//prefix_fsa::compl_rest


/***	EOF prefix.cc	***/
