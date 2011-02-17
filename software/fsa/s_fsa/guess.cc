/***	guess.cc	***/

/*	Copyright (c) Jan Daciuk, 1996-2004	*/
      
/*
  This file contains basic functions implementing guessing automata
  Note that in case of mmorph guessing, the strings in the automaton
  have the form:
  inverted_+K1e1+K2K3K4a1+e2+categories
  where:
  inverted	- is the inverted inflected form;
  K1 		- says how many characters to delete from the end of
  		  the inflected form to get the canonical form;
 		  it is single letter: 'A' means none, 'B' - 1,
 		  'C' - 2, etc.
  e1		- is the ending to append after removing K1 letters to
  		  get the canonical form;
  K2		- says how many character to delete from the end of
  		  the canonical form to get the lexical form;
 		  it is single letter: 'A' means none, 'B' - 1,
 		  'C' - 2, etc.
  K3		- position of the arch-phoneme
 		  'A' means there are no arch-phonemes,
 		  'B' - the first character, 'C' - the second one etc.
 		  (after removal of K2 chars);
  K4		- number of characters the phoneme replaces;
 		  'A' means 0, 'B' - 1, etc.;
 		  present only if K3 is not 'A'
  e2		- ending of lexical form
  a1		- the arch-phoneme; present only (with preceding "+")
		  when K3 is not 'A'
*/

#include	<iostream>
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#ifdef DMALLOC
#include	"dmalloc.h"
#endif
#include	"nstr.h"
#include	"fsa.h"
#include	"common.h"
#include	"guess.h"

static const int	MAX_GUESSES = 30; // max number of guesses / word

char *invert(char *word);

/* Name:	invert
 * Class:	None.
 * Purpose:	Inverts a string.
 * Parameters:	word		- (i/o) the word to be inverted.
 * Returns:	The (inverted) word.
 * Remarks:	Inversion is done in situ. The previous contents is destroyed.
 */
char *
invert(char *word)
{
  int	c;
  int	l = strlen(word);
  char	*p1, *p2;

  for (p1 = word, p2 = word + l; p1 < p2;) {
    c = *p1;
    *p1++ = *--p2;
    *p2 = c;
  }
  return word;
}//invert

/* Name:	guess_fsa
 * Class:	guess_fsa
 * Purpose:	Constructor.
 * Parameters:	dict_list	- (i) list of dictionary (fsa) names;
 *		lexemes_in_dict	- (i) TRUE if dictionary contains information
 *					on how to obtain base forms;
 *		prefixes_in_dict- (i) TRUE if dictionary contains information
 *					on prefixes of words;
 *		infixes_in_dict	- (i) TRUE if dictionary contains information
 *					on infixes of words;
 *		mmorph_in_dict	- (i) TRUE if dictionary contains information
 *					on morphological descriptions of words
 *					in mmorph format.
 *		language_file	- (i) file with character set info.
 * Returns:	Nothing (constructor).
 * Remarks:	Only to launch fsa contructor.
 *		mmorph is MULTEXT morphology tool available from ISSCO, Geneva.
 */
guess_fsa::guess_fsa(word_list *dict_list, const int lexemes_in_dict,
		     const int prefixes_in_dict, const int infixes_in_dict,
		     const int mmorph_in_dict, const char *language_file)
    : fsa(dict_list, language_file)
{
  guess_lexemes = lexemes_in_dict;
  guess_prefix = prefixes_in_dict;
  guess_infix = infixes_in_dict;
  guess_mmorph = mmorph_in_dict;
#ifdef GUESS_MMORPH
  mmorph_alloc = 2 * Max_word_len;
  mmorph_buffer = new char[mmorph_alloc];
#endif
}//guess_fsa::guess_fsa


/* Name:	guess_file
 * Class:	guess_fsa
 * Purpose:	Guess categories of all words in the file.
 * Parameters:	io_obj		- (i/o) where to read words,
 *					and where to print them.
 * Returns:	Exit code.
 * Remarks:	
 */
int
guess_fsa::guess_file(tr_io &io_obj)
{
  int		allocated;
  char		*word;

  word = new char[allocated = Max_word_len];
  while (io_obj >> (word + 1)) {
    if (io_obj.get_junk() != '\n') {
      io_obj.set_buf_len(Max_word_len);
      while (io_obj.get_junk() != '\n') {
	grow_string(word, allocated, Max_word_len);
	word[allocated - Max_word_len - 1] = io_obj.get_junk();
	if (!(io_obj >> (word + allocated - Max_word_len)))
	  break;
      }
      io_obj.set_buf_len(allocated - 1);
    }
    *word = FILLER;			// mark word beginning
    word_length = strlen(word + 1);
    if (guess_word(invert(word))) {
      io_obj.print_repls(&replacements);
      replacements.empty_list();
    }
    else
      io_obj.print_not_found();
  }
  return state;
}//guess_fsa::guess_file




/* Name:	guess_word
 * Class:	guess_fsa
 * Purpose:	guess word category using all available dictionaries.
 * Parameters:	word	- (i) word to be checked.
 * Returns:	List of categories.
 * Remarks:	This is done separately for each dictionary. Maybe a weight
 *		should be attributed to endings, so that only the longest
 *		endings would be considered.
 *
 *		The word to be checked is inverted, with a filler
 *		in front!
 */
int
guess_fsa::guess_word(const char *word)
{
  dict_list		*dict;
#if !(defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE))
  fsa_arc_ptr		*dummy = NULL;
#endif

  word_ff = (char *)word;
  dictionary.reset();
  for (dict = &dictionary; dict->item(); dict->next()) {
    set_dictionary(dict->item());
    ANNOT_SEPARATOR = dict->item()->annot_sep;
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
    sparse_guess(word, sparse_vect->get_first(), 0);
#else
    guess(word, dummy->first_node(current_dict), 0);
#endif
  }

  return replacements.how_many();
}//guess_fsa::guess_word



#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
/* Name:	sparse_guess
 * Class:	guess_fsa
 * Purpose:	Find categories the word might belong to.
 * Parameters:	word		- (i) word to look for;
 *		start		- (i) look at that node;
 *		vanity_level	- (i) level of calls with null word.
 * Returns:	Number of words on the list of categories the word
 *		might belong to.
 * Remarks:	The word is inverted, with a filler in front
 */
int
guess_fsa::sparse_guess(const char *word, long int start,
			const int vanity_level)
{
  int		to_be_completed = TRUE;
  long		current = start;
  long		next;
  fsa_arc_ptr	*dummy;


  // See if we are searching too deep in vain
  if (vanity_level > MAX_VANITY_LEVEL)
    return replacements.how_many();

  // Look at children
  if ((next = sparse_vect->get_target(current, *word)) != -1L) {
    sparse_guess(word + 1, next, 0);
    to_be_completed = FALSE;
  }

  // No appropriate children found - look for annotation separator
  if (to_be_completed || replacements.how_many() == 0) {
    if ((next = sparse_vect->get_target(current, ANNOT_SEPARATOR)) != -1L) {
#ifdef GUESS_PREFIX
      if (guess_prefix || guess_infix)
	check_prefix(current_dict + next
#ifdef NUMBERS
		     + dummy->entryl
#endif
		     , 0);
      else
#endif
#ifdef GUESS_LEXEMES
	if (guess_lexemes) {
	  guess_stem(current_dict + next
#ifdef NUMBERS
		     + dummy->entryl
#endif
		     , 0, 0);
	}
	else
#endif
	  print_rest(current_dict + next
#ifdef NUMBERS
		     + dummy->entryl
#endif
		     , 0);
      to_be_completed = FALSE;
    }

    // No annotation mark found - take all annotations below this node
    if (to_be_completed) {
//       for (unsigned char k = sparse_vect->get_minchar();
// 	   k <= sparse_vect->get_maxchar(); k++) {
// 	char cc = (char)k;
      char *a6t = sparse_vect->get_alphabet() + 1;
      for (int i = 1; i < sparse_vect->get_alphabet_size(); i++) {
	char cc = *a6t++;
	if ((next = sparse_vect->get_target(current, cc)) != -1L) {
	  sparse_guess("", next, vanity_level + 1);
	}
      }
    }
  }
  return replacements.how_many();
}//guess_fsa::sparse_guess
#else //!(FLEXIBLE&STOPBIT&SPARSE)
/* Name:	guess
 * Class:	guess_fsa
 * Purpose:	Find categories the word might belong to.
 * Parameters:	word		- (i) word to look for;
 *		start		- (i) look at children of that node;
 *		vanity_level	- (i) level of calls with null word.
 * Returns:	Number of words on the list of categories the word
 *		might belong to.
 * Remarks:	The word is inverted, with a filler in front
 */
int
guess_fsa::guess(const char *word, fsa_arc_ptr start, const int vanity_level)
{
  fsa_arc_ptr next_node = start.set_next_node(current_dict);
  int		to_be_completed = TRUE;


  // See if we are searching too deep in vain
  if (vanity_level > MAX_VANITY_LEVEL)
    return replacements.how_many();

  // Look at children
  forallnodes(i) {
    if (*word == next_node.get_letter()) {
      guess(word + 1, next_node, 0);
      to_be_completed = FALSE;
      break;
    }
  }

  // No appropriate children found - look for annotation separator
  if (to_be_completed || replacements.how_many() == 0) {
    next_node = start.set_next_node(current_dict);
    forallnodes(j) {
      if (next_node.get_letter() == ANNOT_SEPARATOR) {
	fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
#ifdef GUESS_PREFIX
	if (guess_prefix || guess_infix)
	  check_prefix(nxt_node, 0);
	else
#endif
#ifdef GUESS_LEXEMES
	if (guess_lexemes) {
	  guess_stem(nxt_node, 0, 0);
	}
	else
#endif
	print_rest(nxt_node, 0);
	to_be_completed = FALSE;
	break;
      }
    }

    // No annotation mark found - take all annotations below this node
    if (to_be_completed) {
#ifdef WEIGHTED
      if (weighted) {
	next_node = start.set_next_node(current_dict);
	fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
	if (next_node.is_last()) {
	  // the node has only one child
	  to_be_completed = handle_annot_arc(next_node, vanity_level);
	}
	else {
	  fsa_arc_ptr prev_node = next_node;
	  fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
	  ++next_node;
	  if (next_node.is_last()) {
	    // the node has two children
	    if (prev_node.get_weight() > next_node.get_weight()) {
	      // first node is heavier
	      handle_arc(prev_node, vanity_level);
	      // lighter node
	      handle_arc(next_node, vanity_level);
	    }
	    else {
	      // second node is heavier
	      handle_arc(next_node, vanity_level);
	      // lighter node
	      handle_arc(prev_node, vanity_level);
	    }
	  }
	  else {
	    // more than two nodes
	    // first put arcs in an additional table and sort them
	    int no_of_arcs = 2;
	    while (!(next_node.is_last())) {
	      ++next_node; no_of_arcs++;
	    }
	    int *arc_indx = new int[no_of_arcs];
	    int *weights = new int[no_of_arcs];
	    next_node = start.set_next_node(current_dict);
	    int kk = 0;
	    forallnodes(kk) {
	      weights[kk] = next_node.get_weight();
	      arc_indx[kk] = kk;
	      kk++;
	    }
	    // crude sorting, but should be enough here and avoids problems
	    // with scope
	    for (int ii = 0; ii < no_of_arcs - 1; ii++) {
	      int x = weights[arc_indx[ii]];
	      for (int jj = ii + 1; jj < no_of_arcs; jj++) {
		if (x < weights[arc_indx[jj]]) {
		  x = weights[arc_indx[jj]];
		  int xj = arc_indx[ii];
		  arc_indx[ii] = arc_indx[jj];
		  arc_indx[jj] = xj;
		}
	      }
	    }
	    // now process the arcs in the order specified by arc_indx
	    next_node = start.set_next_node(current_dict);
	    for (int kk = 0; kk < no_of_arcs; kk++) {
	      prev_node = next_node.arc + arc_indx[kk] * next_node.size;
	      handle_annot_arc(prev_node, vanity_level);
	    }
	    delete [] weights;
	    delete [] arc_indx;
	  }
	}
      }
      else {
#endif //!WEIGHTED
      next_node = start.set_next_node(current_dict);
      forallnodes(l) {
	// Now it is possible that there is an arc with annotation separator
	// at this node, and it should be taken into account!
	if (next_node.get_letter() == ANNOT_SEPARATOR) {
	  to_be_completed = FALSE;
	  handle_annot_arc(next_node, vanity_level);
	}
      }
      if (to_be_completed) {
	next_node = start.set_next_node(current_dict);
	forallnodes(k) {
	  guess("", next_node, vanity_level + 1);
	}
      }
#ifdef WEIGHTED
      }
#endif //!WEIGHTED
    }
  }
  return replacements.how_many();
}//guess_fsa::guess
#endif //!(FLEXIBLE&STOPBIT&SPARSE)
  

/* Name:	print_rest
 * Class:	guess_fsa
 * Purpose:	Prints all strings from the automaton that start at a specified
 *		node.
 * Parameters:	start_node	- (i) look at that node;
 *		level		- (i) how many nodes are from the first one
 *					to this one.
 * Returns:	Number of items found in the part of the automaton
 *		starting in start.
 * Remarks:	level specifies how many characters there are already
 *		in the candidate string.
 *
 *		For mmorph guessing, we perform the interpretation
 *		of the annotations after we put them into candidate.
 *		The reason is the speed and ease of processing.
 *		What we have in the candidate buffer while entering here
 *		is the canonical form (already expanded), and then:
 *		+K2K3K4a1+e2+categories.
 *		Note that we need categories (i.e. morphological descriptions)
 *		first, and then the lexical and the canonical form.
 *		The contents of the mmorph_buffer should be:
 *		categories "lexical_form" = "canonical form"
 */
int
guess_fsa::print_rest(fsa_arc_ptr start, const int level)
{
  fsa_arc_ptr next_node = start;
  int already_found = replacements.how_many();
#ifdef GUESS_MMORPH
  char *morph_desc_index;	// position of descriptions in candidate
  char *arch_desc_index;	// position of K2 in candidate
  char *lexical_ending_index = NULL;	// position of e2 in candidate
  int	K2, K3, K4=0;		// codes present in lexicon (see above)
  int	arch_len = 0;		// length of an archiphoneme
#endif

  if (level > MAX_NOT_CYCLE) {
    cerr << "Possible cycle detected. Exiting." << endl;
    exit(5);
  }

  if (replacements.how_many() <= MAX_GUESSES) {
    if (level + 1 >= cand_alloc)
      grow_string(candidate, cand_alloc, Max_word_len);
    forallnodes(i) {
      candidate[level] = next_node.get_letter();
      if (next_node.is_final()) {
	candidate[level + 1] = '\0';
#ifdef GUESS_MMORPH
	if (guess_mmorph) {
	  lexical_ending_index = NULL;
	  // descriptions are the categories
	  // find the first ANNOT_SEPARATOR
	  arch_desc_index = strchr(candidate, ANNOT_SEPARATOR);
	  arch_desc_index++;	// move past separator
	  if ((K3 = (arch_desc_index[1] - 'A')) != 0) {
	    // there is an archiphoneme
	    lexical_ending_index = strchr(arch_desc_index, ANNOT_SEPARATOR);
	    lexical_ending_index++; // move past the separator
	    morph_desc_index = strchr(lexical_ending_index, ANNOT_SEPARATOR);
	  }
	  else {
	    // there is no archiphoneme
	    morph_desc_index = strchr(arch_desc_index, ANNOT_SEPARATOR);
	  }
	  morph_desc_index++;	// move past the separator
	  // copy information into mmorph buffer in correct places
	  // adding quotes, etc.
	  // first check if we have enough place in mmorph_buffer
	  // we need place for:
	  // morphological descriptions = length(morph_desc_index)
	  // space & double quote = 2
	  // lexical form = word_length - code(K2)
	  //	+ morph_desc_index - lexical_ending_index - 1 - code(K4)
	  //	+ length(archiphoneme)
	  //	when there is an archiphoneme
	  // lexical form = word_length - code(K2)
	  //	+ morph_desc_index - arch_desc_index - 2
	  // double quote & space & = & space & double quote = 5
	  // canonical form = (arch_desc_index - candidate)
	  // double quote = 1
	  // TOGETHER = ?
	  int desc_len = level - (morph_desc_index - candidate) + 1;
	  		// = strlen(morph_desc_index);
	  int l = desc_len	// description
	    + word_length - (K2 = *arch_desc_index - 'A') // lexical stem
	    + (arch_desc_index - 1 - candidate)	// canonical form
	    + 6;		// spaces, quotes and equal sign
	  if (K2 < arch_desc_index - candidate - 1) {
	    if (K3) {
	      // there is an archiphoneme
	      // we need lexical ending & archiphoneme
	      K4 = arch_desc_index[2] - 'A';
	      l += (morph_desc_index - arch_desc_index - 4 - K4) + 2;
	    }
	    else {
	      // there is no archiphoneme => K4a1+ not present
	      l += (morph_desc_index - arch_desc_index - 3);
	    }
	    if (l >= mmorph_alloc)
	      grow_string(mmorph_buffer, mmorph_alloc, l);
	    // now we put the descriptions
	    // they are the last, so they have '\0' at the end
	    strcpy(mmorph_buffer, morph_desc_index);
	    // then space and double quote
	    strcpy(mmorph_buffer+desc_len, " \"");
	    // then the lexical form
	    // first stem
	    l = desc_len + 2;
	    if (arch_desc_index - candidate - 1 - K2 > 0)
	      memcpy(mmorph_buffer + l, candidate,
		     arch_desc_index - candidate - 1 - K2);
	    l += arch_desc_index - candidate - 1 - K2;
	    mmorph_buffer[l] = '\0';
	    if (K3--) {
	      // there is an archiphoneme
	      // now K3 indicates the position of the archiphoneme
	      // make room for it
	      if (K3 - K4) {
		arch_len = lexical_ending_index - arch_desc_index - 4;
		// note that it assumes that length(archiphoneme) > K4
#ifdef LOOSING_RPM
		char *p = mmorph_buffer + desc_len + 2 + K3;
		int k = strlen(p);
		for (p += k - 1; k; --k, --p) {
		  p[arch_len + 2] = *p;
		}
#else
		memmove(mmorph_buffer + desc_len + 2 + K3,
			mmorph_buffer + desc_len + 4 + K3 + arch_len,
			//(lexical_ending_index - arch_desc_index) - 4);
			strlen(mmorph_buffer + desc_len + 2 + K3));
#endif
	      }
	      // copy the archiphoneme
	      mmorph_buffer[desc_len + 2 + K3] = '&';
	      memcpy(mmorph_buffer + desc_len + K3 + 3, arch_desc_index + 3,
		      lexical_ending_index - arch_desc_index - 4);
	      mmorph_buffer[desc_len + K3 +
			   arch_len + 3] = ';';
	      l += lexical_ending_index - arch_desc_index - 1 - K4;
	    }
	    // now copy the ending
	    if (lexical_ending_index) {
	      if (*lexical_ending_index != ANNOT_SEPARATOR) {
		memcpy(mmorph_buffer + l, lexical_ending_index,
		       morph_desc_index - lexical_ending_index - 1);
		l += morph_desc_index - lexical_ending_index - 1;
	      }
	    }
	    else if (arch_desc_index[2] != ANNOT_SEPARATOR) {
	      memcpy(mmorph_buffer + l, arch_desc_index + 2,
		      morph_desc_index - arch_desc_index - 3);
	      l += morph_desc_index - arch_desc_index - 3;
	    }
	    if (strncmp(arch_desc_index, "AA+", 3)) {
	      // then double quote, space, equal sign, space, and double quote
	      strcpy(mmorph_buffer + l, "\" = \"");
	      l += 5;
	      // then the canonical form
	      memcpy(mmorph_buffer + l, candidate,
		     arch_desc_index - candidate - 1);
	      l += arch_desc_index - candidate - 1;
	    }
	    // and the closing double quote
	    strcpy(mmorph_buffer + l, "\"");
	    replacements.insert_sorted(mmorph_buffer);
	  }//if K2 small enough
	}
	else {
#endif //GUESS_MMORPH
	  replacements.insert_sorted(candidate);
#ifdef GUESS_MMORPH
	}
#endif
      }
      if (next_node.get_goto() != 0)
	print_rest(next_node.set_next_node(current_dict), level + 1);
    }
  }
  return replacements.how_many() - already_found;
}//guess_fsa::print_rest

#ifdef GUESS_LEXEMES
/* Name:	guess_stem
 * Class:	guess_fsa
 * Purpose:	Computes the stem of the lexeme of the inflected word.
 * Parameters:	start		- (i) look at that node;
 *		start_char	- (i) length of the prefix, or length
 *					of the infix and length of all
 *					characters that precede the infix;
 *		infix_length	- (i) length of the infix (if found);
 * Returns:	Number of different hypothetical morphological analyses
 *		of the word in the part of the automaton reachable from start.
 * Remarks:	The character in the nodes at this stage is a coded number.
 *		It encodes the number of characters that should be deleted
 *		from the end of the word before a new ending is appended.
 *		The number is computed as the code of the character minus 65
 *		(the code of the letter 'A').
 *
 *		I have changed the condition for allowing a new guess
 *		so that at least one character from the original word
 *		must be kept. This is to limit the number of possible
 *		choices, but it prevents the function from recognizing
 *		correctly some irregular words.
 */
int
guess_fsa::guess_stem(fsa_arc_ptr start, const int start_char,
		      const int infix_length)
{
  fsa_arc_ptr next_node = start;
  int reject_from_word;
  int already_found = replacements.how_many();

  if (word_length + 1 >= cand_alloc)
    grow_string(candidate, cand_alloc, Max_word_len);
  forallnodes(i) {
    if ((reject_from_word = (next_node.get_letter() - 'A')) >= 0 &&
	reject_from_word + start_char < word_length) {
      strncpy(candidate, word_ff + reject_from_word, word_length - start_char);
      if (infix_length) {
	// copy what precedes infix
	strncpy(candidate + word_length - reject_from_word - start_char,
		word_ff + word_length - start_char + infix_length,
		start_char - infix_length);
	candidate[word_length - reject_from_word - infix_length] = '\0';
      }
      else
	candidate[word_length - reject_from_word - start_char] = '\0';
      invert(candidate);
      print_rest(next_node.set_next_node(current_dict),
		 word_length - reject_from_word -
		 (infix_length ? infix_length : start_char));
    }
  }
  return replacements.how_many() - already_found;
}//guess_fsa::guess_stem
#endif 

#ifdef GUESS_PREFIX
/* Name:	check_prefix
 * Class:	guess_fsa
 * Purpose:	Check if the word contains a prefix.
 * Parameters:	start		- (i) look at that node;
 *		char_no		- (i) index of the character checked.
 * Returns:	The number of different morphological analyses found
 *		in the part of the automaton reachable from start.
 * Remarks:	The word in word_ff is inverted.
 *
 *		A suffix (a sequence of character at the end) of the word
 *		was recognized, and the next character in the automaton
 *		is the annotation separator. What can be next is:
 *		1) another annotation separator, meaning that there are
 *			no prefixes to be recognized;
 *		2) other letters beginning prefixes;
 *		3) both of the above.
 *
 *		All prefixes must be recognized in full. The filler character
 *		serves as the end marker for the prefix.
 */
int
guess_fsa::check_prefix(fsa_arc_ptr start, const int char_no)
{
  fsa_arc_ptr next_node = start;
  int prefixes_found = 0;
  int already_found = replacements.how_many();

  forallnodes(i) {
    if (char_no >= word_length)
      break;
    if (next_node.get_letter() == word_ff[word_length - char_no - 1]) {
      prefixes_found = check_prefix(next_node.set_next_node(current_dict),
				    char_no + 1);
      break;
    }
  }

  if (prefixes_found == 0) {
    // We got here, because no character on arcs leaving this node
    // can be found in prefix
    next_node = start;
    forallnodes(j) {
      if (next_node.get_letter() == ANNOT_SEPARATOR) {
	// this is either an annotation separator at the end of a prefix
	// or an annotation separator instead of a prefix
	if (guess_infix)
	  prefixes_found += check_infix(next_node.set_next_node(current_dict),
					char_no);
#ifdef GUESS_LEXEMES
	else if (guess_lexemes)
	  prefixes_found += guess_stem(next_node.set_next_node(current_dict),
				       char_no, 0);
#endif
	else
	  prefixes_found += print_rest(next_node.set_next_node(current_dict),
				       0);
      }
    }
  }
  return prefixes_found - already_found;
}//guess_fsa::check_prefix

/* Name:	check_infix
 * Class:	guess_fsa
 * Purpose:	Check if the word contains an infix.
 * Parameters:	start		- (i) look at that node;
 *		char_no		- (i) index of the character checked.
 * Returns:	The number of different morphological analyses found
 *		in the part of the automaton reahcable from start.
 * Remarks:	The word contains prefix if the next character is `A'.
 *		Otherwise the word has an infix, and its length can be
 *		calculated as the character code - the character code of `A'.
 */
int
guess_fsa::check_infix(fsa_arc_ptr start, const int char_no)
{
  fsa_arc_ptr next_node = start;
  int infix_length;
  int infixes_found = 0;

  forallnodes(i) {
    if ((infix_length = (next_node.get_letter() - 'A')) >= 0 &&
	infix_length < word_length - char_no){
      infixes_found += guess_stem(next_node.set_next_node(current_dict),
				  char_no, infix_length);
    }
  }
  return infixes_found;
}//guess_fsa::check_infix

#endif

/***	EOF guess.cc	***/

