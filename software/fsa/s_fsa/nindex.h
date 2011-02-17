/***	nindex.h	***/

/*	Copyright (C) Jan Daciuk, 1996-2004	*/

#ifndef		NINDEX_H
#define		NINDEX_H

enum { PRIM_INDEX, SECOND_INDEX };

/* If a state contains more than this transitions, they will not be
   shuffled to try to put smaller states into it */
#define MAX_ARCS_TO_SHUFFLE	32

/* Max size of a subset in match_subset */
#define MAX_SUBSET_SHUFFLE	10

struct tree_index {
  union down_pointer {
    node	**leaf;		// to the table where each entry contains
                                // a substring of the given length,
                                // given number of children, and
				// begins with the given letter.
                                // This table contains only pointers
				// to nodes,
				// and is sorted alphabetically
    tree_index	*indx;		// points to a table of lower level
  } down;
  int		counter;	// number of entries in *down
    int		allocated;	// number of places allocated in leaf
};//tree_index




/* Name:	get_index_by_name
 * Class:	None
 * Purpose:	Delivers appropriate index.
 * Parameters:	index_name	- (i) index number.
 * Returns:	Root of required index.
 * Remarks:	Currently two indices available: PRIM_INDEX and SECOND_INDEX.
 *		No parameter checking.
 */
tree_index *
get_index_by_name(const int index_name);


/* Name:	register_at_level
 * Class:	None.
 * Purpose:	Looks for an appropriate entry at the given level.
 *		If not found, then creates it.
 * Parameters:	discriminator	- (i) value that should be matched
 *					against `data';
 *		table_node	- (i/o) a node in the table of the upper
 *					level that points to the table
 *					that is to be searched;
 *		to_register	- (i) TRUE means register, otherwise
 *					find and DO NOT REGISTER;
 *		cluster_size	- (i) how many entries are in the table
 *				  at this level.
 * Returns:	A pointer to a table entry (existing previously or
 *		just created) pointing to the next (the lower) level.
 * Remarks:	It was necessary to add the cluster_size parameter in order
 *		to have the first level index on first arc label
 *		in secondary index.
 */
tree_index *
register_at_level(const int discriminator, tree_index &table_node,
		  const int to_register,
		  const int cluster_size = MAX_ARCS_PER_NODE);


/* Name:	cmp_nodes
 * Class:	None (friend of class node).
 * Purpose:	Find if two subgraphs (represented by their root nodes)
 *		are isomorphic.
 * Parameters:	node1		- (i) first node;
 *		node2		- (i) second node;
 * Returns:	0		- if nodes are isomorphic;
 *		negative value	- if `node2' should precede `node1';
 *		positive value	- if `node2' should follow `node1'.
 * Description:	Only the children are compared for letters, being final,
 *		and addresses of the nodes they lead to.
 *		If they are equal, addresses of children are compared.
 * Remarks:	The number of children for the two nodes is not checked,
 *		as it assumed that the function is invoked only when
 *		the tree search has selected nodes of equal number of children
 *		for the comparison.
 *
 *		It is assumed that children are sorted alphabetically.
 *		This should be true if data for the automaton is sorted
 *		alphabetically.
 *
 *		It is possible to compare addresses of children, because
 *		children are registered first, and all child nodes
 *		of the node to be registered (if any) are already registered.
 *
 *		One cannot say, which address is `bigger' than another,
 *		so the function returns `node1 < node2', because this
 *		triggers comparing next nodes at register_at_level.
 */
int
cmp_nodes(const node *node1, const node *node2);

    
/* Name:	part_cmp_nodes
 * Class:	None (friend of class node).
 * Purpose:	Looks if it is possible to replace arcs of the smaller node
 *		with a subset of arcs of a larger node. If so, and replace is
 *		TRUE, it is done.
 * Parameters:	small_node	- (i/o) node to be replaced;
 *		big_node	- (i) node that may contain the same arcs;
 *		offset		- (i) if present, only arcs from #offset from
 *					small_node is used in comparison;
 *		group_size	- (i) (optional) how many arcs to compare.
 * Returns:	Number of the arc of big_node that is the first of
 *		small_node->no_of_children arcs that are identical to the arcs
 *		of small_node, or -1 if such arcs not found.
 * Remarks:	Let the small node have arcs (c d f).
 *		If the big node is (b c d f g) we compare:
 *		(b c d f g)  (b c d f g)  (a b c f g)  (a b c f g)
 *		(c d f)        (c d f)<-found!
 *
 *		But for the big node being (b c d e f g):
 *		(b c d e f g)  (b c d e f g)  (b c d e f g)  (b c d e f g)
 *		(c d f)          (c d f)          (c d f)          (c d f)
 *		it does not work.
 *
 *		Maybe by chaging the order of arcs more arcs could be slashed.
 */
int
part_cmp_nodes(const node *small_node, const node *big_node,
	       const int offset=-1, const int group_size=-1);

/* Name:	a_cmp_nodes
 * Class:	None.
 * Purpose:	Compares two nodes.
 * Parameters:	node1			- (i) first node;
 *		node2			- (i) second node.
 * Returns:	< 0 if node1 < node2,
 *		= 0 if node1 = node2,
 *		> 0 if node1 > node2.
 * Remarks:	It is a wrap-around for cmp_nodes, necessary for
 *		bsearch_reg. Two spurious parameters are for compatibility
 *		with part_cmp_nodes.
 */
inline int
a_cmp_nodes(const node *node1, const node *node2, const int, const int)
{
  return cmp_nodes(node1, node2);
}/*p_cmp_nodes*/


/* Name:	bsearch_reg
 * Class:	None
 * Purpose:	Find either a node (or a subnode), or a place where it should
 *		be inserted in a register.
 * Parameters:	n		- (i) node to be found;
 *		low		- (i) address of the first pointer to node
 *					in register page;
 *		high		- (i) address of the last pointer to node
 *					in register page;
 *		cmp		- (i) function that compares two nodes;
 *		offset		- (i) for pseudonodes, the number of the
 *					first arc of the pseudonode;
 *		group_size	- (i) for pseudonodes, number of arcs in
 *					thye pseudonode.
 * Returns:	Address of a pointer to node n, or a place where that pointer
 *		should be placed.
 * Remarks:	None.
 */
node **
bsearch_reg(const node *n, node **low, node **high,
	    int (*cmp)(const node *, const node *, int, int),
	    int offset, int group_size);



/* Name:	find_or_register
 * Class:	None.
 * Purpose:	Finds an isomorphic subgraph in the automaton or registers
 *		a new node.
 * Parameters:	n		- (i) node to be found or registered;
 *		index_root	- (i/o) which index to use;
 *		register_it	- (i) if TRUE, register the node,
 *					if FALSE, find an isomorphic node.
 * Returns:	Pointer to an isomorphic node or NULL.
 * Remarks:	The index has levels indexed for:
 *		number of children (primary index) / first letter (secondary)
 *		hash function
 *		all features of the node.
 *
 *		It is assumed, that this function is not used for registering
 *		nodes in the secondary index. In theory it could, but the
 *		secondary index registers "subnodes" or "pseudonodes", not
 *		regular nodes, and no provision is taken here to take that
 *		fact into consideration. However, what is looked up in the
 *		secondary nodes are regular nodes, so the "this" pointer
 *		is always a regular node.
 *
 *		255 is the highest possible code of a character (so that
 *		it can be used in the first level index).
 */
node *
find_or_register(node *n, const int index_root_name,
		 const int register_it);


/* Name:	unregister_node
 * Class:	None.
 * Purpose:	Removes a node from the primary register.
 * Parameters:	n		- (i) the node to be unregistered.
 * Returns:	TRUE if the node was unregistered successfully,
 *		FALSE otherwise.
 * Remarks:	Needed for index a tergo when generalizing.
 */
int
unregister_node(const node *n);

/* Name:	register_subnodes
 * Class:	None.
 * Purpose:	Registers sets of `group_size' arcs as a pseudonode.
 * Parameters:	c_node		- (i) groups of arcs of c_node are to be
 *					registered as a pseudonode;
 *		group_size	- (i) number of arcs in a group.
#ifdef STOPBIT
 * Returns:	If isomorphic pseudonode found - that node, NULL otherwise.
#else
 * Returns:	Nothing.
#endif
 * Remarks:	A pointer to the original full size node is registered in
 *		the secondary index as a pseudonode. This is done several
 *		times, i.e. c_node->no_of_children - group_size + 1.
 *		A pseudonode consists of group_size consecutive child
 *		arcs.
 *
 *		When using STOPBIT option, only one group of arcs is registered
 *		as a pseudonode - groupsize arcs at the end of the node.
 *
 *		The first level in the secondary index is now first arc letter.
 *		This is done in order to distinguish different threads
 *		at the lowest level, and to have the posssibility of comparing
 *		pseudonodes at the lowest level.
 *
 *		From version 0.19 on, the function now returns the pseudonode
 *		found, if compiled with STOPBIT.
 */
#ifdef STOPBIT
node *
#else
void
#endif
register_subnodes(node *c_node, const int group_size);


/* Name:	destroy_index
 * Class:	None
 * Purpose:	Destroys index structure (releasing memory!).
 * Parameters:	index_root	- (i) index to destroy.
 * Returns:	Nothing.
 * Remarks:	Nodes pointed to in the index are not destroyed.
 */
void
destroy_index(tree_index &index_root);


/* Name:	share_arcs
 * Class:	None.
 * Purpose:	Tries to find if arcs of one node are a subset of another.
 *		If so, a reference to them is replaced by a reference
 *		to a part (a subset) of the other node.
 * Parameters:	None.
 * Returns:	Number of suppressed arcs.
 * Remarks:	This must take A LOT of time!
 */
int
share_arcs(node *root);


#ifdef JOIN_PAIRS
const int	PAIR_REG_LEN = 32;

struct pair_reg {
  int		counter;
  int		allocated;
  node		**nodes;
};/*pair_reg*/

struct packed_tab {
  pair_reg	*p;
  char		c1;
  char		c2;
};/*packed_tab*/

class pair_registery {		/* friend of node */
  pair_reg	**pntr[256];
  int		no_of_pairs;
  packed_tab	*packed;

public:
  pair_registery(void);		/*	allocate memory */
  ~pair_registery(void);	/*	deallocate memory */
  void reg(node *n);		/*	register a node (with a pair of arcs)*/
  void prepare(void);		/*	convert table of nodes to vector */
  void join_arcs(void);		/*	merges arcs */
  void merge_pairs(void);	/*	launch the process */
};/*pair_registery*/
#endif


//#ifdef DEBUG
/* Name:	show_index
 * Class:	None (friend of class node).
 * Purpose:	Show index contents.
 * Parameters:	index_name	- (i) index number.
 * Returns:	Nothing.
 * Remarks:	I hate optimization.
 */
void
show_index(const int index_name);
//#endif


/* Name:	print_node
 * Class:	None.
 * Purpose:	Prints node details for debugging purposes.
 * Parameters:	n		- (i) the node to be examined.
 * Returns:	Nothing.
 * Remarks:	Printing is done on cerr.
 */
void
print_node(const node *n);

#endif
/***	EOF nindex.h	***/
