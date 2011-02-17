/***	prefix.h	***/

/*	Copyright (C) Jan Daciuk, 1996-2004	*/

class prefix_fsa : public fsa {
public:
  prefix_fsa(word_list *dict_names, const char *language_file = NULL);
  virtual ~prefix_fsa(void) {}
  int complete_file_words(tr_io &io_obj);
  int complete_prefix(const char *word_prefix, tr_io &io_obj);
#if defined(FLEXIBLE) && defined(STOPBIT) &&defined(SPARSE)
  int sparse_compl_prefix(const char *word_prefix, tr_io &io_obj,
			  const int depth, const long start);
  int sparse_compl_rest(tr_io &io_obj, const int depth, const long start);
#endif
  int compl_prefix(const char *word_prefix, tr_io &io_obj, const int depth,
                  fsa_arc_ptr start);
  int compl_rest(tr_io &io_obj, const int depth, fsa_arc_ptr start);
};/*class prefix_fsa*/


/***	EOF prefix.h	***/
