/***	unode.cc	***/

/*
  This file contains functions needed for correct implementation
  of fsa_ubuild.
*/

/*	Copyright (c) Jan Daciuk, 1999-2004	*/

#include	<iostream>
#include	<string.h>
#include	<stddef.h>
#include	<stdlib.h>
#include	"fsa.h"
#include	"nnode.h"
#include	"unode.h"
#include	"nstr.h"
#include	"nindex.h"


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
comp_or_reg(node *n)
{
  node         *new_node;

  if ((new_node = find_or_register(n, PRIM_INDEX, FIND))) {
    // isomorphic graph found
    delete_branch(n);
  }
  else {
    // subgraph is unique, register it
    find_or_register(n, PRIM_INDEX, REGISTER);
    new_node = n;
  }
  new_node->hit_node();
  return new_node;
}//comp_or_reg

/* Name:	mod_child
 * Class:	None.
 * Purpose:	Modify an arc (redirect link, or change final attribute).
 * Parameters:	parent_node	- (i/o) arc source node;
 *		child_node	- (i) arc destination node;
 *		arc_label	- (i) which arc;
 *		final		- (i) whether the arc should be final;
 *		child_already_deleted
 *				- (i) whether to delete child if different
 * Returns:	Parent node.
 * Remarks:	None.
 */
node *
mod_child(node *parent_node, node *child_node, const char arc_label,
	  const int final, const int child_already_deleted)
{
  arc_node *a = parent_node->get_children();
  while (a->letter != arc_label)
    a++;
  if (a->child && a->child != child_node && !child_already_deleted)
    delete_branch(a->child);
  a->child = child_node;
  if (final)
    a->is_final |= TRUE;
  return parent_node;
}//mod_child
  

/* Name:	add_postfix
 * Class:	node
 * Purpose:	Create a chain of nodes with each node holding a consecutive
 *		letter from postfix. Link it to the current node.
 * Parameters:	postfix		- (i) the string to be added.
 * Returns:	Pointer to this node.
 * Remarks:	Nodes are registered as they are created,
 *		and replaced when necessary.
 */
node *
node::add_postfix(const char *postfix)
{
  char	node_char;

  node_char = *postfix++;
  node *n = (*postfix ? new node() : (node *)NULL);
  add_child(node_char, (*postfix ?
			comp_or_reg(n->add_postfix(postfix))
			: (class node *)NULL));
  return this;
}//node::add_postfix

/* Name:	add_child
 * Class:	node
 * Purpose:	Add a child (an arc) to the given node.
 * Parameters:	letter		- (i) arc label;
 *		next_node	- (i) a node that the arc leads to
 *					or NULL for final node.
 * Returns:	Pointer to this node.
 * Remarks:	There may be an arc of this node labelled `letter'.
 *		In that case, use that arc (i.e., attach next node to it).
 *		If not, create a new arc.
 */
node *
node::add_child(char letter, node *next_node)
{
  arc_node	*new_children;
  arc_node	*new_child;
  int		i;

  // find if there is already an arc labelled `letter'
  // if so, put `next_node' at the end of that arc
  for (i = 0; i < no_of_children; i++) {
    if (children[i].letter == letter) {
      // arc found
      if (children[i].child) {
	cerr << "Two arcs from the same node with the same label!\n";
	cerr << "Trying to add an arc labeled `" << letter << "' leading to:"
	     << endl;
	print_node(next_node);
	cerr << "to the following node:" << endl;
	print_node(this);
      }
      children[i].child = next_node;
      return this;
    }
  }

  // a new arc must be created
  new_children = new arc_node[no_of_children + 1];
  for (i = 0; i < no_of_children && letter > children[i].letter; i++)
    new_children[i] = children[i];

  new_child = new_children + i;
  new_child->child = next_node;
  new_child->is_final = (next_node == NULL);
  new_child->letter = letter;
#ifdef WEIGHTED
  new_child->weight = 0;
#endif

  for (; i < no_of_children; i++)
    new_children[i + 1] = children[i];

  if (no_of_children)
    delete [] children;
  children = new_children;
  no_of_children++;
#ifdef MORE_COMPR
  free_end++;
#endif

#ifdef DEBUG
  cerr << "Arc added. Now " << no_of_children << ":";
  for (int qq = 0; qq < no_of_children; qq++) {
    cerr << " " << children[qq].letter;
    if (children[qq].child) {
      cerr << "(";
      for (int qqq = 0; qqq < children[qq].child->no_of_children; qqq++) {
	cerr << " " << children[qq].child->children[qqq].letter;
      }
      cerr << " )";
    }
  }
  cerr << "\n";
#endif

  return this;
}//node::add_child

/***	EOF unode.cc	***/
