/***	accent.cc	***/

/*	Copyright (C) Jan Daciuk, 1996-2004	*/


#include        <iostream>
#include	<string.h>
#include        "nstr.h"
#include        "fsa.h"
#include        "common.h"
#include        "accent.h"
      

/* Name:        accent_fsa
 * Class:       accent_fsa
 * Purpose:     Constructor.
 * Parameters:  dict_list       - (i) list of dictionary (fsa) names;
 *		language_file	- (i) file with character set.
 * Returns:     Nothing (constructor).
 * Remarks:     Only to launch fsa contructor.
 */
accent_fsa::accent_fsa(word_list *dict_list, const char *language_file = NULL)
: fsa(dict_list, language_file)
{
}//accent_fsa::accent_fsa
 


/* Name:	accent_file
 * Class:	accent_fsa
 * Purpose:	Restore accents in all words of a file.
 * Parameters:	io_obj		- (i/o) where to read words,
 *					and where to print them;
 *		equiv		- (i) character equivalent classes.
 * Returns:	Exit code.
 * Remarks:	Class variable `replacements' is set to the list of words
 *		equivalent to the `word'.
 */
int
accent_fsa::accent_file(tr_io &io_obj, const accent_tabs *equiv)
{
  int		allocated;
  char		*word;

  word = new char[allocated = Max_word_len];
  while (get_word(io_obj, word, allocated, Max_word_len)) {
    if (accent_word(word, equiv)) {
      io_obj.print_repls(&replacements);
      replacements.empty_list();
    }
    else
      io_obj.print_not_found();
  }
  delete [] word;
  return state;
}//accent_fsa::accent_file




/* Name:	accent_word
 * Class:	accent_fsa
 * Purpose:	Restore accents of a word using all specified dictionaries.
 * Parameters:	word	- (i) word to be checked;
 *		equiv		- (i/o) classes of equivalences for characters.
 * Returns:	Number of words in the list of equivalent words
 *		(stored in replacements).
 * Remarks:	Class variable `replacements' is set to the list of equivalent
 *		words.
 *		The table of equivalent characters contains 256 characters,
 *		and contains:
 *		for characters with diacritics:
 *			- corresponding character without diacritic;
 *		for characters without diacritics:
 *			- that character.
 *		Note: this can be used for other forms of equivalencies,
 *		not necessarily with diacritics.
 */
int
accent_fsa::accent_word(const char *word, const accent_tabs *equiv)
{
  dict_list		*dict;
  fsa_arc_ptr		*dummy = NULL;
#ifdef CASECONV
  int			converted = FALSE;
#endif

#if defined(UTF8) && !(defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE))
  char_eq = (accent_tabs *)equiv;
#else
  char_eq = equiv->accents;
  same_class = equiv->eqchs;
#endif
  dictionary.reset();
  for (dict = &dictionary; dict->item(); dict->next()) {
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
    sparse_word_accents(word, 0, sparse_vect->get_first());
#else
    fsa_arc_ptr nxt_node = dummy->first_node(current_dict);
    word_accents(word, 0, nxt_node.set_next_node(current_dict));
#endif
#ifdef CASECONV
    if (converted) {
      // convert back to uppercase
      myflipcase((char *)word, 1);
      //*((char *)word) = casetab[(unsigned char)*word];
      fsa_arc_ptr xnt_node = dummy->first_node(current_dict);
      word_accents(word, 0, xnt_node.set_next_node(current_dict));
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
}//accent_fsa::accent_word


#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
/* Name:	sparse_word_accents
 * Class:	accent_fsa
 * Purpose:	Find all words that have the same letters, but sometimes
 *		with diacritics.
 * Parameters:	word	- (i) word to look for;
 *		level	- (i) how many characters of the word have been
 *				considered so far;
 *		start	- (i) look at that node.
 * Returns:	Number of words on the list of words equivalent to the `word'.
 * Remarks:	Class variable `replacements' is set to the list of words
 *		equivalent to the `word'.
 *		The table of equivalent characters contains 256 characters,
 *		and contains:
 *		for characters with diacritics:
 *			- corresponding character without diacritic;
 *		for characters without diacritics:
 *			- that character.
 *		Note: this can be used for other forms of equivalencies,
 *		not necessarily with diacritics.
 */
int
accent_fsa::sparse_word_accents(const char *word, const int level,
				const long start)
{
  unsigned char	char_no;
  long current = start;
  long next;
  fsa_arc_ptr *dummy;

  if (level + 1 >= cand_alloc)
    grow_string(candidate, cand_alloc, Max_word_len);
  char_no = (unsigned char)(*word);
  if (same_class[char_no]) {
    char *cc = same_class[char_no];
    int l = strlen(cc);
#ifdef UTF8
    while (*cc && l > 0) {
      char *ccc = cc;
      int u = utf8len(*cc);
      cc += u; l -= u;
      int lev = level;
      bool f = false;
      while (*ccc && ccc != cc &&
	     (next = sparse_vect->get_target(current, *ccc)) != -1L) {
	if (lev + 1 >= cand_alloc)
	  grow_string(candidate, cand_alloc, Max_word_len);
	candidate[lev++] = *ccc;
	if (sparse_vect->is_final(current, *ccc)) {
	  f = true;
	}
      }
      if (ccc = cc) {
	if (word[1] == '\0' && f) {
	  candidate[lev + 1] = '\0';
	  replacements.insert_sorted(candidate);
	}
	else {
	  sparse_word_accents(word + 1, lev + 1, next);
	}
      }
    }
#else
#endif
    for (int i = 0; i < l; i++) {
      if ((next = sparse_vect->get_target(current, *cc))
	  != -1L) {
	candidate[level] = *cc;
	if (word[1] == '\0' && sparse_vect->is_final(current, *cc)) {
	  candidate[level + 1] = '\0';
	  replacements.insert_sorted(candidate);
	}
	else {
	  sparse_word_accents(word + 1, level + 1, next);
	}
      }
      cc++;
    }
  }
  else if ((next = sparse_vect->get_target(current, *word)) != -1L) {
    candidate[level] = *word;
    if (word[1] == '\0' && sparse_vect->is_final(current, *word)) {
      candidate[level + 1] = '\0';
      replacements.insert_sorted(candidate);
    }
    else if (*word != ANNOT_SEPARATOR) {
      sparse_word_accents(word + 1, level + 1, next);
    }
    else {
      word_accents(word + 1, level + 1, current_dict + next
#ifdef NUMBERS
		   + dummy->entryl
#endif
		   );
    }
  }
  return replacements.how_many();
}//accent_fsa::sparse_word_accents
#endif //FLEXIBLE&STOPBIT&SPARSE

#if defined(UTF8) && !(defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE))
/* Name:	word_accents_dia
 * Class:	accent_fsa
 * Purpose:	Checks whether a subsequent letter in a dictionary can be one
 *		of the letters with diacritics specified with dia.
 * Parameters:	word	- (i) word to look for;
 *		level	- (i) how many characters of the word (or rather from
 *				its replacement) have been considered so far;
 *		start	- (i) look at that node of the dictionary;
 *		dia	- (i) look at that node of the tree of UTF8 characters
 *				with diacritics equivalent to a character
 *				without diacritics at *word.
 * Returns:	Number of words on the list of words equivalent to the `word'.
 * Remarks:	Class variable `replacements' is set to the list of words
 *		equivalent to the `word'.
 *		
 */
int
accent_fsa::word_accents_dia(const char *word, const int level,
			     fsa_arc_ptr start, const unsigned int dia)
{
  fsa_arc_ptr next_node = start;
  unsigned char	char_no;

  forallnodes(i) {
    char_no = (unsigned char)next_node.get_letter();
    if (char_eq[dia + char_no].chr == char_no) {
      fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
      candidate[level] = char_no;
      if (char_eq[dia + char_no].follow == -1) {
	// There is an arc labeled with one of the equivalent characters
	// with diacritics
	word_accents(word + 1, level + 1, nxt_node);
      }
      else {
	// Check subsequent byte of the letter
	word_accents_dia(word, level + 1, nxt_node,
			 char_eq[dia + char_no].follow);
      }
    }
  }
  return replacements.how_many();
}//accent_fsa::word_accents_dia
#endif

/* Name:	word_accents
 * Class:	accent_fsa
 * Purpose:	Find all words that have the same letters, but sometimes
 *		with diacritics.
 * Parameters:	word	- (i) word to look for;
 *		level	- (i) how many characters of the word have been
 *				considered so far;
 *		start	- (i) look at that node.
 * Returns:	Number of words on the list of words equivalent to the `word'.
 * Remarks:	Class variable `replacements' is set to the list of words
 *		equivalent to the `word'.
 *		The table of equivalent characters contains 256 characters,
 *		and contains:
 *		for characters with diacritics:
 *			- corresponding character without diacritic;
 *		for characters without diacritics:
 *			- that character.
 *		Note: this can be used for other forms of equivalencies,
 *		not necessarily with diacritics.
 */
int
accent_fsa::word_accents(const char *word, const int level, fsa_arc_ptr start)
{
  fsa_arc_ptr next_node = start;

  if (level + 1 >= cand_alloc)
    grow_string(candidate, cand_alloc, Max_word_len);
#if defined(UTF8) && !(defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE))
  // Check characters with diacritics
  if (char_eq[(unsigned char)(*word)].chr == (unsigned char)(*word)) {
    // Possible UTF8 character without diacritics that has equivalent characters
    // with diacritics
    const unsigned char *w = (const unsigned char *)word;
    unsigned int b = 0;
    while (*w && char_eq[b + *w].chr == *w && char_eq[b + *w].follow != -1) {
      b = char_eq[b + *w++].follow;
    }
    if (*w && char_eq[b + *w].chr == *w && char_eq[b + *w].repl != -1) {
      word_accents_dia((const char *)w, level, start,
		       (unsigned int)char_eq[b + *w].repl);
    }
  }
  // Check whether the current character stands for itself
  forallnodes(i) {
    if (*word == next_node.get_letter()) {
      candidate[level] = next_node.get_letter();
      if (word[1] == '\0' && next_node.is_final()) {
	candidate[level + 1] = '\0';
	replacements.insert_sorted(candidate);
      }
      else {
	fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
	word_accents(word + 1, level + 1, nxt_node);
      }
    }
  }
#else
  unsigned char	char_no;
  forallnodes(i) {
    char_no = (unsigned char)(next_node.get_letter());
    if (*word == char_eq[char_no]) {
      candidate[level] = next_node.get_letter();
      if (word[1] == '\0' && next_node.is_final()) {
	candidate[level + 1] = '\0';
	replacements.insert_sorted(candidate);
      }
      else {
	fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
	word_accents(word + 1, level + 1, nxt_node);
      }
    }
  }
#endif
  return replacements.how_many();
}//accent_fsa::word_accents



/***	EOF accent.cc	***/
