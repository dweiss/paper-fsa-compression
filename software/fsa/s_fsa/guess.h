/***	guess.h		***/

/*	Copyright (c) Jan Daciuk, 1996-2004	*/

const int MAX_VANITY_LEVEL = 5;	/* how many letters to go without suffix */

class guess_fsa : public fsa {
protected:
  int guess_lexemes;		/* whether dictionary contains lexemes */
  int guess_prefix;		/* whether dictionary contains prefixes */
  int guess_infix;		/* whether dictionary contains infixes */
  int guess_mmorph;		/* whether dictionary contains mmorph
				   descriptions */
#ifdef GUESS_MMORPH
  char *mmorph_buffer;		/* buffer for mmorph descriptions */
  int  mmorph_alloc;		/* size of the allocated mmorph buffer */
#endif
public:
  guess_fsa(word_list *dict_names, const int lexemes_in_dict,
	    const int prefixes_in_dict, const int infixes_in_dict,
	    const int mmorph_in_dict, const char *language_file=NULL);
  virtual ~guess_fsa(void) {}
  int guess_file(tr_io &io_obj);
  int guess_word(const char *word);
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
  int sparse_guess(const char *word, const long start, const int vanity_level);
#else
  int guess(const char *word, fsa_arc_ptr start, const int vanity_level);
#endif
  int print_rest(fsa_arc_ptr start, const int level);
#ifdef GUESS_LEXEMES
  int guess_stem(fsa_arc_ptr start, const int start_char,
		 const int infix_length);
#endif
#ifdef GUESS_PREFIX
  int check_prefix(fsa_arc_ptr start, const int char_no);
  int check_infix(fsa_arc_ptr start, const int char_no);
#endif
  int handle_arc(fsa_arc_ptr next_node, const int vanity_level);
  int handle_annot_arc(fsa_arc_ptr next_node, const int vanity_level);
};/*class guess_fsa*/

#if !(defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE))
/* Name:	handle_arc
 * Class:	guess_fsa
 * Purpose:	Decides what to do with an arc in guess when no matching
 *		letters found on arcs.
 * Parameters:	arc	- (i) arc to be processed.
 * Returns:	TRUE if arc has an annotation separator, FALSE otherwise.
 * Remarks:	None.
 */
inline int
guess_fsa::handle_arc(fsa_arc_ptr next_node, const int vanity_level)
{
  if (next_node.get_letter() == ANNOT_SEPARATOR) {
    handle_annot_arc(next_node, vanity_level);
    return FALSE;
  }
  else {
    guess("", next_node, vanity_level + 1);
    return TRUE;
  }
}/*handle_arc*/

/* Name:	handle_annot_arc
 * Class:	guess_fsa
 * Purpose:	Decides what to do with an arc in guess when no matching
 *		letters found on arcs.
 * Parameters:	arc	- (i) arc to be processed.
 * Returns:	TRUE if arc has an annotation separator, FALSE otherwise.
 * Remarks:	None.
 */
inline int
guess_fsa::handle_annot_arc(fsa_arc_ptr next_node, const int vanity_level)
{
  if (next_node.get_letter() == ANNOT_SEPARATOR) {
    fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
#ifdef GUESS_PREFIX
    if (guess_prefix || guess_infix)
      check_prefix(nxt_node, 0);
    else
#endif //GUESS_PREFIX
#ifdef GUESS_LEXEMES
      if (guess_lexemes)
	guess_stem(nxt_node, 0, 0);
      else
#endif //GUESS_LEXEMES
	guess("", nxt_node, vanity_level + 1);
    return FALSE;
  }
  else {
    fsa_arc_ptr xnt_node = next_node.set_next_node(current_dict);
    guess("", xnt_node, vanity_level + 1);
    return TRUE;
  }
}/*handle_annot_arc*/
#endif //!(FLEXIBLE&STOPBIT&SPARSE)

/***	EOF guess.fsa	***/
