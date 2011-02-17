/***	morph.h		***/

/*	Copyright (c) Jan Daciuk, 1996-2004	*/

class morph_fsa : public fsa {
protected:
#ifdef MORPH_INFIX
  int	morph_infixes;		/* TRUE if -I flag used */
  int	morph_prefixes;		/* TRUE if -P flag used */
#endif /*MORPH_INFIX*/
#ifdef POOR_MORPH
  int	only_categories;	/* TRUE if -A flag used */
#endif /*POOR_MORPH*/
  int	ignore_filler;		/* TRUE if -F flag used */
public:
#ifdef MORPH_INFIX
#ifdef POOR_MORPH
  morph_fsa(const int ignorefiller, const int no_baseforms,
	    const int file_has_infixes,
	    const int file_has_prefixes, word_list *dict_names,
	    const char *language_file);
#else
  morph_fsa(const int ignorefiller, const int file_has_infixes,
	    const int file_has_prefixes,
	    word_list *dict_names, const char *language_file);
#endif /*POOR_MORPH*/
#else
#ifdef POOR_MORPH
  morph_fsa(const int ignorefiller,
	    const int file_has_infixes, word_list *dict_names,
	    const char *language_file);
#else
  morph_fsa(const int ignorefiller,
	    word_list *dict_names, const char *language_file);
#endif /*POOR_MORPH*/
#endif /*MORPH_INFIX*/
  virtual ~morph_fsa(void) {}
  int morph_file(tr_io &io_obj);
  int morph_word(const char *word);
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
  int sparse_morph_next_char(const char *word, const int level,
			     const long start);
#else
  int morph_next_char(const char *word, const int level, fsa_arc_ptr start);
#endif
#ifdef MORPH_INFIX
  int morph_infix(const int level, fsa_arc_ptr start);
  int morph_prefix(const int delete_position, const int level,
		   fsa_arc_ptr start);
  int morph_stem(const int delete_length, const int delete_position,
		 const int level, fsa_arc_ptr start);
#else
  int morph_stem(const int level, fsa_arc_ptr start);
#endif
  int morph_rest(const int level, fsa_arc_ptr start);
};/*class morph_fsa*/

/***	EOF morph.fsa	***/
