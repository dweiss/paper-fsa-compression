/***	build_fsa.h	***/


void
not_enough_memory(void);

extern const	int	WORD_BUFFER_LENGTH;

extern char		FILLER;			/* for compatibility */
extern char		ANNOT_SEPARATOR;	/* annotation separator */

using namespace std;

#ifdef A_TERGO
node *reduce_inner(node *n);
node *rebuild_index(node *n);
node *mark_inner(node *n, const int no);
int delete_index_node(node *n, const int hit_children=TRUE);
#ifdef PRUNE_ARCS
int is_annotated(const node *n);
node *prune_arcs(node *n);
#endif
#endif
#if defined(WEIGHTED) && defined(A_TERGO)
int weight_arcs(const node *n);
#endif
#if defined(A_TERGO) && defined(GENERALIZE)
int new_gen(node *n, const int pref);
void remove_annot_pointers(node *n);
int collect_annotations(node *n, const int prefix);
#endif




/* Class name:	automaton
 * Purpose:	Provide methods and common variables for building an automaton.
 * Methods:	automaton	- initiates roots of automaton and index;
 *		get_root	- returns root node of the automaton;
 *		build_fsa	- build the automaton;
 *		write_fsa	- writes the automaton to a file.
 * Variables:	root		- root of the automaton.
 */
class automaton {
private:
  node		*root;
public:
  char		FILLER;		/* character to be ignored (for fsa_guess) */
  automaton(void) { root = new node();
  }
  node *get_root() const { return root; }
  node *set_root(node *new_root) { return (root = new_root); }
  int build_fsa(istream &infile);
  int write_fsa(ostream &out_file, const int make_numbers = FALSE);
};/* automaton */



/***	EOF build_fsa.h	***/
