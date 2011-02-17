/***	spell.h		***/

/*	Copyright (C) Jan Daciuk, 1996-2004	*/


const int	Max_edit_distance = 3;	/* max edit distance allowed */

/* Found candidates for replacements with additional information */
struct ranked_hits {
  char		*list_item;	/* candidate */
  int		dist;		/* edit distance between candidate and word */
  int		cost;		/* cost of restoring this word */
};

/* Name:	comp
 * Class:	None.
 * Purpose:	Compares two items.
 * Parameters:	it1		- (i/o) first item;
 *		it2		- (i) second item.
 * Returns:	< 0 if it1 < it2;
 *		= 0 if it1 = it2;
 *		> 0 if it1 > it2.
 * Remarks:	It is necessary to modify the distance field during
 *		the comparison, because otherwise the new item should replace
 *		the old one, and the would be too expensive.
 */
inline int
comp(const ranked_hits *it1, const ranked_hits *it2)
{
  int c;
  ranked_hits *rh1 = (ranked_hits *)it1;
  if ((c = strcmp(it1->list_item, it2->list_item)) == 0)
    if (it1->dist < it2->dist)
      rh1->dist = it2->dist;
  return c;
}/*comp*/

/* Name:	new_copy
 * Class:	None.
 * Purpose:	Creates a copy of the parameter in dynamic memory.
 * Parameters:	it		- (i) item to be copied.
 * Returns:	Copy of the item.
 * Remarks:	None.
 */
inline ranked_hits *
new_copy(const ranked_hits *it)
{
  ranked_hits	*rh;

  rh = new ranked_hits;
  memcpy(rh, it, sizeof(ranked_hits));
  return rh;
}/*new_copy*/


typedef		list<ranked_hits>	hit_list;


/* Class name:	H_matrix
 * Purpose:	Keeps track of already computed values of edit distance.
 * Remarks:	To save space, the matrix is kept in a vector,
 */
class H_matrix {
private:
  int	*p;			/* the vector */
  int	row_length;		/* row length of matrix */
  int	column_height;		/* column height of matrix */
  int	edit_distance;		/* edit distance */
public:
  H_matrix(const int distance, const int max_length);	/* initialize */
  int operator()(const int i, const int j);		/* get value */
  void set(const int i, const int j, const int val);	/* set value */
};/*H_matrix*/


class spell_fsa : public fsa {
protected:
  H_matrix		H;		/* previously computed distances */
  int			edit_dist;	/* edit distance */
  int			e_d;		/* effective edit distance */
  hit_list		results;	/* list of replacements with their
					   edit distances */
#ifdef CHCLASS
  char			**first_column;	/* for each index equal to the code
					   of a single character that
					   may appear in text, and may be
					   replaced with a two-character
					   sequence, a pointer to a string
					   of such sequences (pairs of
					   characters) */
  char			**second_column;/* for each index equal to the code
					   of a single character that
					   may replace a two-letter sequence
					   of characters in a text, a pointer
					   to a string of such sequences
					   (pairs of characters) */
#endif


  int rank_replacements(void);
#ifdef CHCLASS
  int ed(const int i, const int j, const int word_index, const int cand_index);
  int cuted(const int depth, const int word_index, const int cand_index);
#else
  int ed(const int i, const int j);
  int cuted(const int depth);
#endif
public:
  spell_fsa(word_list *dict_names, const int distance,
	    const char *chclass_file,
	    const char *language_file = NULL);
  virtual ~spell_fsa(void) {}
  int spell_word(const char * word, const bool force);
#ifdef RUNON_WORDS
  hit_list *find_runon(const char *word);
#endif
#ifdef CHCLASS
  int read_character_class_tables(const char *file_name);
  int match_word(const int i, const int j);
  int match_candidate(const int i, const int j);
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
  hit_list *sparse_find_repl(const int depth, const long start,
			     const int word_index, const int cand_index);
#endif
  hit_list *find_repl(const int depth, fsa_arc_ptr start, const int word_index,
		      const int cand_index);
#else //!CHCLASS
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
  hit_list *find_repl(const int depth, const long start);
#endif
  hit_list *find_repl(const int depth, fsa_arc_ptr start);
#endif
  void find_repl_all_dicts(void);
  int spell_file(const int distance, const bool force, tr_io &io_obj);
};/*class spell_fsa*/


/* Name:	comp_cost
 * Class:	None.
 * Purpose:	Compares two ranked_hits structures on cost.
 * Parameters:	rh1		- (i) first structure;
 *		rh2		- (i) second structure.
 * Returns:	< 0 if rh1 < rh2
 *		= 0 if rh1 = rh2
 *		> 0 if rh1 > rh2.
 * Remarks:	Parameters must have the void pointer type for compatibility.
 */
int
comp_cost(const void *rh1, const void *rh2);

/***	EOF spell.h	***/
