/***	visualize.h	***/

/*	Copyright (c) Jan Daciuk, 1999-2004	*/

/* Class name:	visual_fsa
 * Purpose:	Provides an environment for programs creating graphs
 *		from dictionaries.
 * Remarks:
 */
class visual_fsa : public fsa {
protected:
  int current_offset;		/* what arcs already processed */
  int compressed;		/* whether dictionary produced with -O */
  int no_of_arcs;		/* number of arcs in the automaton */
  unsigned char	*visited;	/* what arcs visited so far (only with
				   compressed dictionaries (-O)) */
  int arc_size;			/* 1 for not flexible, arc size for flexible */

  int words_in_node(fsa_arc_ptr start);
  int create_node(fsa_arc_ptr start);
  int create_edges(fsa_arc_ptr start);
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
  void sparse_create_nodes(void);
  void sparse_create_edges(void);
#endif
public:
  visual_fsa(word_list *dict_names, const int compressed_dictionary);
  ~visual_fsa(void) {};
  int create_graphs(void);
};/*class visual_fsa*/

/***	EOF visualize.h	***/
