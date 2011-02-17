/***	unode.h	***/

/*	Copyright (c) Jan Daciuk, 1999-2004	*/

/* Name:	comp_or_reg
 * Class:	None.
 * Purpose:	Find if there is subgraph in the automaton that is isomorphic
 * 		to the subgraph that begins at the last child of this node.
 * Parameters:  n		- (i/o) node to be registered or replaced by
 *					an already existing node.
 * Returns:	Pointer to n or to the equivalent node that replaced it.
 * Remarks:	hit_count counts the number of (registered) links that lead
 *		to a node. hit_count = 0 means the node is not registered yet.
 *		hit_count is also used to avoid deleting nodes that are
 *		pointed to somewhere else in the transducer.
 *
 *		Note that hit_count is always increased for the unique node,
 *		regardless whether it is the original parameter of comp_or_reg,
 *		or a node found found somewhere else in the transducer.
 *		This means that in the situation where we modify an existing
 *		path in the transducer (path from the start node to
 *		the first reentrant node, 
 *		that is a node with more than one incoming arc), we should
 *		decrease hit_count for that node _before_ calling comp_or_reg.
 *		If an isomorphic node is found, then n can be deleted
 *		by delete_branch (note that delete_branch only deletes nodes
 *		that have hit_count equal to zero). Remember that the node
 *		n _must_ be unregistered first!!!
 *		If an isomorphic node is not found, then comp_or_reg simply
 *		restores the hit_count.
 */
node *
comp_or_reg(node *n);


/* Name:	mod_child
 * Class:	None.
 * Purpose:	Modify an arc (redirect link, or change final attribute).
 * Parameters:	parent_node	- (i/o) arc source node;
 *		child_node	- (i) arc destination node;
 *		arc_label	- (i) which arc;
 *		final		- (i) whether the arc should be final;
 *		child_already_deleted
 *				- (i) whether to delete child if different.
 * Returns:	Parent node.
 * Remarks:	None.
 */
node *
mod_child(node *parent_node, node *child_node, const char arc_label,
	  const int final, const int child_already_deleted = FALSE);


/***	EOF unode.h	***/
