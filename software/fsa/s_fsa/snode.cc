/***	snode.cc	***/

/* Copyright (c) Jan Daciuk, 1999-2004	*/

#include	<iostream>
#include	<string.h>
#include	<stddef.h>
#include	<stdlib.h>
#include	"fsa.h"
#include	"nnode.h"
//#include	"snode.h"
#include	"nstr.h"
#include	"nindex.h"


/* Name:	add_postfix
 * Class:	node
 * Purpose:	Create a chain of nodes with each node holding a consecutive
 *		letter from postfix. Link it to the current node.
 * Parameters:	postfix		- (i) the string to be added.
 * Returns:	Pointer to this node.
 * Remarks:	None.
 */
node *
node::add_postfix(const char *postfix)
{
  char	node_char;

  node_char = *postfix++;
  node *n = (*postfix ? new node() : (node *)NULL);
  add_child(node_char, (*postfix ? n->add_postfix(postfix) : (class node *)NULL));
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
  for (i = 0; i < no_of_children; i++)
    new_children[i] = children[i];

  new_child = new_children + no_of_children;
  new_child->child = next_node;
  new_child->is_final = (next_node == NULL);
  new_child->letter = letter;
#ifdef WEIGHTED
  new_child->weight = 0;
#endif

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

/***	EOF snode.c	***/
