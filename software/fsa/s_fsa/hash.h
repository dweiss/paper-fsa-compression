/***	hash.h	***/

/*	Copyright (C) Jan Daciuk, 1996-2004	*/

enum direction_t {unspecified, words_to_numbers, numbers_to_words};

class hash_fsa : public fsa {
public:
  hash_fsa(word_list *dict_names, const char *language_file = NULL);
  virtual ~hash_fsa(void) {}
  int hash_file(tr_io &io_obj, const direction_t direction);
#if defined(FLEXIBLE) && defined(STOPBIT) & defined(SPARSE)
  int sparse_find_number(const char *word, const long start, int word_no);
#endif
  int find_number(const char *word, fsa_arc_ptr start, int word_no);
  int words_in_node(fsa_arc_ptr start);
#if defined(FLEXIBLE) && defined(STOPBIT) & defined(SPARSE)
  const char *sparse_find_word(const int word_no, int n, const long start,
			       const int level);
#endif
  const char *find_word(const int word_no, int n, fsa_arc_ptr start,
			const int level);
};/*class hash_fsa*/


/***	EOF prefix.h	***/
