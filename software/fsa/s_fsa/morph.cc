/***	morph.cc	***/

/*	Copyright (C) Jan Daciuk, 1997-2004	*/


#include        <iostream>
#include	<string.h>
#include        "nstr.h"
#include        "fsa.h"
#include        "common.h"
#include        "morph.h"
      

/* Name:        morph_fsa
 * Class:       morph_fsa
 * Purpose:     Constructor.
 * Parameters:  dict_list       - (i) list of dictionary (fsa) names;
 *		language_file	- (i) file with character set.
 * Returns:     Nothing (constructor).
 * Remarks:     Only to launch fsa contructor.
 */
#ifdef MORPH_INFIX
#ifdef POOR_MORPH
morph_fsa::morph_fsa(const int ignorefiller,
		     const int no_baseforms, const int file_has_infixes,
		     const int file_has_prefixes, word_list *dict_list,
		     const char *language_file = NULL)
#else
morph_fsa::morph_fsa(const int ignorefiller,
		     const int file_has_infixes, const int file_has_prefixes,
		     word_list *dict_list, const char *language_file = NULL)
#endif
#else
#ifdef POOR_MORPH
morph_fsa::morph_fsa(const int ignorefiller,
		     const int no_baseforms, word_list *dict_list,
		     const char *language_file = NULL)
#else
morph_fsa::morph_fsa(const int ignorefiller,
		     word_list *dict_list, const char *language_file = NULL)
#endif
#endif
: fsa(dict_list, language_file)
{
#ifdef MORPH_INFIX
  morph_infixes = file_has_infixes;
  morph_prefixes = file_has_prefixes;
#endif
#ifdef POOR_MORPH
  only_categories = no_baseforms;
#endif
  ignore_filler = ignorefiller;
}//morph_fsa::morph_fsa
 


/* Name:	morph_file
 * Class:	morph_fsa
 * Purpose:	Perform morphological analysis on all words in a file.
 * Parameters:	io_obj		- (i/o) where to read words,
 *					and where to print analyses;
 * Returns:	Exit code.
 * Remarks:	None.
 */
int
morph_fsa::morph_file(tr_io &io_obj)
{
  int		allocated;
  char		*word;

  word = new char[allocated = Max_word_len];
  while (get_word(io_obj, word, allocated, Max_word_len)) {
    word_length = strlen(word); word_ff = word;
    if (morph_word(word)) {
      io_obj.print_repls(&replacements);
      replacements.empty_list();
    }
    else
      io_obj.print_not_found();
  }
  return state;
}//morph_fsa::morph_file




/* Name:	morph_word
 * Class:	morph_fsa
 * Purpose:	Perform morphological analysis of a word
 *		using all specified dictionaries.
 * Parameters:	word	- (i) word to be checked.
 * Returns:	Number of different analyses of the word.
 * Remarks:	Class variable `replacements' is set to the list
 *		of possible analyses.
 */
int
morph_fsa::morph_word(const char *word)
{
  dict_list		*dict;
#if !(defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE))
  fsa_arc_ptr		*dummy = NULL;
#endif
#ifdef CASECONV
  int			converted = FALSE;
#endif

  dictionary.reset();
  for (dict = &dictionary; dict->item(); dict->next()) {
    ANNOT_SEPARATOR = dict->item()->annot_sep;
    set_dictionary(dict->item());
#ifdef CASECONV
    converted = FALSE;
    if (is_downcaseable(word)) {
      // word is uppercase - convert to lowercase
      myflipcase((char *)word, -1);
      //*((char *)word) = casetab[(unsigned char)*word];
      converted = *word;
    }
#endif
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
    sparse_morph_next_char(word, 0, sparse_vect->get_first());
#else
    morph_next_char(word, 0, dummy->first_node(current_dict));
#endif
#ifdef CASECONV
    if (converted) {
      // convert back to uppercase
      myflipcase((char *)word, 1);
      //*((char *)word) = casetab[(unsigned char)*word];
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
      sparse_morph_next_char(word, 0, sparse_vect->get_first());
#else
      morph_next_char(word, 0, dummy->first_node(current_dict));
#endif
    }
#endif
  }

#ifdef CASECONV
  if (converted) {
    // word was uppercase
    replacements.reset();
    for (;replacements.item();replacements.next()) {
      // Covert back to uppercase
      myflipcase((char *)word, 1);
    }
  }
#endif

  return replacements.how_many();
}//morph_fsa::morph_word


#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
/* Name:	sparse_morph_next_char
 * Class:	morph_fsa
 * Purpose:	Consider the next node in morphological analysis.
 * Parameters:	word	- (i) word to look for;
 *		level	- (i) how many characters of the word have been
 *				considered so far;
 *		start	- (i) look at that node.
 * Returns:	Number of different analyses of the word.
 * Remarks:	Class variable `replacements' is set to the list
 *		of different analyses of the word.
 *		Entries in the dictionary have the following format:
 *		inflected_word+Kending+tags
 *		where:
 *		inflected_word is an inflected form of a word,
 *		K specifies how many characters from the end of inflected_word
 *			do not match those from the respective lexeme; that
 *			number is computed as K - 'A',
 *		ending is the ending of lexeme,
 *		tags is a set of categories (annotation) of the inflected form.
 *		Example:
 *		cried+Dy+Vpp means that lexeme is cry, and tags are Vpp
 *		('D' - 'A' = 3, "cried" - 3 letters at end = "cr")
 */
int
morph_fsa::sparse_morph_next_char(const char *word, const int level,
				  const long start)
{
  bool found = false;
  int lev = level;
  long current = start;
  long next;
  fsa_arc_ptr *dummy;
  do {
    found = false;
    if (*word == '\0') {
      if ((next = sparse_vect->get_target(current, ANNOT_SEPARATOR)) != -1L) {
#ifdef MORPH_INFIX
#ifdef POOR_MORPH
	if (only_categories) {
	  if (lev >= cand_alloc)
	    grow_string(candidate, cand_alloc, Max_word_len);
	  strcpy(candidate, word_ff);
	  candidate[lev] = ANNOT_SEPARATOR;
	  candidate[lev + 1] = '\0';
	  morph_rest(lev + 1, current_dict + next
#ifdef NUMBERS
		     + dummy->entryl
#endif
		     );
	}
	else
#endif
	  if (morph_infixes)
	    morph_infix(lev, current_dict + next
#ifdef NUMBERS
			+ dummy->entryl
#endif
			);
	  else if (morph_prefixes)
	    morph_prefix(0, lev, current_dict + next
#ifdef NUMBERS
			 + dummy->entryl
#endif
			 );
	  else
	    morph_stem(0, 0, lev, current_dict + next
#ifdef NUMBERS
			+ dummy->entryl
#endif
		       );
#else
#ifdef POOR_MORPH
	if (only categories) {
	  if (lev >= cand_alloc)
	    grow_string(candidate, cand_alloc, Max_word_len);
	  strcpy(candidate, word_ff);
	  candidate[lev] = ANNOT_SEPARATOR;
	  candidate[lev + 1] = '\0';
	  morph_rest(lev, current_dict + next
#ifdef NUMBERS
		     + dummy->entryl
#endif
		     );
	}
	else {
#endif
	  morph_stem(lev, current_dict + next
#ifdef NUMBERS
		     + dummy->entryl
#endif
		     );
#ifdef POOR_MORPH
	}
#endif
#endif
      }
    }
    else {
      if ((next = sparse_vect->get_target(current, *word)) != -1L) {
	if (lev >= cand_alloc)
	  grow_string(candidate, cand_alloc, Max_word_len);
	word++;
	lev++;
	current = next;
	found = true;
      }
    }
  } while (found);
  return replacements.how_many();
}//morph_fsa::sparse_morph_next_char

#else //!(FLEXIBLE&STOPBIT&NEXTBIT)

/* Name:	morph_next_char
 * Class:	morph_fsa
 * Purpose:	Consider the next node in morphological analysis.
 * Parameters:	word	- (i) word to look for;
 *		level	- (i) how many characters of the word have been
 *				considered so far;
 *		start	- (i) look at children of that node.
 * Returns:	Number of different analyses of the word.
 * Remarks:	Class variable `replacements' is set to the list
 *		of different analyses of the word.
 *		Entries in the dictionary have the following format:
 *		inflected_word+Kending+tags
 *		where:
 *		inflected_word is an inflected form of a word,
 *		K specifies how many characters from the end of inflected_word
 *			do not match those from the respective lexeme; that
 *			number is computed as K - 'A',
 *		ending is the ending of lexeme,
 *		tags is a set of categories (annotation) of the inflected form.
 *		Example:
 *		cried+Dy+Vpp means that lexeme is cry, and tags are Vpp
 *		('D' - 'A' = 3, "cried" - 3 letters at end = "cr")
 */
int
morph_fsa::morph_next_char(const char *word, const int level,
			   fsa_arc_ptr start)
{
  bool found = false;
  int lev = level;
  do {
    found = false;
    fsa_arc_ptr next_node = start.set_next_node(current_dict);
    if (*word == '\0') {
      forallnodes(i) {
	if (next_node.get_letter() == ANNOT_SEPARATOR) {
	  fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
#ifdef MORPH_INFIX
#ifdef POOR_MORPH
	  if (only_categories) {
	    if (lev >= cand_alloc)
	      grow_string(candidate, cand_alloc, Max_word_len);
	    strcpy(candidate, word_ff);
	    candidate[lev] = ANNOT_SEPARATOR;
	    candidate[lev + 1] = '\0';
	    morph_rest(lev + 1, nxt_node);
	  }
	  else
#endif
	    if (morph_infixes)
	      morph_infix(lev, nxt_node);
	    else if (morph_prefixes)
	      morph_prefix(0, lev, nxt_node);
	    else
	      morph_stem(0, 0, lev, nxt_node);
#else
#ifdef POOR_MORPH
	  if (only categories) {
	    if (lev >= cand_alloc)
	      grow_string(candidate, cand_alloc, Max_word_len);
	    strcpy(candidate, word_ff);
	    candidate[lev] = ANNOT_SEPARATOR;
	    candidate[lev + 1] = '\0';
	    morph_rest(lev, nxt_node);
	  }
	  else {
#endif
	    morph_stem(lev, nxt_node);
#ifdef POOR_MORPH
	  }
#endif
#endif
	  break;
	}
      }
    }
    else {
      forallnodes(j) {
	if (*word == next_node.get_letter()) {
	  if (lev >= cand_alloc)
	    grow_string(candidate, cand_alloc, Max_word_len);
	  word++;
	  lev++;
	  start = next_node;
	  found = true;
	  break;
	}
      }
    }
  } while (found);
  return replacements.how_many();
}//morph_fsa::morph_next_char
#endif //!(FLEXIBLE&STOPBIT&NEXTBIT)

#ifdef MORPH_INFIX

/* Name:	morph_infix
 * Class:	morph_fsa
 * Purpose:	Establish whether the inflected form has an infix,
 *		and locate it.
 * Parameters:	level	- (i) how many characters there are
 *				in the inflected form;
 *		start	- (i) look at that node.
 * Returns:	Number of different analysis available in the analysed part
 *		of the automaton.
 * Remarks:	Class variable `replacements' is set to the list
 *		of different analyses of the word.
 *		Entries in the dictionary have the following format:
 *		inflected_word+MLKending+tags
 *		where:
 *		inflected_word is an inflected form of a word,
 *		M specifies whether the word has an infix, and locates it:
 *			"A" means there is no infix, "B" - the infix begins
 *			at the second character of the form, "C" - at the 3rd,
 *			"D" - at the fourth, and so on.
 *		L specifies the length of a prefix (in case M="A") or infix.
 *			"A" means there is no prefix or infix, "B" - it is
 *			one character long, "C" - 2 characters, and so on.
 *		K specifies how many characters from the end of inflected_word
 *			do not match those from the respective lexeme; that
 *			number is computed as K - 'A',
 *		ending is the ending of lexeme,
 *		tags is a set of categories (annotation) of the inflected form.
 *		Example:
 *		to come.
 */
int
morph_fsa::morph_infix(const int level, fsa_arc_ptr start)
{
//  fsa_arc_ptr next_node = start.set_next_node(current_dict);
  fsa_arc_ptr next_node = start;
  int	delete_position;

  if (level >= cand_alloc)
    grow_string(candidate, cand_alloc, Max_word_len);

  forallnodes(i) {
    if ((delete_position = (next_node.get_letter() - 'A')) >= 0 &&
	delete_position < word_length) {
      fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
      morph_prefix(delete_position, level, nxt_node);
    }
  }
  return replacements.how_many();
}//morph_fsa::morph_infix


/* Name:	morph_prefix
 * Class:	morph_fsa
 * Purpose:	Establish how many characters from the beginning part
 *		of the inflected word must be deleted to form the base form.
 * Parameters:	delete_position	- (i) where the characters to be deleted are;
 *		level		- (i) how many characters there are
 *					in the inflected form;
 *		start		- (i) look at this node.
 * Returns:	The number of different morphological analyses in the analysed
 *		part of the automaton.
 * Remarks:	Class variable `replacements' is set to the list
 *		of different analyses of the word.
 */
int
morph_fsa::morph_prefix(const int delete_position, const int level,
			fsa_arc_ptr start)
{
  fsa_arc_ptr next_node = start;
  int	delete_length;

  if (level >= cand_alloc)
    grow_string(candidate, cand_alloc, Max_word_len);

  forallnodes(i) {
    if ((delete_length = (next_node.get_letter() - 'A')) >= 0 &&
	delete_length < word_length) {
      fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
      morph_stem(delete_length, delete_position, level, nxt_node);
    }
  }
  return replacements.how_many();
}//morph_fsa::morph_prefix

#endif


/* Name:	morph_stem
 * Class:	morph_fsa
 * Purpose:	Establish how many characters from the end of the inflected
 *		form must be deleted to form the lexeme (with possibly some
 *		new characters).
 * Parameters:	level	- (i) how many characters there are
 *				in the inflected form;
 *		start	- (i) look at that node.
 * Returns:	Number of different analysis available in the analysed part
 *		of the automaton.
 * Remarks:	Class variable `replacements' is set to the list
 *		of different analyses of the word.
 *		Entries in the dictionary have the following format:
 *		inflected_word+Kending+tags
 *		where:
 *		inflected_word is an inflected form of a word,
 *		K specifies how many characters from the end of inflected_word
 *			do not match those from the respective lexeme; that
 *			number is computed as K - 'A',
 *		ending is the ending of lexeme,
 *		tags is a set of categories (annotation) of the inflected form.
 *		Example:
 *		cried+Dy+Vpp means that lexeme is cry, and tags are Vpp
 *		('D' - 'A' = 3, "cried" - 3 letters at end = "cr")
 */
#ifdef MORPH_INFIX
int
morph_fsa::morph_stem(const int delete_length, const int delete_position,
		      const int level, fsa_arc_ptr start)
#else
int
morph_fsa::morph_stem(const int level, fsa_arc_ptr start)
#endif
{
  fsa_arc_ptr next_node = start;
  int reject_from_word;

  if (level >= cand_alloc)
    grow_string(candidate, cand_alloc, Max_word_len);

  forallnodes(i) {
    if ((reject_from_word = (next_node.get_letter() - 'A')) >= 0 &&
	reject_from_word <= word_length) {
#ifdef MORPH_INFIX
      if (delete_length > 0) {
	if (delete_position) {
	  // infix
	  // copy the word without ending
	  strncpy(candidate, word_ff, word_length - reject_from_word);
	  // remove the infix
	  memmove(candidate + delete_position,
		  candidate + delete_position + delete_length,
		  word_length - delete_position - delete_length);
	}
	else {
	  // prefix
	  // copy the word without the prefix and without ending
	  strncpy(candidate, word_ff + delete_length,
		  word_length - reject_from_word - delete_length);
	}
      }
      else
#endif
      strncpy(candidate, word_ff, word_length - reject_from_word);
      if (next_node.is_final()) {
#ifdef MORPH_INFIX
	candidate[level - reject_from_word - delete_length] = '\0';
#else
	candidate[level - reject_from_word] = '\0';
#endif
//	candidate[level + 1] = '\0';
	replacements.insert_sorted(candidate);
      }
      fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
#ifdef MORPH_INFIX
      morph_rest(level - reject_from_word - delete_length, nxt_node);
#else
      morph_rest(level - reject_from_word, nxt_node);
#endif
    }
  }
  return replacements.how_many();
}//morph_fsa::morph_stem


/* Name:	morph_rest
 * Class:	morph_fsa
 * Purpose:	Append lexeme ending and inflected form tags at the end
 *		of the candidate.
 * Parameters:	level	- (i) how many characters there are so far
 *				in the candidate;
 *		start	- (i) look at this node.
 * Returns:	The number of different morphological analyses in the analysed
 *		part of the automaton.
 * Remarks:	Class variable `replacements' is set to the list
 *		of different analyses of the word.
 *		Entries in the dictionary have the following format:
 *		inflected_word+Kending+tags
 *		where:
 *		inflected_word is an inflected form of a word,
 *		K specifies how many characters from the end of inflected_word
 *			do not match those from the respective lexeme; that
 *			number is computed as K - 'A',
 *		ending is the ending of lexeme,
 *		tags is a set of categories (annotation) of the inflected form.
 *		Example:
 *		cried+Dy+Vpp means that lexeme is cry, and tags are Vpp
 *		('D' - 'A' = 3, "cried" - 3 letters at end = "cr")
 */
int
morph_fsa::morph_rest(const int level, fsa_arc_ptr start)
{
  fsa_arc_ptr next_node = start;

  if (level >= cand_alloc)
    grow_string(candidate, cand_alloc, Max_word_len);

  if (start.arc != current_dict) {
    forallnodes(i) {
      if (!ignore_filler || next_node.get_letter() != FILLER) {
	candidate[level] = next_node.get_letter();
      }
      if (next_node.is_final()) {
	candidate[level + 1] = '\0';
	replacements.insert_sorted(candidate);
      }
      fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
      morph_rest(level + 1, nxt_node);
    }
  }
  return replacements.how_many();
}//morph_fsa::morph_rest




/***	EOF morph.cc	***/
