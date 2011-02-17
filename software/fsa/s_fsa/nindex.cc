/***	nindex.cc	***/

/* Copyright (C) Jan Daciuk, 1996-2004. */

#include	<iostream>
#include	<stddef.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	"fsa.h"
#include	"nnode.h"
#include	"nindex.h"

static tree_index	primary_index = { {NULL}, 0 };
static tree_index	secondary_index = { {NULL}, 0 };
const int	INDEX_SIZE_STEP = 16;	// allocate chunks of 16 pointers

using namespace std;

/* internal prototypes */
int match_tails(node *n, const int tail_size);


/* Name:	get_index_by_name
 * Class:	None
 * Purpose:	Delivers appropriate index.
 * Parameters:	index_name	- (i) index number.
 * Returns:	Root of required index.
 * Remarks:	Currently two indices available: PRIM_INDEX and SECOND_INDEX.
 *		No parameter checking.
 */
tree_index *
get_index_by_name(const int index_name)
{
  return (index_name ? &secondary_index : &primary_index);
}//get_index_by_name

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
		  const int cluster_size)
{
  tree_index	*tip;
  int		i;

#ifdef DEBUG
  cerr << (to_register ? "register" : "find") << "_at_level called with "
       << discriminator;
#endif

  if (table_node.counter == 0) {
    if (!to_register)
      return NULL;
    tip = table_node.down.indx = new tree_index[cluster_size + 1];
    table_node.counter = cluster_size + 1;
    for (i = 0; i <= cluster_size; i++) {
      tip[i].counter = 0;
      tip[i].allocated = 0;
      tip[i].down.indx = NULL;
    }
  }
  tip = table_node.down.indx;
  return ((tip[discriminator].counter || to_register) ? tip + discriminator
	                                              : (tree_index *)NULL);
}//register_at_level

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
 */
int
cmp_nodes(const node *node1, const node *node2)
{
  int		c;
  long int	l;
  arc_node	*p1, *p2;

  // compare children
  p1 = node1->children; p2 = node2->children;
  for (int i = 0; i < node1->no_of_children; i++, p1++, p2++)
    if ((l = ((long int)(p1->child) - (long int)(p2->child))) != 0)
      return ((l < 0) ? -1 : 1);
    else if ((c = p1->letter - p2->letter) != 0)
      return c;
    else if ((c = p1->is_final - p2->is_final) != 0)
      return c;

  // here nodes are isomorphic
#ifdef DEBUG
  cerr << "Nodes are isomorphic\n";
#endif
  return 0;
}//cmp_nodes

/* Name:	destroy_index
 * Class:	None
 * Purpose:	Destroys index structure (releasing memory!).
 * Parameters:	index_root	- (i) index to destroy.
 * Returns:	Nothing.
 * Remarks:	Nodes pointed to in the index are not destroyed.
 */
void
destroy_index(tree_index &index_root)
{
  tree_index	*tip1, *tip2;

  tip1 = index_root.down.indx;
  for (int i = 0; i < index_root.counter; i++, tip1++) {
    tip2 = tip1->down.indx;
    for (int j = 0; j < tip1->counter; j++, tip2++) {
      if (tip2->counter)
	delete [] tip2->down.leaf;
    }
    delete [] tip1->down.indx;
  }
  delete [] index_root.down.indx;
  index_root.counter = 0;
}//destroy_index



/* Name:	bsearch_reg
 * Class:	None
 * Purpose:	Find either a node (or a subnode), or a place where it should
 *		be inserted in a register.
 * Parameters:	n		- node to be found;
 *		low		- address of the first pointer to node
 *					in register page;
 *		high		- address of the last pointer to node
 *					in register page;
 *		cmp		- function that compares two nodes;
 *		group_size	- for pseudonodes, number of arcs forming
 *					a pseudonode.
 * Returns:	Address of a pointer to node n, or a place where that pointer
 *		should be placed.
 * Remarks:	None.
 */
node **
bsearch_reg(const node *n, node **low, node **high,
	    int (*cmp)(const node *, const node *, int, int),
	    int offset, int group_size)
{
  node	**mid;
  int		c;

#ifdef DEBUG
  // Warning: this works for primary index - how should I know this is it?
  for (mid = low; mid < high; mid++)
    if (cmp(*mid, mid[1], offset, group_size) >= 0) {
      cerr << "Error in index: order not preserved" << endl;
      cerr << "Offending nodes: " << hex << (long)(*mid) << " and "
	   << mid[1] << endl;
      cerr << "were found in a sequence:" << endl;
      for (mid = low; mid <= high; mid++)
	print_node(*mid);
      exit(20);
    }
#endif

  while (low <= high) {
    mid = low + ((high - low) / 2);
    if ((c = cmp(n, *mid, offset, group_size)) < 0)
      high = mid - 1;
    else if (c > 0)
      low = mid + 1;
    else
      return mid;
  }
  return high + 1;
}//bsearch_reg


/* Name:	find_or_register
 * Class:	None.
 * Purpose:	Finds an isomorphic subgraph in the automaton or registers
 *		a new node.
 * Parameters:	n		- (i) node to be found or registered;
 *		index_root	- (i/o) which index to use;
 *		register_it	- (i) if TRUE, register the node,
 *					if FALSE, find an isomorphic node.
 * Returns:	Pointer to an isomorphic node - if a node isomorphic to n
 *		found in the index, or NULL otherwise.
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
 *
 *		Changed:
 *		If register_it is true, if a node isomorphic to n is:
 *		found		- do not register, return isomorphic node;
 *		not found	- register, return NULL.
 *		This should speed up the code, as this function is
 *		rarely invoked for finding only, normally it is a
 *		sequence: find, and then register if not found.
 */
node *
find_or_register(node *n, const int index_root_name,
		 const int register_it)
{
  tree_index	*tip;		// tree index pointer
  node		**current_node;	// subsequent entry in the leaf table
  int		not_found = TRUE;
  int		i;
  tree_index	*index_root;
  unsigned char	uc = (unsigned char)' ';

  index_root = get_index_by_name(index_root_name);

  // searching for the number of children entry
  if (n->get_no_of_kids())
    uc = n->get_children()->letter;
  tip = (index_root_name ?
	 // secondary index
	 register_at_level(uc, *index_root, register_it, 255)
	 // primary index
	 : register_at_level(n->get_no_of_kids(), *index_root, register_it));
  if (tip == NULL)
    return NULL;

  // searching according to hash function value
  tip = register_at_level(n->hash(0, n->get_no_of_kids()), *tip, register_it);
  if (tip == NULL)
    return NULL;


  // now we search the lowest level, which has entries pointing directly
  // to end nodes
  not_found = TRUE;
  i = 0;
  current_node = tip->down.leaf;

  if (tip->down.leaf) {
    current_node = bsearch_reg(n, current_node,
			       current_node + (tip->counter - 1),
			       (index_root_name ? part_cmp_nodes :
				                  a_cmp_nodes),
			       -1, -1);
    not_found = !(current_node - tip->down.leaf < tip->counter &&
                  (index_root_name ? part_cmp_nodes(n, *current_node)
		                   : cmp_nodes(n, *current_node)) == 0);
  }
  i = current_node - tip->down.leaf;

  if (not_found) {

    if (!register_it)
      return NULL;	// do not register

    // now `i' is the item number where we should put (`register') our new node
    current_node = tip->down.leaf;
    if (++(tip->counter) > tip->allocated) {
      node **p = new node *[tip->allocated += INDEX_SIZE_STEP];
      node **new_children = p;
      for (int j1 = 0; j1 < i; j1++)
	*p++ = *current_node++;
      *p++ = n;
      for (int j2 = i + 1; j2 < tip->counter; j2++)
	*p++ = *current_node++;
      if (tip->allocated > INDEX_SIZE_STEP)
	delete [] tip->down.leaf;
      tip->down.leaf = new_children;
    }
    else {
      for (int j = tip->counter - 1; j > i; --j)
	current_node[j] = current_node[j-1];
      current_node[i] = n;
    }

#ifdef DEBUG
    cerr << "Node registered\n";
#endif

    // node registered
    return NULL;	// because node not found
  }//if not found

  // now `current_node->child' points to a node isomorphic to n
  if (register_it) {
#ifdef OLD_CODE
    cerr << "Attempt to register already registered node\n";
    cerr << "Offending node is: " << endl;
    print_node(n);
    cerr << "Node found in index is:" << endl;
    print_node(*current_node);
    return NULL;
#endif
    return *current_node;
  }

  return *current_node;
}//find_or_register

/* Name:	unregister_node
 * Class:	None.
 * Purpose:	Removes a node from the primary register.
 * Parameters:	n		- (i) the node to be unregistered.
 * Returns:	TRUE if the node was unregistered successfully,
 *		FALSE otherwise.
 * Remarks:	Needed for index a tergo when generalizing.
 */
int
unregister_node(const node *n)
{
  tree_index	*tip;		// tree index pointer
  node		**current_node;	// subsequent entry in the leaf table
  int		not_found = TRUE;
  int		i;
  tree_index	*index_root;
  unsigned char	uc = (unsigned char)' ';

  index_root = get_index_by_name(0);

  // searching for the number of children entry
  if (n->get_no_of_kids())
    uc = n->get_children()->letter;
  tip = register_at_level(n->get_no_of_kids(), *index_root, FALSE);
  if (tip == NULL)
    return FALSE;

  // searching according to hash function value
  tip = register_at_level(n->hash(0, n->get_no_of_kids()), *tip, FALSE);
  if (tip == NULL)
    return FALSE;


  // now we search the lowest level, which has entries pointing directly
  // to end nodes
  not_found = TRUE;
  i = 0;
  current_node = tip->down.leaf;

  current_node = bsearch_reg(n, current_node,
			     current_node + tip->counter - 1,
			     a_cmp_nodes, -1, -1);
  not_found = !(current_node - tip->down.leaf < tip->counter &&
                cmp_nodes(n, *current_node) == 0);
  i = current_node - tip->down.leaf;

  if (not_found)
    return FALSE;

  if (--(tip->counter) - i > 0)
    memmove(current_node, current_node + 1,
	    (tip->counter - i) * sizeof(node *));
  return TRUE;
}//unregister_node



/* Name:	register_subnodes
 * Class:	None (friend of class node).
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
register_subnodes(node *c_node, const int group_size)
{
  int		i, j;
  node		**current_node, **new_node;
  tree_index	*tip;
  tree_index	*index_root;
  int		not_found;

#ifdef DEBUG
  cerr << "Registering subnodes of " << long(c_node) << " " << group_size
       << "/" << c_node->no_of_children << "\n";
#endif
  index_root = get_index_by_name(SECOND_INDEX);
#ifdef STOPBIT
  int offset = c_node->no_of_children - group_size;
#else
  for (int offset=0; offset <= c_node->no_of_children - group_size; offset++){
#endif

    // Register in the index of number of children (should be 1 entry)
    // tip = register_at_level(group_size, *index_root, REGISTER);
    // Change to the above:
    // Register in the index of the first arc letter
    // 255 is the highest possible code of a letter
    unsigned char uc = (unsigned char)(c_node->children[offset].letter);
    tip = register_at_level((int)uc, *index_root, REGISTER, 255);

    // Register in the index of hash function value
    tip = register_at_level(c_node->hash(offset, group_size), *tip, REGISTER);

    // Register in the lowest level table
    not_found = TRUE;
    current_node = tip->down.leaf;
    if (tip->down.leaf) {
      current_node = bsearch_reg(c_node, current_node,
				 current_node + (tip->counter - 1),
				 part_cmp_nodes, offset, group_size);
      not_found = !(current_node - tip->down.leaf < tip->counter &&
	            part_cmp_nodes(c_node, *current_node, offset,
				   group_size) == 0);
    }
    i = current_node - tip->down.leaf;

    if (not_found) {
      // Node not found, register it
      current_node = new_node = tip->down.leaf;
      if (tip->counter >= tip->allocated) {
	new_node = new node *[tip->allocated += INDEX_SIZE_STEP];
	for (j = 0; j < i; j++)
	  new_node[j] = current_node[j];
      }
      for (j = tip->counter; j > i; --j)
	new_node[j] = current_node[j-1];
      new_node[i] = c_node;
      if (new_node != current_node) {
	if (tip->allocated != INDEX_SIZE_STEP)
	  delete [] current_node;
	tip->down.leaf = new_node;
      }
      tip->counter++;
#ifdef STOPBIT
      return NULL;
#endif
    }
#ifdef STOPBIT
    else
      return *current_node;
#else
  }
#endif
}//register_subnodes
	

/* Name:	part_cmp_nodes
 * Class:	None (friend of class node).
 * Purpose:	Looks if it is possible to replace arcs of the smaller node
 *		with a subset of arcs of a larger node.
 * Parameters:	small_node	- (i/o) node to be replaced;
 *		big_node	- (i) node that may contain the same arcs;
 *		offset		- (i) (optional) if present, take to comparison
 *					only arcs #offset from small_node;
 *		group_size	- (i) (optional) how many arcs to consider.
 * Returns:	< 0 if group_size arcs of small_node from offset should precede
 *			the appropriate arcs of big_node in a list;
 *		= 0 if group_size arcs of small_node from offset are equal
 *			to the appropriate arcs of big_node;
 *		> 0 if group_size arcs of small_node from offset should follow
 *			the appropriate arcs of big_node in a list.
 * Remarks:	Let the small node have arcs (c d f) at offset.
 *		If the big node is (b c d f g) we compare:
 *		(b c d f g)  (b c d f g)  (a b c f g)  (a b c f g)
 *		(c d f)        (c d f)<-found! (return 0)
 *
 *		But for the big node being (b c d e f g):
 *		(b c d e f g)  (b c d e f g)  (b c d e f g)  (b c d e f g)
 *		(c d f)          (c d f)          (c d f)          (c d f)
 *		it does not work. First, the nodes are aligned, then compared
 *		arc after arc:
 *		(b c d e f g)
 *		  (c d f)
 *		       \--difference found (returning > 0)
 *
 *		Maybe by chaging the order of arcs more arcs could be slashed.
 */
int
part_cmp_nodes(const node *small_node, const node *big_node, const int offset,
	       int group_size)
{
  arc_node	*sp, *bp;
  int		big_offset;
  int		os = offset;
  int		gs = group_size;

  if (os == -1) {
    os = 0;
    gs = small_node->no_of_children;
  }
  int l = big_node->no_of_children - gs;
  bp = big_node->children;
  sp = small_node->children + os;

  // Find which arcs of big_node form a pseudonode
  for (big_offset = 0; big_offset <= l; big_offset++, bp++)
    if (sp->letter == bp->letter)
      break;

  // Check for errors (this can be removed later for speed
  if (sp->letter != bp->letter) {
    cerr << "Error in secondary index\n";
    cerr << "First arc label of the smaller node not found in the bigger one"
	 << endl;
    cerr << "Smaller node:" << endl;
    print_node(small_node);
    cerr << "Bigger node:" << endl;
    print_node(big_node);
    exit (-20);
  }

  // Now compare arc by arc
  for (int i = 0; i < gs; i++, sp++, bp++) {
    if (sp->child != bp->child)
      return (sp->child < bp->child ? -1 : 1);
    else if (sp->letter != bp->letter)
      return (sp->letter - bp->letter);
    else if (sp->is_final != bp->is_final)
      return (sp->is_final - bp->is_final);
    //else compare more arcs...
  }

  // Here the subnodes must be equal
  return 0;
}//part_cmp_nodes



/* Name:	get_pseudo_offset
 * Class:	None (friend of class node).
 * Purpose:	Looks if it is possible to replace arcs of the smaller node
 *		with a subset of arcs of a larger node.
 * Parameters:	small_node	- (i/o) node to be replaced;
 *		big_node	- (i) node that may contain the same arcs;
 * Returns:	Number of the arc of big_node that is the first of
 *		small_node->no_of_children arcs that are identical to the arcs
 *		of small_node, or -1 if such arcs not found.
 * Remarks:	Let the small node have arcs (c d f).
 *		If the big node is (b c d f g) we compare:
 *		(b c d f g)  (b c d f g)  (a b c f g)  (a b c f g)
 *		(c d f)        (c d f)<-found(offset 1)!
 *
 *		But for the big node being (b c d e f g):
 *		(b c d e f g)  (b c d e f g)  (b c d e f g)  (b c d e f g)
 *		(c d f)          (c d f)          (c d f)          (c d f)
 *		it does not work (offset -1).
 *
 *		Maybe by chaging the order of arcs more arcs could be slashed.
 *
 *		Note that small_node IS NOT a pseudonode.
 */
int
get_pseudo_offset(const node *small_node, const node *big_node)
{
  arc_node	*sp, *bp;
  int		big_offset;

  if (big_node->big_brother) {
    // big node has a bigger brother
    // since we must have looked at that brother earlier we do not waste time
    // on dealing with a node containing only a subset of what he had
    return -1;
  }

  // Find where the pseudonode in big_node begins
  int l = big_node->no_of_children - small_node->no_of_children;
  sp = small_node->children;
  bp = big_node->children;
  for (big_offset = 0; big_offset <= l; big_offset++, bp++)
    if (sp->letter == bp->letter)
      break;
  return ((sp->letter != bp->letter) ? -1 : big_offset);
  // Since the big_node has already been found by find_or_register
  // there is no need to check the arcs...
}//get_pseudo_offset

#ifdef MORE_COMPR
/* Name:	match_subset
 * Class:	None.
 * Purpose:	Find if there is a node in the register that consists
 *		of a subset of arcs of the present node. If there is one,
 *		rearrange the arcs of the node so that the arcs corresponding
 *		to the node in the register are grouped at the end
 *		of the node. Then link nodes so that the node becomes
 *		the big brother of the smaller node from the register.
 * Parameters:	n		- (i/o) the node to be analyzed;
 *		kids_tab	- (i) a table with numbers of arcs
 *					of nodes in the automaton,
 *					kids_tab[0] - the biggest no of kids,
 *					kids_tab[1] - the second biggest,etc.;
 *		kids_no_no	- (i) number of entries in kids_tab.
 * Returns:	True if a match found, false otherwise.
 * Remarks:	The arcs in all nodes are ordered in the same way,
 *		so it is sufficient to select arcs.
 *		We must allocate memory for arcs dynamically, as it is
 *		released by the destructor of node.
 */
int
match_subset(node *n, const int kids_tab[], const int kids_no_no)
{
  if (n->no_of_children > MAX_ARCS_TO_SHUFFLE) {
    return false;
  }
  static node		node_template;
  arc_node *arc_template =  new arc_node[MAX_ARCS_PER_NODE];
  node_template.children = arc_template;
  for (int i = 1; i < kids_no_no; i++) {
    int subset_size = kids_tab[i];
    if (subset_size <= MAX_SUBSET_SHUFFLE) {
      node_template.no_of_children = subset_size;
      if (match_part(n, &node_template, subset_size, 0, subset_size))
	return TRUE;
    }
  }
  return FALSE;
}//match_subset

/* Name:	match_part
 * Class:	None.
 * Purpose:	Creates another subset of arcs of a node.
 * Parameters:	n		- (i/o) source of subsets;
 *		nn		- (i/o) temporary node;
 *		to_do		- (i) number of arcs to complete;
 *		start_at	- (i) start selecting arcs from that arc;
 *		subset_size	- (i) number of arcs in the node
 *					to be created;
 *		reg_tails	- (i) TRUE if tails are to be registered.
 * Returns:	TRUE if an isomorphic node found, FALSE otherwise.
 * Remarks:	None.
 */
int
match_part(node *n, node *nn, const int to_do, const int start_at,
	   const int subset_size, const int reg_tails)
{
  node	*isomorphic;

  if (to_do == 0) {
    // subset complete
    if ((isomorphic = find_or_register(nn, PRIM_INDEX, reg_tails)) != NULL) {
      // check if a link can be created
      if (isomorphic->big_brother ||
	  isomorphic->free_end != isomorphic->no_of_children)
	return FALSE;
      // reorder arcs
      int nk = n->no_of_children - subset_size;
      for (int j = 0; j < isomorphic->no_of_children; j++) {
	int k = 0;
	while (n->children[k] != isomorphic->children[j]) k++;
	if (k != nk + j) {
	  // exchange arcs in n
	  arc_node temp = n->children[nk + j];
	  n->children[nk + j] = n->children[k];
	  n->children[k] = temp;
	}
	k++;
      }
      // link nodes
      isomorphic->set_link(n, nk);
      // cerr << "A node linked!\n";
      return TRUE;
    }
    return FALSE;
  }
  if (start_at >= n->no_of_children) return FALSE;
  for (int i = start_at; i <= n->no_of_children - to_do; i++) {
    // the arc must point to a node pointed to by another node
    if (n->children[i].child && n->children[i].child->hit_node(0) > 1) {
      nn->children[subset_size - to_do] = n->children[i];
      if (match_part(n, nn, to_do - 1, i + 1, subset_size, reg_tails))
	return TRUE;
    }
  }
  return FALSE;
}//match_part
#ifdef STOPBIT
#ifdef TAILS
/* Name:	match_tails
 * Class:	None.
 * Purpose:	Tries to find an isomorphic tail of given length.
 * Parameters:	n		- (i) node to be tested;
 *		tail_size	- (i) size of the tail to be found.
 * Returns:	TRUE if the tail found, false otherwise.
 * Remarks:	Memory for arc_template has to allocated dynamically
 *		because it is disposed of by the destructor.
 */
int
match_tails(node *n, const int tail_size)
{
  static node	node_template;
  arc_node *arc_template = new arc_node[MAX_ARCS_PER_NODE];

  node_template.set_children(arc_template, tail_size);
  return match_part(n, &node_template, tail_size, 0, tail_size, TRUE);
}//match_tails
#endif
#endif
#endif

#ifdef STOPBIT
#ifdef TAILS
/* Name:	check_tails
 * Class:	None, friend of class node.
 * Purpose:	Tries to find if a tail of the specified size of the specified
 *		node has already been registered in the secondary index.
 *		If so, a link should be created between this node,
 *		and the parent node of the registered tail. If not, then
 *		registers the tail.
 * Parameters:	n		- (i/o) the node whose tail is to be registerd;
 *		tail_size	- (i) current tail size.
 * Returns:	TRUE if isomorphic tail found, FALSE otherwise.
 * Remarks:	We must allocate space for arcs dynamically, as it is
 released
 *		by the destructor for automatic objects.
 */
int
check_tails(node *n, const int tail_size)
{
  // cerr << "check_tails called with:"; print_node(n);

  node		*isomorphic;
  // Try to register the tail of this node
  if ((isomorphic = register_subnodes(n, tail_size))
#ifdef MORE_COMPR
      && isomorphic->free_end >= tail_size
#endif
      ) {
    // Registering failed because there is an isomorphic tail already
    // Link the nodes.
    if (n->get_big_brother() != NULL || isomorphic->get_big_brother() != NULL)
      return FALSE;
    if (n->brother_offset != (unsigned char)-1) {
      // We move the tail of n to isomorphic
      n->set_link(isomorphic, isomorphic->get_no_of_kids() - tail_size);
      n->free_beg = n->get_no_of_kids() - tail_size;
    }
    else if (isomorphic->brother_offset == (unsigned char)-1)
      return FALSE;
    else {
      // The node n is already a big_brother of another node,
      // but we can store the tail of isomorphic in n
      isomorphic->set_link(n, n->get_no_of_kids() - tail_size);
      isomorphic->free_beg = isomorphic->get_no_of_kids() - tail_size;
    }
    // arcs_knocked += tail_size;
    return TRUE;
  }
  return FALSE;
}//check_tails
#endif
#endif

#if defined(FLEXIBLE) && defined(STOPBIT) && defined(TAILS)
/* Name:	contains_many_nodes
 * Class:	None.
 * Purpose:	Checks if a particular branch of the primary index contains
 *		more than 1 node.
 * Parameters:	n	- (i) number of children.
 * Returns:	TRUE if in the primary index there is more than 1 node
 *		that has n children, FALSE otherwise.
 * Remarks:	The primary register has 3 levels. Each level contains
 *		information on the subsequent level. The levels are:
 *		number of children, hash value, all. The top level has
 *		no information on the number of children in the whole index.
 */
int
contains_many_nodes(int n)
{
  tree_index	*prim_index, *tip1;
  int		count;

  count = 0;
  prim_index = get_index_by_name(PRIM_INDEX);
  tip1 = prim_index->down.indx + n;
  for (int j = 0; j < tip1->counter; j++) {
    if (tip1->down.indx[j].counter) {
      count++;
      if (count > 1)
	return TRUE;
      if (tip1->down.indx[j].counter > 1)
	return TRUE;
    }
  }
  return FALSE;
}//contains_many_nodes



#endif

/* Name:	share_arcs
 * Class:	None.
 * Purpose:	Tries to find if arcs of one node are a subset of another.
 *		If so, a reference to them is replaced by a reference
 *		to a part (a subset) of the other node.
 * Parameters:	root		- (i) root of the automaton.
 * Returns:	Number of suppressed arcs.
 * Remarks:	This must take A LOT of time!
 *
 *		The algorithm:
 *		Let n1 be the second biggest number of children per node
 *		in the automaton.
 *		Take all consecutive sequences of n1 arcs from each of
 *		nodes with the biggest number of arcs and register them
 *		as nodes in the secondary index.
 *		For each node with n1 children, search for an isomorphic
 *		node in the secondary index. If found, set a link
 *		from the smaller node to the bigger one.
 *
 *		IMPORTANT note or myself: with STOPBIT, sharing can be used
 *		together with numbering information, provided the nodes
 *		are not included entirely one in another.
 */
int
share_arcs(node *root)
{
  tree_index	*prim_index;
  tree_index	*tip1, *tip2, *tip3;
  node		**tip4;
  node		*new_node;
  int		nodes_knocked, arcs_knocked;
  int		i, j, l, offset;
#ifdef STOPBIT
#ifdef TAILS
  int		k;
#endif
#endif
  int		prime_kids_tab[MAX_ARCS_PER_NODE + 1];
  				/* [0] - biggest no of children per node in
  					 the automaton
				   [1] - 2nd biggest no of children
				   ...
				   [prime_kids-1] - 1 child */
  int		prime_kids;	/* number of different numbers of children
				   per node in the automaton */
#ifdef SORT_ON_FREQ
  extern int	frequency_table[256];	// no of occurences of labels
#endif


#ifdef PROGRESS
  cerr << "Sharing arcs" << endl;
#endif
  nodes_knocked = 0; arcs_knocked = 0;
  prim_index = get_index_by_name(PRIM_INDEX);

#ifdef DEBUG
  cerr << "Primary index (before sorting on freq.)\n";
  show_index(PRIM_INDEX);
#endif

#ifdef	SORT_ON_FREQ
  // sort arcs on frequency
  for (i = 0; i < 256; i++)
    frequency_table[i] = 0;
  root->count_arcs(frequency_table);
  root->sort_arcs();

#ifdef DEBUG
  cerr << "Primary index (after sorting)\n";
  show_index(PRIM_INDEX);
#endif
#endif

  prime_kids = 0;
  // start from nodes with the biggest number of children
  tip1 = prim_index->down.indx + prim_index->counter - 1;

  for (i = prim_index->counter - 1; i >= 0; --i, --tip1) {
    if (tip1->counter) {
      prime_kids_tab[prime_kids++] = i;
    }
  }

  for (i = 1; i < prime_kids; i++) {
    tip1 = prim_index->down.indx + prime_kids_tab[i];


    // Now register pseudonodes consisting of sets of `prime_kids_tab[i]' arcs
    // taken from the nodes that have more than `prime_kids_tab[i]' arcs
    for (j = 0; j < i; j++) {
      tip2 = prim_index->down.indx + prime_kids_tab[j];

      // We have passed # of children index, now go through hash function
      tip3 = tip2->down.indx;
      for (int k = 0; k < tip2->counter; k++, tip3++) {
	if (tip3->counter == 0)
	  continue;

	// Now we register each set of tip->data arcs
	tip4 = tip3->down.leaf;
	for (l = 0; l < tip3->counter; l++, tip4++) {
	  if ((*tip4)->get_big_brother() == NULL)
	    register_subnodes(*tip4, prime_kids_tab[i]);
	}
      }
    }

    // Secondary index created, now look for nodes in primary index
#ifdef DEBUG
    cerr << "Secondary index for " << prime_kids_tab[i] << " arcs\n";
    show_index(SECOND_INDEX);
#endif
    tip3 = tip1->down.indx;
    for (j = 0; j < tip1->counter; j++, tip3++) {
      if (tip3->counter == 0)
	continue;

      tip4 = tip3->down.leaf;
      for (l = 0; l < tip3->counter; l++, tip4++) {
	if ((new_node = find_or_register(*tip4, SECOND_INDEX, FIND))) {
	  // Node *tip4 (taken from primary index) was found in secondary
	  // index (of subnodes), so it can be replaced by a reference
	  // to a set of arcs of the node `new_node'. So we do that.

	  if ((offset = get_pseudo_offset(*tip4, new_node)) >= 0) {
	    (*tip4)->set_link(new_node, offset);
	    nodes_knocked++;
	    arcs_knocked += prime_kids_tab[i];
	  }
	  else {
	    cerr << "Error in index\n";
	    cerr << "While looking for " << (long)(*tip4) << " found "
	      << (long)new_node << "\n";
	    cerr << (long)(*tip4) << " has bb "
	      << (long)((*tip4)->get_big_brother()) << "\n";
	  }
	}
      }
    }

    // Now secondary index for tip1->data number of nodes is no longer needed
    // Destroy the index to make room for other data
    tip3 = get_index_by_name(SECOND_INDEX);
    destroy_index(*tip3);
  }

#ifdef MORE_COMPR
  // Take nodes from the primary index in groups of equal number of arcs.
  // Begin from the biggest nodes.
  // Take subsets of arcs from those nodes, treat them as new nodes,
  // and try to find if there are other nodes in the register that
  // are isomorphic to those new nodes. If so, create links.

#ifdef PROGRESS
  cerr << "Changing the order of transitions" << endl;
#endif

  // Begin with the largest nodes, proceed to smaller
  for (i = 0; i < prime_kids - 1; i++) {
#ifdef PROGRESS
    cerr << "  Considering states with " << prime_kids_tab[i] << " states.\n";
#endif
    tip1 = prim_index->down.indx + prime_kids_tab[i];
    // Go through hash level
    tip3 = tip1->down.indx;
    for (j = 0; j < tip1->counter; j++, tip3++) {
      if (tip3->counter == 0)
	continue;

      tip4 = tip3->down.leaf;	// the leaf level
      for (l = 0; l < tip3->counter; l++, tip4++) {
	if ((*tip4)->get_big_brother() == NULL
	    && (*tip4)->free_end == (*tip4)->get_no_of_kids()) {
	  if (match_subset(*tip4, prime_kids_tab + i, prime_kids - i)) {
	    nodes_knocked++;
	    arcs_knocked += prime_kids_tab[i];
	  }
	}
      }
    }
  }
#endif

#ifdef STOPBIT
#ifdef TAILS
  // Share tails of nodes, i.e. their last arcs.
  // This is done by registering the tails in the secondary index.
  // We start with the biggest tails possible, moving towards smaller ones,
  // creating an index for the given tail size.
  // We try to register the tails, and if we fail, then it means we found
  // isomorphic tails, and we can create appropriate links.

#ifdef PROGRESS
  cerr << "Changing the order of transitions in tails" << endl;
#endif
  // Try with tails one arc smaller then the largest node.
  // If there is one largest node, then start with tails of the size
  // one arc smaller then the second largest one (we do not start with
  // the size of the second largest one, because that case should have been
  // handled above).
  if (prime_kids > 1) {
    for (i = prime_kids_tab[(contains_many_nodes(prime_kids_tab[0])?0:1)] - 1;
	 i > 0; --i) {
      // Now `i' is the current tail size.
      // Examine all nodes bigger than that size
      for (j = 0; prime_kids_tab[j] > i; j++) {
	tip1 = prim_index->down.indx + prime_kids_tab[j];
	// Go through the hash level
	tip3 = tip1->down.indx;
	for (k = 0; k < tip1->counter; k++, tip3++) {
	  if (tip3->counter == 0)
	    continue;

	  tip4 = tip3->down.leaf;	// the leaf level
	  for (l = 0; l < tip3->counter; l++, tip4++) {
	    if ((*tip4)->free_beg == 0 && (*tip4)->get_big_brother() == NULL
#ifdef MORE_COMPR
		&& (*tip4)->free_end >= i
#endif
		) {
#ifdef MORE_COMPR
	      if (!check_tails(*tip4, i))
		if ((*tip4)->get_brother_offset() != (unsigned char)-1)
		  match_tails(*tip4, i);
#else
	      check_tails(*tip4, i);
#endif
	    }
	  }
	}
      }
      tip3 = get_index_by_name(SECOND_INDEX);
      destroy_index(*tip3);
    }
  }
#endif
#endif

#ifdef JOIN_PAIRS
#ifndef STOPBIT
  pair_registery	pr;
#ifdef PROGRESS
  cerr << "Merging pairs" << endl;
#endif
  pr.merge_pairs();
#endif
#endif
#ifdef DEBUG
  cerr << "Slashed " << nodes_knocked << " nodes and " << arcs_knocked
       << " arcs\n";
#endif
  return arcs_knocked;
}//share_arcs


#ifdef JOIN_PAIRS
/* Name:	pair_registery
 * Class:	pair_registery
 * Purpose:	Create a table of pointers to nodes with two arcs.
 * Parameters:	None.
 * Returns:	Nothing (constructor).
 * Remarks:	None.
 */
pair_registery::pair_registery(void)
{
  for (int i = 0; i < 256; i++)
    pntr[i] = NULL;
}//pair_registery::pair_registery


/* Name:	~pair_registery
 * Class:	pair_registery
 * Purpose:	Deallocate memory (destructor).
 * Parameters:	None.
 * Returns:	Nothing.
 * Remarks:	None.
 */
pair_registery::~pair_registery(void)
{
  for (int i = 0; i < no_of_pairs; i++)
    if (packed[i].p)
      delete packed[i].p;
  delete packed;
}//pair_registery::~pair_registery


/* Name:	reg
 * Class:	pair_registery
 * Purpose:	Register a two arc node.
 * Parameters:	n	- (i) the node to be registered.
 * Returns:	Nothing.
 * Remarks:	It is assumed that n has two children.
 *		Registering is made in a 2-dimensional table, but the entries
 *		are transfered afterwards to a vector.
 */
void
pair_registery::reg(node *n)
{
  unsigned char	c1, c2;
  pair_reg	*p;
  pair_reg	**p1;
  node	**p2;

  c1 = unsigned(n->children[0].letter);
  c2 = unsigned(n->children[1].letter);
  if ((p1 = pntr[c1]) == NULL) {
    p1 = pntr[c1] = new pair_reg *[256];
    for (int j = 0; j < 256; j++)
      p1[j] = NULL;
  }
  if ((p = p1[c2]) == NULL) {
    p = p1[c2] = new pair_reg;
    p->counter = 0;
    p->allocated = 0;
  }
  if (p->counter >= p->allocated) {
    p->allocated += PAIR_REG_LEN;
    p2 = new node *[p->allocated];
    for (int i = 0; i < p->counter; i++)
      p2[i] = p->nodes[i];
    if (p->allocated != PAIR_REG_LEN)
      delete p->nodes;
    p->nodes = p2;
  }
  p->nodes[p->counter++] = n;
}//pair_registery::reg



/* Name:	prepare
 * Class:	pair_registery
 * Purpose:	Transforms a two dimensional table of pointers with lots
 *		of gaps into a packed vector.
 * Parameters:	None.
 * Returns:	Nothing.
 * Remarks:	Counting must be done first to get the size of the vector.
 */
void
pair_registery::prepare(void)
{
  int		i, j;

  no_of_pairs = 0;
  for (i = 1; i < 255; i++)
    for (j = i+1; j < 256; j++)
      if (pntr[i] && pntr[i][j])
	no_of_pairs++;

  packed = new packed_tab[no_of_pairs];

  pair_reg *p;
  no_of_pairs = 0;
  for (i = 1; i < 255; i++) {
    for (j = i+1; j < 256; j++)
      if (pntr[i] && (p = pntr[i][j])) {
	packed[no_of_pairs].p = p;
	packed[no_of_pairs].c1 = i;
	packed[no_of_pairs++].c2 = j;
      }
    if (pntr[i])
      delete pntr[i];
  }
}//pair_registery::prepare


/* Name:	join_arcs
 * Class:	pair_registery
 * Purpose:	Joins nodes that have two arcs, and one arc is identical.
 * Parameters:	None.
 * Returns:	Nothing.
 * Remarks:	Nodes are already in a vector.
 *		Only pairs are merged. For future versions:
 *		It is possible to join more than two arcs in this way, e.g.
 *
 *		(a, b)(c, d)
 *		   (b, c)(d, e)
 *
 *		etc. In that case, the big_brother pointer in the first node
 *		(i.e. the big brother of the other nodes that are merged)
 *		should point to a table of pointers to nodes, and the offset
 *		of the big brother should be the number of nodes in that
 *		table with minus (to indicate it is a big brother).
 */
void
pair_registery::join_arcs(void)
{
  int		nop1 = no_of_pairs - 1;

  for (int i = 0; i < nop1; i++) {
    char c2 = packed[i].c2;

    // choose a node with (c2,c3)
    for (int j = i + 1; j < no_of_pairs; j++)
	if (packed[j].c1 == c2) {
	  /* Now 2 vectors of nodes have been found that have arcs labelled
	     (c1, c2), (c2, c3). Having the same labels
	     is not sufficient: the arcs must be identical, i.e.
	     they must lead to the same nodes, and have the same
	     "final" property.
	     */
	  pair_reg *p1 = packed[i].p;
	  node **pp1 = p1->nodes;
	  for (int i1 = 0; i1 < p1->counter; i1++, pp1++) {
	    node *pc2 = (*pp1)->children[1].child;
	    int f2 = (*pp1)->children[1].is_final;

	    pair_reg *p2 = packed[j].p;
	    node **pp2 = p2->nodes;
	    for (int j1 = 0; j1 < p2->counter; j1++, pp2++) {
	      if ((*pp2)->children[0].child != pc2)
		continue;
	      if ((*pp2)->children[0].is_final != f2)
		continue;

	      /* Now we have 2 nodes that can be merged:
		 (c1, c2)
		     (c2, c3)
		 in a bigger node: (c1, c2, c3).
		 Node (c2, c3) has the node (c1, c2) as its big brother.
		 brother_offset is set to 1.
		 Node (c1, c2) receives brother_offset = -1, which indicates
		 that it is the big brother.
		 */

	      if ((*pp1)->big_brother) {
		cerr << "Node 1 has already a big brother!\n";
		cerr << "Node 1 is:\n";
		print_node(*pp1);
		cerr << "and its big brother is:\n";
		print_node((*pp1)->big_brother);
		exit(21);
	      }
	      if ((*pp2)->big_brother) {
		cerr << "Node 2 has already a big brother!\n";
		cerr << "Node 2 is:\n";
		print_node(*pp2);
		cerr << "and its big brother is:\n";
		print_node((*pp2)->big_brother);
		exit(21);
	      }
	      (*pp1)->set_link(*pp2, -1);
	      (*pp2)->set_link(*pp1, 1);

	      /* Nodes are merged, but pp2 must be removed from the register,
		 in order not to be considered for joining once again.
		 Removing pp1 is also needed; it wold be considered
		 again after choosing different c3.
		 */
	      for (int k = j1 + 1; k < p2->counter; k++, pp2++)
		pp2[0] = pp2[1];
	      --(p2->counter);

	      node **ppp = pp1;
	      for (int l = i1 + 1; l < p1->counter; l++, ppp++)
		ppp[0] = ppp[1];
	      --(p1->counter);
	      break;
	    }//for j1
	  }//for i1
	}//if node (c2, c3) found
  }//for i
}//pair_registery::join_arcs


/* Name:	merge_pairs
 * Class:	pair_registery
 * Purpose:	Launches the process of merging arcs.
 * Parameters:	None.
 * Returns:	Nothing.
 * Remarks:	None.
 */
void
pair_registery::merge_pairs(void)
{
  tree_index	*prim_index;
  node		**tip3;

  prim_index = get_index_by_name(PRIM_INDEX);
  tree_index *tip1 = prim_index->down.indx + 2;
  if (tip1 == NULL)
    return;
  tree_index *tip2 = tip1->down.indx;
  for (int i = 0; i < tip1->counter; i++, tip2++) {
    tip3 = tip2->down.leaf;
    for (int j = 0; j < tip2->counter; j++, tip3++)
      if ((*tip3)->big_brother == NULL)
	reg((*tip3));
  }
  prepare();
  join_arcs();
}//pair_registery::merge_pairs
  


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
show_index(const int index_name)
{
  int		kids_tab[MAX_ARCS_PER_NODE + 1];
  int		hash_tab[MAX_ARCS_PER_NODE + 1];
  int		kids, hashes;
  int		i, j, k, l, c;
  tree_index	*tip1;
  node		**tip2;
  arc_node	*tip3;

  tree_index *index_root = get_index_by_name(index_name);
  kids = 0;
  for (i = 0; i < index_root->counter; i++)
    if (index_root->down.indx[i].counter)
      kids_tab[kids++] = i;
  for (i = 0; i < kids; i++) {
    tip1 = index_root->down.indx + kids_tab[i];
    hashes = 0;
    cerr << "+-children: " << dec << kids_tab[i] << " (";
    for (j = 0; j < tip1->counter; j++)
      if (tip1->down.indx[j].counter)
	hash_tab[hashes++] = j;
    cerr << hashes << (hashes == 1 ? " entry)\n" : " entries)\n");
    for (j = 0; j < hashes; j++) {
      c = tip1->down.indx[hash_tab[j]].counter;
      cerr << (i < kids - 1 ? "| " : "  ") << "+-hash: " << dec << hash_tab[j]
	   << " (" << dec << c
	   << (c == 1 ? " entry)\n" : " entries)\n");
      tip2 = tip1->down.indx[hash_tab[j]].down.leaf;
      for (k = 0; k < tip1->down.indx[hash_tab[j]].counter; k++, tip2++) {
	cerr << (i < kids - 1 ? "| " : "  ") << (j < hashes - 1 ? "| " : "  ")
	     << "+-node: " << hex << (long)(*tip2);
	if ((*tip2)->get_big_brother())
	  cerr << "(->" << hex << (long)((*tip2)->get_big_brother()) << ")";
	cerr << "\n";
	tip3 = (*tip2)->get_children();
	for (l = 0; l < (*tip2)->get_no_of_kids(); l++, tip3++) {
	  cerr << (i < kids - 1 ? "| " : "  ")
	       << (j < hashes - 1 ? "| " : "  ")
	       << (k < tip1->down.indx[hash_tab[j]].counter - 1 ? "| " : "  ")
	       << "+-" << tip3->letter << (tip3->is_final ? "!" : " ")
	       << " " << hex << (long)(tip3->child)
	       << "\n";
	}
      }
    }
  }
  cerr << dec;
}//show_index
//#endif

/***	EOF nindex.cc	***/
