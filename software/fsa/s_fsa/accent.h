/***	accent.h		***/

/*	Copyright (c) Jan Daciuk, 1996-2010	*/

#ifdef UTF8
/* accent_tab_type is a type for an item of a sparse matrix (vector)
   holding one arc of a tree containing information about characters
   with and without diacritics.
   The tree is formed with strings of bytes (type char) that constitute
   wide UTF8 characters. As the strings can have some common prefixes,
   a tree structure emerges. The root has the base 0. The arcs (edges,
   branches) represent first bytes of UTF8 characters. They lead to nodes
   that are roots of subtrees representing all UTF8 characters starting
   with the same given bytes.
   Each node is represented as a collection of arcs starting at a certain
   base in a vector. An arc is present if an item with an index equal to
   the sum of the base and the label of the arc contains the label
   as the value of the `chr' field. Different nodes can share space
   in the vector, but they cannot have the same bases. The address of a node
   is its base. An arc leads from one node to another, and the address
   (the base) of the target node is stored in the field named `follow'.
   Characters without diacritics form a tree. Leaves of that tree
   are at the ends of paths representing each such character, so they
   themselves represent each such character. They have pointers (`repl' field)
   to roots of other trees, that represent all UTF8 characters that have
   the same shape, but that also have diacritics.
*/
struct accent_tab_type {
  int		follow;		/* target node of the arc */
  int		repl;		/* root of a tree for characters with diacr. */
  unsigned char	chr;		/* arc label - subsequent UTF8 byte */
};

typedef accent_tab_type		accent_tabs;

#else
struct accent_tabs {
  /* For every character: either itself or a character without diacritics */
  char *accents;
  /* For characters that may have diacritics: all equivalent characters */
  char **eqchs;
};
#endif

class accent_fsa : public fsa {
protected:
#ifdef UTF8
  accent_tab_type	*char_eq;
#else
  const char		*char_eq;
#endif
  char			**same_class;
public:
  accent_fsa(word_list *dict_names, const char *language_file);
  virtual ~accent_fsa(void) {}
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
  int sparse_word_accents(const char *word, const int level,
			  const long start);
#endif
  int word_accents(const char *word, const int level,
			  fsa_arc_ptr start);
#if defined(UTF8) && !(defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE))
  int word_accents_dia(const char *word, const int level, fsa_arc_ptr start,
		       const unsigned int dia);
#endif
  int accent_word(const char *word, const accent_tabs *equiv);
  int accent_file(tr_io &io_obj, const accent_tabs *equiv);
};/*class accent_fsa*/

/***	EOF accent.fsa	***/
