/***	nnode.h		***/

/*	Copyright (C) Jan Daciuk, 1996-2004	*/

#ifndef		NNODE_H
#define		NNODE_H

#include	<vector>


enum {FIND, REGISTER};
enum {FALSE, TRUE};

using namespace std;

class pair_registery;
class node;

const int	MAX_SPARSE_WAIT = 3; /* How many alphabet sizes should we wait
					before we abandon a whole in sparse
				        matrix */

/* Arc_node contains information describing an arc during building phase.
 * It is then enclosed in fsa_arc structure.
 */
struct arc_node {
  node		*child;		/* pointer to a child node (NULL - common
				   final node) */
#ifdef WEIGHTED
  int		weight;		/* number of different sets of annotations
				   recognized by the automaton beginning
				   in this node */
#endif
  char		is_final;	/* whether labels up to that contained
				   in this arc form a word */
  char		letter;		/* label of this arc */
};/*arc_node*/

#ifdef MORE_COMPR
  inline int operator !=(arc_node &a1, arc_node &a2)
     { return (a1.child != a2.child || a1.letter != a2.letter ||
	       a1.is_final != a2.is_final); }
#endif

class node;

#if defined(A_TERGO) && defined(GENERALIZE)
typedef struct {
 public:
  node	*annot;		// first node of annotation
#ifdef WEIGHTED
  int		weight;		// weight of annotation
#endif //!WEIGHTED
} ann_inf;
#endif //A_TERGO,GENERALIZE


/* Class name:	node
 * Purpose:	Provide methods and data for handling automaton nodes.
 */
class node {
private:
  arc_node	*children;		/* arcs leading from node */
  int		arc_no;			/* number of the first arc of the
					   node in all nodes of the automaton
					   */
  int		hit_count;		/* number of arcs leading to node */
  node		*big_brother;		/* node containing all arcs of this
					   node (and some others), or - in
					   case of JOIN_PAIRS - a two-arc
					   node sharing one arc with this node
					   */
#if defined(SUBAUT)
  node		**back_arcs;		/* addresses of all nodes referring
					   to this node */
  int		back_filled;		/* number of back references filled */
#endif
#if defined(NUMBERS) || defined(SUBAUT)
  int		entries;		/* number of entries (words) in
					   subgraph begining at node */
				        /* if SUBAUT defined, then this field
					   is used for numbering the nodes
					   so that the node without any
					   children is the first one, then
					   come all the nodes that have
					   all arcs going to that node,
					   then all nodes that have all
					   arcs going to nodes already
					   numbered, and so on
					*/
#endif
  unsigned char	no_of_children;		/* number of those arcs */
#ifdef JOIN_PAIRS
  char		brother_offset;		/* number of the arc in big brother
					   that is the first arc in this node;
					   a negative number number in case of
					   JOIN_PAIRS indicates brother_offset
					   of the big_brother
					   */
#else
  unsigned char	brother_offset;		/* number of the arc in big brother
					   that is the first arc in this node
					   */
#endif
#ifdef NUMBERS
  static int	arc_count;		/* number of arcs in automaton */
  static int	node_count;		/* number of nodes in the automaton */
  static int	a_size;			/* arc size */
  static int	entryl;			/* bytes per entries count */
#endif
#ifdef STATISTICS
  static int	total_arcs;		/* number of arcs in the automaton
					   (includes arcs stored in other
					   arcs) */
  static int	total_nodes;		/* number of nodes in the automaton */
  static int	in_single_line;		/* number of nodes forming
					   single lines */
  static int	line_length;		/* number of nodes in a line */
  static node	*current_line;		/* next node in current line
					   of single nodes */
  static int	outgoing_arcs[10];	/* number of nodes having the
					   corresponding number of outgoing
					   arcs */
  static int	incoming_arcs[12];	/* number of nodes having the
					   corresponding number of incoming
					   arcs */
  static int	line_of[10];		/* number of nodes in a single line
					   of nodes of given length */
  static int	sink_count;		/* number of incoming transitions
					   for the sink state */
#endif

public:
#ifdef FLEXIBLE
#ifdef NEXTBIT
  static int	next_nodes;
#endif
#if defined(STOPBIT) && defined(TAILS)
  static int	tails;
#endif
#endif
#ifdef SPARSE
  static bool in_annotations;
#endif //SPARSE
#ifdef MORE_COMPR
  unsigned char	free_end; 	/* the order of arcs can change in
					   the big brother in free_end
					   arcs at the end of the node
					*/
#endif
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(TAILS)
  unsigned char free_beg;	/* arcs from free_beg are
				   in a different node */
#endif
#if defined(A_TERGO) && defined(GENERALIZE)
  int nannots;			/* number of annotations */
  ann_inf *annots;		/* annotations (possibly with weights) */
#endif //A_TERGO,GENERALIZE

#ifdef FLEXIBLE
#ifdef NEXTBIT
  /* in write_arcs: is the node the next node in the automaton */
  int wis_next_node(const node *n) const
     {return (n != NULL &&
	      (n->big_brother ?
	       (n->brother_offset == 0 && n->big_brother->hit_count != 0)
	       : n->hit_count != 0)); }
  /* in number_arcs: is the node n the next node in the automaton
     after the current node;
     if it does not have a big brother, and it has not been numbered yet
     then we can put it right after the current node;
     if it has a big brother, and the big brother is not numbered yet,
     then the brother offset must be 0 for n to be the next node,
     because otherwise the first arc of n will not be the first arc
     of its big brother, so it will not be the next arc after the last arc
     of the current node.
  */
  int nis_next_node(const node *n) const
    {return (n != NULL &&
	     (n->big_brother ?
	      (n->brother_offset == 0 && n->big_brother->arc_no == -1)
	      : n->arc_no == -1));}
#endif
#endif
  int hash(const int start, const int how_many) const;
  node(void) { children = NULL; no_of_children = 0; arc_no = -1; hit_count = 0;
	       big_brother = NULL; brother_offset = 0;
#ifdef MORE_COMPR
		  free_end = no_of_children;
#endif
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(TAILS)
		  free_beg = 0; 
#endif
#ifdef NUMBERS
	       entries = -1;
#endif
#ifdef SUBAUT
	       back_arcs = NULL;
	       back_filled = 0;
#endif
#if defined(A_TERGO) && defined(GENERALIZE)
	       annots = NULL;
	       nannots = 0;
#endif
  }
  node(node *n) { no_of_children = n->no_of_children; hit_count = 0;
		  arc_no = n->arc_no; big_brother = NULL; brother_offset = 0;
#ifdef NUMBERS
		  /* this is probably never called */
		  entries = n->entries;
#endif
#ifdef MORE_COMPR
		  free_end = n->free_end;
#endif
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(TAILS)
		  free_beg = n->free_beg; 
#endif
#ifdef SUBAUT
		  back_arcs = NULL;
		  back_filled = 0;
#endif
#if defined(A_TERGO) && defined(GENERALIZE)
	       annots = NULL;
	       nannots = 0;
#endif
		  if (no_of_children) {
		    children = new arc_node[no_of_children];
		    memcpy(children, n->children,
			   no_of_children * sizeof(arc_node));
		  }
		  else
		    children = NULL;
		}
  ~node(void) {
#if defined(A_TERGO) && defined(GENERALIZE)
    if (nannots) {
      if (annots != NULL) {
	delete [] annots;
      }
    }
#endif
    delete [] children;
  }
  arc_node *get_children(void) const { return children; }
  arc_node *set_children(arc_node *kids, const int no_of_kids);
  int get_no_of_kids(void) const { return no_of_children; }
  int get_arc_no(void) const { return arc_no; }
  void set_arc_no(const int n) { arc_no = n; }
  node *get_big_brother(void) const { return big_brother; } /*I hate C++*/
  int get_brother_offset(void) const { return brother_offset; } /* -||- */
  friend int cmp_nodes(const node *node1, const node *node2);
  friend int part_cmp_nodes(const node *small_node, const node *big_node,
			    const int offset, const int group_size);
  friend int get_pseudo_offset(const node *small_node, const node *big_node);
  friend
#ifdef STOPBIT
    node *
#else
    void
#endif
    register_subnodes(node *c_node, const int group_size);
  node *add_postfix(const char *postfix);
  node *add_child(char letter, node *next_node);
  int fertile(void) const;
  int hit_node(int h = 1) { return hit_count += h; }
  node *compress_or_register(void);
  /*  friend int find_common_prefix(node *start_node, const char *word,
      prefix *common_prefix, const int length);*/
  int
#if defined(FLEXIBLE) && ((defined(STOPBIT) && defined(TAILS)) || defined(NEXTBIT))
#ifdef WEIGHTED
  number_arcs(const int gtl, const int weighted);
#else //!WEIGHTED
  number_arcs(const int gtl);
#endif //!WEIGHTED
#else //!WEIGHTED
#ifdef WEIGHTED
  number_arcs(const int weighted);
#else //!WEIGHTED
  number_arcs(void);
#endif //!WEIGHTED
#endif
  int
#ifdef WEIGHTED
  write_arcs(ostream &outfile, const int weighted);
#else //!WEIGHTED
  write_arcs(ostream &outfile);
#endif //!WEIGHTED
  node *set_link(node *big_guy, const int offset);
#if defined(FLEXIBLE) && ((defined(STOPBIT) && defined(TAILS)) || defined(NEXTBIT))
  friend int check_tails(node *n, const int tail_size);
#endif
#ifdef	SORT_ON_FREQ
  void count_arcs(int *freq_table);
  void sort_arcs(void);
#endif
#ifdef	JOIN_PAIRS
  friend class pair_registery;
#endif
#ifdef DEBUG
  friend void show_index(const int index_name);
#endif
#ifdef NUMBERS
  int get_entries(void) const { return entries; }
  void set_entries(const int e) { entries = e; }
  int inc_arc_count(const int a) { return arc_count += a; }
  int set_arc_count(const int a) { return arc_count = a; }
  int get_arc_count(void) const { return arc_count; }
  int inc_node_count(void) { return node_count++; }
  int get_node_count(void) const { return node_count; }
  int set_a_size(const int a) { return a_size = a; }
  int get_a_size(void) const { return a_size; }
  int set_entryl(const int e) { return entryl = e; }
  int get_entryl(void) const { return entryl; }
#endif
#ifdef STATISTICS
  void print_statistics(const node *n) {
    cerr << "Statistics:" << endl
      << "States: " << n->total_nodes << endl
      << "Transitions: " << n->total_arcs - 1 << endl
      << "Number of nodes with given # of outgoing transitions" << endl
      << "1:\t\t" << n->outgoing_arcs[1] - 1 << endl
      << "2:\t\t" << n->outgoing_arcs[2] << endl
      << "3:\t\t" << n->outgoing_arcs[3] << endl
      << "4:\t\t" << n->outgoing_arcs[4] << endl
      << "5:\t\t" << n->outgoing_arcs[5] << endl
      << "6:\t\t" << n->outgoing_arcs[6] << endl
      << "7:\t\t" << n->outgoing_arcs[7] << endl
      << "8:\t\t" << n->outgoing_arcs[8] << endl
      << "9:\t\t" << n->outgoing_arcs[9] << endl
      << ">9:\t\t" << n->outgoing_arcs[0] << endl;
    cerr << "Number of nodes with given # of incoming transitions" << endl
      << "1:\t\t" << n->incoming_arcs[1] - 2 << endl
      << "2:\t\t" << n->incoming_arcs[2] << endl
      << "3:\t\t" << n->incoming_arcs[3] << endl
      << "4:\t\t" << n->incoming_arcs[4] << endl
      << "5:\t\t" << n->incoming_arcs[5] << endl
      << "6:\t\t" << n->incoming_arcs[6] << endl
      << "7:\t\t" << n->incoming_arcs[7] << endl
      << "8:\t\t" << n->incoming_arcs[8] << endl
      << "9:\t\t" << n->incoming_arcs[9] << endl
      << "10-99:\t\t" << n->incoming_arcs[10] << endl
      << "100-999:\t" << n->incoming_arcs[11] << endl
      << ">999:\t\t" << n->incoming_arcs[0] << endl
      << "sink state has " << sink_count << " incoming transitions" << endl;
    cerr << "Number of nodes in single lines: " << n->in_single_line - 1
	 << endl;
    cerr << "Number single lines of given length" << endl
      << "1:\t\t" << n->line_of[1] - 1 << endl
      << "2:\t\t" << n->line_of[2] << endl
      << "3:\t\t" << n->line_of[3] << endl
      << "4:\t\t" << n->line_of[4] << endl
      << "5:\t\t" << n->line_of[5] << endl
      << "6:\t\t" << n->line_of[6] << endl
      << "7:\t\t" << n->line_of[7] << endl
      << "8:\t\t" << n->line_of[8] << endl
      << "9:\t\t" << n->line_of[9] << endl
      << ">9:\t\t" << n->line_of[0] << endl;
  }
#endif
#ifdef MORE_COMPR
  friend int match_subset(node *n, const int kids_tab[], const int kids_no_no);
  friend int match_part(node *n, node *nn, const int to_do, const int start_at,
			const int subset_size, const int reg_tails=FIND);
#endif


public:
  static int	no_of_arcs;	/* number of arcs generated */
};/*node*/



/* Name:	delete_branch
 * Class:	None.
 * Purpose:	Deletes a subgraph beginning at the node.
 * Parameters:	n	- (i/o) starting node of the subgraph to delete.
 * Returns:	Nothing.
 * Remarks:	Only nodes with hit_count greater than zero are deleted.
 *		There is no need to deregister deleted nodes.
 *		Only unique nodes are registered.
 *		Only non-unique nodes are deleted.
 */
void
delete_branch(node *branch);



/* Name:	delete_old_branch
 * Class:	None.
 * Purpose:	Deletes a node and all nodes beneath it if appropriate
 *		nodes have hit count equal to one.
 * Parameters:	n		- (i/o) starting node.
 * Returns:	TRUE if the node deleted, FALSE otherwise.
 * Remarks:	This is similar to delete_branch, but delete_branch deleted
 *		nodes that were not yet registered, whereas delete_old_branch
 *		deletes nodes that are already in the register. The difference
 *		lies in the treatment of hit count.
 */
int
delete_old_branch(node *branch);


/* Name:	cmp_arcs
 * Class:	None.
 * Purpose:	Compare arcs on label for sorting.
 * Parameters:	a1		- (i) first arc;
 *		a2		- (i) second arc.
 * Returns:	< 0 if a1 < a2;
 *		= 0 if a1 = a2;
 *		> 0 if a1 > a2.
 * Remarks:	For sorting in prune_arcs() so that optimization works.
 */
int cmp_arcs(const void *a1, const void *a2);

#ifdef	SORT_ON_FREQ
/* Name:	freq_cmp
 * Class:	None.
 * Purpose:	Compares arcs on frequency.
 * Parameters:	el1		- (i) the first arc;
 *		el2		- (i) the second arc;
 * Result:	-1	- if the label on the first arc appears less frequently
 *				on arcs in the automaton than the label on the
 *				second arc;
 *		0	- if the label on the first arc appears the same number
 *				of times in the automaton as the label on the
 *				second arc;
 *		1	- if the label on the first arc appears more frequently
 *				on arcs in the autoamton than the label on the
 *				second arc.
 * Remarks:	None.
 */
int
freq_cmp(const void *el1, const void *el2);
#endif //SORT_ON_FREQ


#ifdef NUMBERS
/* Name:	number_entries
 * Class:	None
 * Purpose:	Assign the number of entries (words) contained in the subgraph
 *		beginning at this node to the field entries.
 * Parameters:	n		- beginning of subgraph.
 * Returns:	Number of entries in this subgraph.
 * Remarks:	Number of entries in a subgraph is the sum of the number of
 *		entries in each of its subgraphs. The number of entries in
 *		an empty graph is 1.
 *		Initially, entries is -1, so it denotes a node that has not
 *		been visited yet.
 *		Additionally, arcs and nodes are counted in static variables.
 */
int
number_entries(node *n);
#endif //NUMBERS



/* Name:	mark_inner
 * Class:	None.
 * Purpose:	Mark nodes with a specified value of arc_no.
 * Parameters:	n	- (i) node to be marked;
 *		no	- (i) arc_no for that node.
 * Returns:	n - the node to be marked.
 * Remarks:	It is assumed that node with arc_no = no are already marked.
 */
node *
mark_inner(node *n, const int no);


#ifdef SPARSE
class SparseTrans {
 private:
  unsigned long	target;		/* index of the target state */
  char		label;		/* transition label */
  bool		final;		/* true if transition is final */
#ifdef NUMBERS
  unsigned long	hash_v;		/* the smallest number in this branch */
#endif

 public:
  SparseTrans(void) : target(0L), label(0), final(false)
#ifdef NUMBERS
    , hash_v(0L)
#endif
    {}

  SparseTrans(const long int t, const char l, const bool f)
    : target(t), label(l), final(f) {}

  void set_trans(const long int t, const char l, const bool f) {
    target = t; label = l; final = f;
  }

  void set_target(const unsigned long t) { target = t; }
  void set_label(const char l) { label = l; }
  void set_finality(const bool f) { final = f; }
  unsigned long get_target(void) const { return target; }
  char get_label(void) const { return label; }
  bool get_finality(void) const { return final; }
#ifdef NUMBERS
  void set_hash_v(const unsigned long v) { hash_v = v; }
  unsigned long get_hash_v(void) const { return hash_v; }
#endif
  /* write one transition */
  int write(ostream &outfile,	/* where to write */
#ifdef NUMBERS
	    const int entryl,	/* length of numbering info */
#endif
	    const int gtl) {
    /* write the label */
    if (!(outfile.write((char *)&label, 1)))
      return FALSE;
    char buf[16];
#ifdef NUMBERS
    if (entryl > 0) {
      /* write the numbering info */
      unsigned long hv = hash_v;
      for (int j = 0; j < entryl; j++) {
	buf[j] = hv & 0xff;
	hv >>= 8;
      }
      if (!(outfile.write(buf, entryl)))
	return FALSE;
    }
#endif
    /* write the pointer */
    unsigned long tp = target * 2 + (final ? 1 : 0);
    for (int k = 0; k < gtl; k++) {
      buf[k] = tp & 0xff;
      tp >>= 8;
    }
    if (!(outfile.write(buf, gtl)))
      return FALSE;
    return TRUE;
  }
};//SparseTrans

class SparseVect {
private:
  vector<char> states;		// a bit is set when the state is occupied
  vector<SparseTrans> transitions;	// 
  unsigned int first_free_state;// first free state position
//  unsigned char min_char;	// the smallest character code
//  unsigned char max_char;	// the largest character code
  int		alphabet_size;	// number of letters used
  char		alphabet[256];// translates numbers to characters
  char		char_num[256];	// translates characters to numbers

public:

  /* constructor */
  //SparseVect(const unsigned char mnc, const unsigned char mxc)
  //  : first_free_state(1), min_char(mnc), max_char(mxc) {}

  /* constructor */
  //SparseVect(void) : first_free_state(1) {}
  //void set_char_range(const unsigned char mnc, const unsigned char mxc) {
  //  min_char = mnc; max_char = mxc;
  //}

  /* constructor */
  SparseVect(void) : first_free_state(1), alphabet_size(1) {
    alphabet[0] = 0;
    for (int i = 0; i < 256; i++) {
      char_num[i] = 0;
    }
    transitions.push_back(SparseTrans()); /* for character 0 */
    states.push_back(1);		/* reserve #0 for an empty state */
  }

//  /* set the minimal and maximal label */
//  void set_label_bounds(const unsigned char labmin,
//			const unsigned char labmax) {
//    min_char = labmin; max_char = labmax;
//    /* initialize first state */
//    for (int i = labmin; i <= labmax; i++) {
//      transitions.push_back(SparseTrans());
//    }
//    states.push_back(1);	// reserve #0 for empty state
//    first_free_state = 1;
//  }

  /* add a character to the alphabet if necessary - return its number */
  unsigned char add_label(const char c) {
    if (char_num[(unsigned char)c] == 0) {
      alphabet[alphabet_size] = c;
      char_num[(unsigned char)c] = alphabet_size++;
      transitions.push_back(SparseTrans()); /* for state number 0 */
      return alphabet_size - 1;
    }
    else {
      return char_num[(unsigned char)c];
    }
  }

  /* get all chars that appear as labels of transitions (also in annots) */
  const char *get_alphabet(void) const { return char_num; }

  /* get the ordinal number of the character in the alphabet */
  unsigned char get_character_number(const char c) const {
    return char_num[(const unsigned char)c];
  }

  /* reset the vector */
  void reset(void) {
    transitions.clear(); states.clear();
    /* initialize first state */
//    for (int i = min_char; i <= max_char; i++) {
//      transitions.push_back(SparseTrans());
//    }
    for (int i = 0; i < alphabet_size; i++) {
      transitions.push_back(SparseTrans());
    }
    states.push_back(1);	// reserve #0 for empty state
    first_free_state = 1;
  }

  /* number of transitions */
  int size(void) const { return transitions.size(); }

  /* number of states rounded up to the multiple of 8 */
  int no_of_states(void) const { return states.size() * 8; }

  /* return first available free node */
  unsigned int first_free_pos(void) { return first_free_state; }
  
  // find first free state position
  unsigned int fits_in(node *node_to_fit) {
    int n = node_to_fit->get_no_of_kids();
#ifndef SLOW_SPARSE
    if ((states.size() << 3) > first_free_state +
	MAX_SPARSE_WAIT * alphabet_size) {
      // There is a waisted state position, but little chance of fixing it
      first_free_state++;
      while (((first_free_state >> 3) + 1 <= states.size()) && 
	     (states[(first_free_state >> 3)] &
	      (1 << (first_free_state & 7))) != 0) {
	first_free_state++;
      }
    }
#endif
    for (unsigned int j = first_free_state;;j++) {
      arc_node *tt = node_to_fit->get_children();
      if ((j >> 3) + 1 > states.size()) {
	states.push_back(0);	// enlarge the state vector
      }
      //      if (j + (max_char - min_char) >= transitions.size()) {
      if (j + alphabet_size >= transitions.size()) {
	transitions.push_back(SparseTrans()); // enlarge the transition vector
      }
      if ((states[(j >> 3)] & (1 << (j & 7))) == 0) {
	// there is a free state position here
	bool found = true;
	for (int i = 0; i < n; i++, tt++) {
	  // unsigned char uc = (unsigned char)(tt->letter - min_char);
	  unsigned char uc = char_num[(unsigned char)(tt->letter)] - 1;
	  if (transitions[j + uc].get_label() != 0) {
	    found = false;
	    break;
	  }
	}
	if (found) {
	  tt = node_to_fit->get_children(); /* reset */
	  for (int k = 0; k < n; k++, tt++) {
	    // set the bits for transitions
	    //unsigned char uc1 = (unsigned char)(tt->letter - min_char);
	    unsigned char uc1 = char_num[(unsigned char)(tt->letter)] - 1;
	    transitions[j + uc1].set_label(tt->letter);
	    transitions[j + uc1].set_finality(tt->is_final);
	  }
	  // Set the bit for the state
	  states[j >> 3] |= (1 << (j & 7));
	  // Update first_free_state
	  while (((first_free_state >> 3) + 1 <= states.size()) && 
		 (states[(first_free_state >> 3)] &
		  (1 << (first_free_state & 7))) != 0) {
	    first_free_state++;
	  }
	  return j;
	}//if found
      }//if state free
    }//for j
    // Since we can have 0's at the end of vectors forever, we will always find
    // a free space there, and "return j" above will terminate the function.
    // So we will NEVER get here!
    return 0;			// to calm the compiler
  }//fits_in

  /* write the vector */
  int write(ostream &outfile,	/* where to write */
#ifdef NUMBERS
	    const int entryl,	/* length of numbering info */
#endif
	    const int gtl) {
    /* Write the alphabet size */
    char as = alphabet_size;
    if (!(outfile.write(&as, 1))) {
      return FALSE;
    }

    /* Write the alphabet */
    if (!(outfile.write(alphabet, alphabet_size))) {
      return FALSE;
    }

    /* Write character numbers */
    if (!(outfile.write(char_num, 256))) {
      return FALSE;
    }

    int size = transitions.size();
#ifdef STATISTICS
    int waisted = 0;
#endif
    for (int i = 0; i < size; i++) {
#ifdef STATISTICS
      if (transitions[i].get_label() == '\0') {
	waisted++;
      }
#endif
#ifdef NUMBERS
      if (!(transitions[i].write(outfile, entryl, gtl)))
	return FALSE;
#else
      if (!(transitions[i].write(outfile, gtl)))
	return FALSE;
#endif
    }//for i
#ifdef STATISTICS
    cerr << "Waisted " << waisted << " / " << size << " sparse transitions"
	 << endl << "First free state is " << first_free_state << endl;
#endif
    return size;
  }//write

  SparseTrans & operator[](const int i) { return transitions[i]; }

  SparseTrans & operator()(const int i, const unsigned char c) {
    return transitions[i + char_num[c] - 1];
  }
};//SparseVect

//extern unsigned char min_label, max_label;
extern SparseVect sparse_vector;
extern char *annot_buffer;		// buffer for annotations
extern long int ab_ptr;		// index in that buffer

/* Name:	add_labels_to_alphabet
 * Class:	None.
 * Purpose:	Adds labels on all transitions of the node to the alphabet.
 * Parameters:	n		- (i) the node to examine.
 * Returns:	Nothing.
 * Remarks:	None.
 */
void inline
add_labels_to_alphabet(const node *n)
{
  arc_node *tt = n->get_children();
  int k = n->get_no_of_kids();
  for (int i = 0; i < k; i++,tt++) {
    sparse_vector.add_label(tt->letter);
  }
}//add_labels_to_alphabet
#endif //SPARSE

#endif
/***	EOF nnode.h	***/
