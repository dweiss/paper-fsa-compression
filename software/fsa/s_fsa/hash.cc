/***	hash.cc	***/

/*	Copyright (C) by Jan Daciuk, 1996-2004.	*/

#include	<iostream>
//#include	<sstream>
#include	<string.h>
#include	<stdlib.h>
#include	"fsa.h"
#include	"common.h"
#include	"hash.h"
#include	"nstr.h"

using namespace std;

/* Name:	hash_fsa
 * Class:	hash_fsa
 * Purpose:	Initialize class (constructor).
 * Parameters:	dict__names 	- (i) list of dictionary (fsa) names;
 *		language_file	- (i) file with character set info.
 * Returns:     Nothing (constructor).
 * Remarks:     Only to launch fsa contructor.
 */
hash_fsa::hash_fsa(word_list *dict_names, const char *language_file)
: fsa(dict_names, language_file)
{
  dictionary.reset();
  for (dict_list *p = &dictionary; p->item() != NULL; p->next()) {
    if (p->item()->entryl <= 0) {
      state = 2;		// the dictionary has not been built with -N
      std::cerr << "fsa_hash: the dictionary has not been built with -N\n";
    }
  }
}//hash_fsa::hash_fsa


/* Name:        hash_file
 * Class:       hash_fsa
 * Purpose:     translate numbers to words or words to numbers in a file.
 * Parameters:  io_obj          - (i/o) where to read words, and where
 *                                      to print them;
 *		direction	- whether translate words to numbers, or
 *				  numbers to words.
 * Returns:     Exit code.
 * Remarks:     
 */
int
hash_fsa::hash_file(tr_io &io_obj, const direction_t direction)
{
#ifdef FLEXIBLE
#ifdef NUMBERS
  const int	Num_buf_len = 20;
  char          *word;
  int		allocated;
  int		n;
  char		number_buffer[Num_buf_len];
  fsa_arc_ptr	*dummy = NULL;
  const char	*w;
#endif
#endif

#ifndef FLEXIBLE
  cerr << "This program must be compiled with NUMBERS and FLEXIBLE" << endl;
  return 0;
#else
#ifndef NUMBERS
  cerr << "This program must be compiled with NUMBERS and FLEXIBLE" << endl;
  return 0;
#else
  dictionary.reset();
  dict_list *dict = &dictionary;
  current_dict = dict->item()->dict.arc;
#if defined(STOPBIT) && defined(SPARSE)
  sparse_vect = dict->item()->sparse_vect;
#endif
  FILLER = dict->item()->filler;
  dummy->gtl = dict->item()->gtl;
  dummy->size = dummy->gtl + goto_offset;
  dummy->entryl = dict->item()->entryl;
  dummy->aunit = dummy->entryl ? 1 : (2 + dummy->gtl);

  word = new char[allocated = Max_word_len];
  while (get_word(io_obj, word, allocated, Max_word_len)) {
    if (direction == words_to_numbers) {
      // This is a queer way of converting an integer to string
      // Is there a simpler way?
//	ostringstream os(number_buffer, Num_buf_len, ios::out);
//	os.seekp(ios::beg);
//	os << find_number(word, dummy->first_node(current_dict), 0) << ends;
//	replacements.insert(os.str());
      // Update: Stroustrup no longer permits even the code above.
      // I had to write my own conversion!!!
#if defined(STOPBIT) && defined(SPARSE)
      long osn = sparse_find_number(word, sparse_vect->get_first(), 0);
#else
      fsa_arc_ptr nxt_node = dummy->first_node(current_dict);
      long osn = find_number(word, nxt_node.set_next_node(current_dict), 0);
#endif
      int osnp = 0;
      if (osn == -1L) {
	number_buffer[0] = '-';
	number_buffer[1] = '1';
	number_buffer[2] = '\0';
      }
      else {
	do {
	  number_buffer[osnp] = (osn % 10) + '0';
	  osn /= 10;
	  if (++osnp == Num_buf_len - 1) {
	    break;    // our numbers are less than 20 digits long, aren't they?
	  }
	} while (osn > 0);
	// reverse the number
	number_buffer[osnp] = 0;
	int middle = (osnp / 2);
	--osnp;
	for (int i = 0; i < middle; i++) {
	  char temp = number_buffer[i];
	  number_buffer[i] = number_buffer[osnp - i];
	  number_buffer[osnp - i] = temp;
	}
      }
      replacements.insert(number_buffer);
      io_obj.print_repls(&replacements);
    }
    else {
      // Sorry, I give up, I will not use istrstream for this
      if ((n = atoi(word)) < 0) {
	cerr << "Number out of range. Ignored" << endl;
	continue;
      }
#if defined(STOPBIT) && defined(SPARSE)
      w = sparse_find_word(n, 0, sparse_vect->get_first(), 0);
#else
      fsa_arc_ptr xnt_node = dummy->first_node(current_dict);
      w = find_word(n, 0, xnt_node.set_next_node(current_dict), 0);
#endif
      if (w) {
	replacements.insert(w);
	io_obj.print_repls(&replacements);
      }
      else
	io_obj.print_not_found();
    }
    replacements.empty_list();
  }
  return state;
#endif
#endif
}//hash_fsa::hash_file

#ifdef FLEXIBLE
#ifdef NUMBERS

#if defined(STOPBIT) && defined(SPARSE)
/* Name:	sparse_find_number
 * Class:	hash_fsa
 * Purpose:	Translates word into a corresponding number.
 * Parameters:	word		- (i) word to be found;
 *		start		- (i) the node to examined
 *		word_no		- (i) number of words before the given word.
 * Returns:	The number assigned to the word or -1 if not found.
 * Remarks:	The number assigned to a word is its position in a (sorted)
 *		file used to build the automaton.
 *		When more than one automaton is used, the numbers concern
 *		individual automata. No information about a specific
 *		automaton is returned.
 *
 *		All nodes contain information about the number of different
 *		words (more precisely: word suffixes) contained in this node
 *		and all nodes below.
 */
int
hash_fsa::sparse_find_number(const char *word, const long start, int word_no)
{
  bool found = false;
  long current = start;
  long next = current;
  fsa_arc_ptr *dummy;
  do {
    found = false;
    current = next;
    if ((next = sparse_vect->get_target(current, *word)) != -1L) {
      word_no += sparse_vect->get_hash_v(current, *word);
      if (word[1] == '\0' && sparse_vect->is_final(current, *word)) {
	// This is the end of the word, and the word has been found
	return word_no;
      }
      else {
	if (current == 0) {
	  // end of the string in the automaton
	  return -1;
	}
	word_no += sparse_vect->is_final(current, *word);
	current = next;
	if (*word++ == ANNOT_SEPARATOR) {
	  // Move to annotations (out of sparse matrix)
	  return find_number(word, current_dict + next
#ifdef NUMBERS
			     + dummy->entryl
#endif
			     , word_no);
	}
	else {
	  found = true;
	}//if
      }//if
    }//if
  } while (found); 
  return -1;
}//hash_fsa::sparse_find_number
#endif //STOPBIT&SPARSE

/* Name:	find_number
 * Class:	hash_fsa
 * Purpose:	Translates word into a corresponding number.
 * Parameters:	word		- (i) word to be found;
 *		start		- (i) the node to examined
 *		word_no		- (i) number of words before the given word.
 * Returns:	The number assigned to the word or -1 if not found.
 * Remarks:	The number assigned to a word is its position in a (sorted)
 *		file used to build the automaton.
 *		When more than one automaton is used, the numbers concern
 *		individual automata. No information about a specific
 *		automaton is returned.
 *
 *		All nodes contain information about the number of different
 *		words (more precisely: word suffixes) contained in this node
 *		and all nodes below.
 */
int
hash_fsa::find_number(const char *word, fsa_arc_ptr start, int word_no)
{
  bool found = false;
  do {
    found = false;
    fsa_arc_ptr next_node = start;
    forallnodes(i) {
      if (*word == next_node.get_letter()) {
	if (word[1] == '\0' && next_node.is_final()) {
	  // This is the end of the word, and the word has been found
	  return word_no;
	}
	else {
	  if (next_node.get_goto() == 0) {
	    // end of the string in the automaton
	    return -1;
	  }
	  start = next_node.set_next_node(current_dict);
	  word++;
	  word_no += next_node.is_final();
	  found = true;
	  break;
	}
      }
      else {
	if (next_node.is_final())
	  word_no++;
	word_no += words_in_node(next_node);
      }
    }
  } while (found); 
  return -1;
}//hash_fsa::find_number


/* Name:	words_in_node
 * Class:	hash_fsa
 * Purpose:	Returns the number of different words (word suffixes)
 *		in the given node.
 * Parameters:	start		- (i) parent of the node to be examined.
 * Returns:	Number of different word suffixes in the given node.
 * Remarks:	None.
 */
int
hash_fsa::words_in_node(fsa_arc_ptr start)
{
  fsa_arc_ptr next_node = start.set_next_node(current_dict);

  return (start.get_goto() ?
	  bytes2int((unsigned char *)next_node.arc - next_node.entryl,
		    next_node.entryl)
	  : 0);
}//hash_fsa::words_in_node


#if defined(STOPBIT) && defined(SPARSE)
/* Name:	sparse_find_word
 * Class:	hash_fsa
 * Purpose:	Finds a word whose number in a dictionary is given as argument.
 * Parameters:	word_no		- (i) number of the word to be found;
 *		n		- (i) number of words analysed;
 *		start		- (i) current arc;
 *		level		- (i) character number of a word,
 *					or the distance from root.
 * Returns:	The word, or NULL if not found.
 * Remarks:	Only the first dictionary is searched.
 *		If we draw a graph representing the automaton, with the root
 *		on the left, and leaves on the right sorted from top to bottom,
 *		then the number of words analysed means the number of words
 *		that are in branches top of the search path in the automaton.
 */
const char *
hash_fsa::sparse_find_word(const int word_no, int n, const long start,
			   const int level)
{
  
  int m;
  bool found = false;
  int l = level;
  long current = start;
  long next = start;
  long last_target = 0L;
  char last_label = '\0';
  fsa_arc_ptr *dummy;

  do {
    if (current == 0L) {
      return NULL;
    }
    found = false;
    last_label = '\0';
    if (l + 1 >= cand_alloc)
      grow_string(candidate, cand_alloc, Max_word_len);
//     for (unsigned char i = sparse_vect->get_minchar();
// 	 i <= sparse_vect->get_maxchar(); i++) {
//       char cc = (char)i;
    const char *a6t = sparse_vect->get_alphabet() + 1;
    for (int i = 1; i < sparse_vect->get_alphabet_size(); i++) {
      char cc = *a6t++;
      if ((next = sparse_vect->get_target(current, cc)) != -1L) {
	m = n + sparse_vect->get_hash_v(current, cc);
	if (m == word_no && sparse_vect->is_final(current, cc)) {
	  candidate[l] = cc;
	  candidate[l + 1] = '\0';
	  return candidate;
	}//if transition final

	if (m > word_no) {
	  // We have gone one transition too far
	  cc = last_label;
	  candidate[l] = cc;
	  l++;
	  if (cc == ANNOT_SEPARATOR) {
	    return find_word(word_no, n + sparse_vect->is_final(current,
								last_label),
			     current_dict + last_target
			     
#ifdef NUMBERS
			     + dummy->entryl
#endif
			     , l);
	  }
	  n += sparse_vect->get_hash_v(current, last_label) +
	    sparse_vect->is_final(current, last_label);
	  current = last_target;
	  found = true;
	  break;
	}//if word ending in descendats of the current arc
	else {
	  last_label = cc;
	  last_target = next;
	}
      }//if there is a transition for the label
    }//for
    if (last_label && !found) {
      // Use the last transition
      candidate[l++] = last_label;
      if (last_label == ANNOT_SEPARATOR) {
	return find_word(word_no, n + sparse_vect->is_final(current,
							    last_label),
			 current_dict + last_target
			 
#ifdef NUMBERS
			 + dummy->entryl
#endif
			 , l);
      }//if transition labeled with annotation separator
      n += sparse_vect->get_hash_v(current, last_label) +
	sparse_vect->is_final(current, last_label);
      current = last_target;
      found = true;
    }//if any transitions found in this state
  } while (found);

  return NULL;
}//hash_fsa::sparse_find_word
#endif //STOPBIT&SPARSE

/* Name:	find_word
 * Class:	hash_fsa
 * Purpose:	Finds a word whose number in a dictionary is given as argument.
 * Parameters:	word_no		- (i) number of the word to be found;
 *		n		- (i) number of words analysed;
 *		start		- (i) current arc;
 *		level		- (i) character number of a word,
 *					or the distance from root.
 * Returns:	The word, or NULL if not found.
 * Remarks:	Only the first dictionary is searched.
 *		If we draw a graph representing the automaton, with the root
 *		on the left, and leaves on the right sorted from top to bottom,
 *		then the number of words analysed means the number of words
 *		that are in branches top of the search path in the automaton.
 */
const char *
hash_fsa::find_word(const int word_no, int n, fsa_arc_ptr start,
		    const int level)
{
  
  int m;
  bool found = false;
  int l = level;

  do {
    found = false;
    fsa_arc_ptr next_node = start;
    if (l + 1 >= cand_alloc)
      grow_string(candidate, cand_alloc, Max_word_len);
    forallnodes(i) {
      if (next_node.is_final()) {
	if (n == word_no) {
	  candidate[l] = next_node.get_letter();
	  candidate[l + 1] = '\0';
	  return candidate;
	}
	else
	  n++;
      }

      if ((m = n + words_in_node(next_node)) > word_no) {
	candidate[l] = next_node.get_letter();
	l++;
	start = next_node.set_next_node(current_dict);
	found = true;
	break;
      }
      else
	n = m;
    }//for
  } while (found);

  return NULL;
}//hash_fsa::find_word

#endif
#endif

/***	EOF hash.cc	***/
