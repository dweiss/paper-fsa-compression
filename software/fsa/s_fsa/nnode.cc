/***	nnode.cc	***/

/* Copyright (C) Jan Daciuk, 1996-2004.	*/

/*

This file implements methods of class node.

*/

#include	<iostream>
#include	<string.h>
#include	<stddef.h>
#include	<stdlib.h>
#include	<unistd.h>
#ifdef DMALLOC
#include	"dmalloc.h"
#endif
#include	"fsa.h"
#include	"nnode.h"
#include	"nstr.h"
#include	"nindex.h"
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
#include	"build_fsa.h"
#endif //FLEXIBLE&STOPBIT&SPARSE

const int	INDEX_SIZE_STEP = 16;	// allocate chunks of 16 pointers

int	node::no_of_arcs = 1;	// initially there is an empty arc #0

int	frequency_table[256];	// no of occurences of labels on arcs

using namespace std;

#ifdef FLEXIBLE
#ifdef NEXTBIT
int	node::next_nodes = 0;
#endif //NEXTBIT
#if defined(STOPBIT) && defined(TAILS)
int	node::tails = 0;
arc_pointer curr_dict_address;
#endif //STOPBIT&TAILS
#endif //FLEXIBLE
#ifdef SPARSE
bool	node::in_annotations = false;
#endif //SPARSE
#ifdef STATISTICS

int node::total_nodes = 0;	/* total number of nodes in the automaton */
int node::total_arcs = 0;	/* total number of arcs in the automaton */
int node::in_single_line = 0;	/* number of nodes forming single lines */
int node::line_length = 0;
node *node::current_line = NULL;
int node::outgoing_arcs[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int node::incoming_arcs[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int node::line_of[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int node::sink_count = 0;	// number of incoming arcs for the sink state

#endif

#ifdef WEIGHTED
void flatten_weights(node *n);
#endif //WEIGHTED

#ifdef SPARSE
SparseVect sparse_vector;
unsigned char min_label = 255, max_label = 0;
#endif //SPARSE

/* Name:	set_children
 * Class:	node
 * Purpose:	Set children to given offsprings.
 * Parameters:	kids		- (i) new children;
 *		no_of_kids	- (i) number of (new) children.
 * Returns:	Old children.
 * Remarks:	Old children can be used e.g. with delete [].
 */
arc_node *
node::set_children(arc_node *kids, const int no_of_kids)
{
  arc_node	*old;

  old = children;
  children = kids;
  no_of_children = no_of_kids;
#ifdef MORE_COMPR
  free_end = no_of_kids;
#endif
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(TAILS)
  free_beg = 0; 
#endif
  return old;
}//node::set_children

/* Name:	fertile
 * Class:	node
 * Purpose:	Checks whether the last arc of this node leads to another node.
 * Parameters:	None.
 * Returns:	TRUE if the arcs leads to a node, FALSE otherwise.
 * Remarks:	Node with no arcs is infertile.
 */
int
node::fertile(void) const
{
  if (no_of_children) {
    if (children[no_of_children - 1].child) {
      return TRUE;
    }
  }
  return FALSE;
}//node::fertile

/* Name:	compress_or_register
 * Class:	node
 * Purpose:	Find if there is a subgraph in the automaton that is isomorphic
 *		to the subgraph that begins at the last child of this node.
 * Parameters:	None.
 * Returns:	the node.
 * Remarks:	Because the node can be deleted, and replaced by a pointer
 *		to another node, the method must be invoked with the parent
 *		of the node in question, so that the pointer can be modified.
 *
 *		hit_count counts the number of (registered) links that lead
 *		to a node. hit_count = 0 means the node is not registered yet.
 *		hit_count is also used to avoid deleting nodes that are
 *		pointed to somewhere else in the automaton.
 */
node *
node::compress_or_register(void)
{
  node		*c_node;		// current node
  node		*new_node;

#ifdef DEBUG
  cerr << "Compress_or_register with:\n"; // print();
#endif

  if (no_of_children) {
    c_node = children[no_of_children - 1].child;
    if (c_node == NULL) {
      // Nothing to be registered. One letter has been previously added
      // to this node, which resulted only in creation of another arc,
      // but there is no new node attached to it.
      return this;
    }
    if (c_node->hit_count == 0) {
      // node has not been registered yet
      if (c_node->no_of_children) {
	// compress or register the children (i.e. the line of the youngest)
	// NOTE: the node may have children, but this only means that there
	//       are arcs going from that node; there is no guarrantee
	//       there are nodes at the end of those arcs.
	c_node->compress_or_register();

#ifdef GENERALIZE
	// As new nodes are created as a merger of of other nodes
	// the original order of arcs can no longer be preserved.
	// The following forces the order of character codes treated
	// as small integers
	qsort(c_node->get_children(), c_node->get_no_of_kids(),
	      sizeof(arc_node), cmp_arcs);
#else //!GENERALIZE
#ifdef PRUNE_ARCS
	// The same as above
	qsort(c_node->get_children(), c_node->get_no_of_kids(),
	      sizeof(arc_node), cmp_arcs);
#endif //PRUNE_ARCS
#endif //!GENERALIZE
	// Check if there is already an isomorphic subgraph in the automaton
	if ((new_node = find_or_register(c_node, PRIM_INDEX, REGISTER))) {
	  // Isomorphic subgraph found


	  // Delete c_node
	  delete_branch(c_node);

	  // Link found node to parent
	  children[no_of_children - 1].child = new_node;
	}
	else {
	  // subgraph is unique, registered
	  new_node = c_node;
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
//	  // Find minimal and maximal label
//	  arc_node *cc = c_node->get_children();
//	  int nc = c_node->get_no_of_kids();
//	  for (int i = 0; i < nc; i++,cc++) {
//	    unsigned char lab = (unsigned char)(cc->letter);
//	    if (lab < min_label) {
//	      min_label = lab;
//	    }
//	    if (lab > max_label) {
//	      max_label = lab;
//	    }
//	  }//for i
	  add_labels_to_alphabet(c_node);
#endif // FLEXIBLE&STOPBIT&SPARSE
	}
	new_node->hit_count++;	// count number of links to that node
      }
    }//if node not registered previously
  }//if there is a node to register

  return this;
}//node::compress_or_register

/* Name:	hash
 * Class:	node
 * Purpose:	Computes a hash function for the node to be used in index.
 * Parameters:	start		- (i) first arc number
 *		how_many	- (i) how many arcs are to be considered.
 * Returns:	Hash function.
 * Remarks:	Value of hash function depends on labels of arcs going
 *		from this node.
 */
int
node::hash(const int start, const int how_many) const
{
  long	h = 0;
  int	finish = start + how_many;
  for (int i = start; i < finish; i++)
    h = (((h * 17) ^ children[i].letter) ^ (long(children[i].child) >> 2));
  return (h & MAX_ARCS_PER_NODE);
}//node::hash


/* Name:	number_arcs
 * Class:	node
 * Purpose:	Assign an index to each arc in the automaton.
#ifdef NEXTBIT
 * Parameters:	gtl		- (i) length of the goto field
 *					or 0 meaning count arcs not bytes.
#else
 * Parameters:	None.
#endif
#ifdef WEIGHTED
 *		weighted	- (i) TRUE if weights are used, FALSE if not.
#endif
 * Returns:	Number of the first arc of the node.
 * Remarks:	Arc numbers are kept in nodes, rather than in arcs themselves.
 *		A node keeps an arc number of its first arc.
 *		Arc number equal -1 means the node has not been visited yet.
 *		Arcs of the root are numbered from 1, because arc #0 is
 *		a dummy arc.
 *
 *		The pictures below describe the situation without STOPBIT:
 *
 *		In pictures below, "->-" is a link to big_brother, either
 *		a true big_brother (i.e. one that is bigger), or the first
 *		node of merged nodes. "-=-" indicates a link to a node
 *		that this node was merged with. Index n indicates already
 *		numbered node. Xo means brother_offset of X. a means
 *		current arc number to be allocated.
 *
 *		Situations and actions (this (A) is always not numbered):
 *
 *		1) A			- number A and children of A
 *					  Note: A may be a big brother
 *					  of another node. It does not matter.
 *					  A = a.
 *
 *		2a) A ->-> B		- number B, number A, and number
 *					  children of B.
 *					  B = a; A = B + Ao.
 *
 *		2b) A ->-> Bn		- number A.
 *					  A = B + Ao.
 *
 *		3a) A ->-> B		- number B, number A, number children
 *		      <-=-		  of B, number children[1] of A.
 *					  B = a; A = B + Ao;
 *
 *		3b) A -=-> B		- number A, number B, number children
 *		      <-<-		  of A, number children[1] of B
 *					  Note: in 3, A & B are always numbered
 *					  at the same time.
 *					  A = Ao; B = A + Bo.
 *
 *		4a) A ->-> B ->-> C	- number C, number B, number A,
 *			     <-=-	  number children of C,
 *					  number children[1] of B.
 *					  C = a; B = C + Bo; A = B + Ao.
 *
 *		4b) A ->-> B -=-> C	- number B, number C, number A,
 *			     <-<-	  number children of B,
 *					  number children[1] of C.
 *					  B = a; C = B + Co; A = B + Ao;
 *
 *		4c) A ->-> Bn ->-> Cn	- number A.
 *			      <-=-	  A = B + Ao.
 *
 *		4d) A ->-> Bn -=-> Cn	- number A.
 *			      <-<-	  A = B + Ao.
 *
 *		With STOPBIT, the situation is quite less complicated,
 *		as JOIN_PAIRS cannot be on.
 *
 *		When using NEXTBIT and !STOPBIT, no arc sharing can be used,
 *		or rather only such arc sharing that results in sharing tails
 *		of nodes (not implemented yet for !STOPBIT).
 *
 *		With TAILS on, tails of nodes (their last arcs) can be shared.
 *		So with the TAILS option on, when a node has a big brother,
 *		there might be the following situations:
 *		t1) The big brother has already been numbered
 *		  a) This node is entirely in big brother, free_beg = 0
 *			Set arc_no to big_brother + offset, no need
 *			to number children
 *		  b) This node shares a tail with big brother, free_beg != 0
 *			Set arc_no to no_of_arcs, add free_beg arcs and
 *			and gtl to no_of_arcs
 *		t2) The big brother is to be numbered
 */
int
#if defined(FLEXIBLE) && ((defined(STOPBIT) && defined(TAILS)) || defined(NEXTBIT))
#ifdef WEIGHTED
node::number_arcs(const int gtl, const int weighted)
#else //!WEIGHTED
node::number_arcs(const int gtl)
#endif //!WEIGHTED
#else //!WEIGHTED
#ifdef WEIGHTED
node::number_arcs(const int weighted)
#else //!WEIGHTED
node::number_arcs(void)
#endif //!WEIGHTED
#endif
{
  node		*p, *pp;
  int		limit;
  // Set the offset for the goto field, i.e. how far the pointer is
  // from the beginning of the arc/transition
#if defined(FLEXIBLE) && defined(NEXTBIT)
#ifdef STOPBIT
#ifdef WEIGHTED
  int		goto_offset = 1 + weighted;
#else //!WEIGHTED
  const int	goto_offset = 1;
#endif //!WEIGHTED
#else //!STOPBIT
#endif //!STOPBIT
#endif //!(FLEXIBLE,NEXTBIT)
// no conditionals
#ifndef LARGE_DICTIONARIES
#ifndef FLEXIBLE
  static int	not_yet_warned = TRUE;
#endif
#endif


  // Debugging info
  // if (free_beg) cerr << "Node with free_beg=" << free_beg << endl;
// no conditionals
#ifdef DEBUG
  cerr << "Numbering arcs of node " << hex << (long)this << " with ";
  for (int i = 0; i < no_of_children; i++)
    cerr << children[i].letter;
  cerr << "\n";
#endif

  // statistics
#ifdef STATISTICS
  total_nodes++;
  total_arcs += no_of_children;
  if (no_of_children < 10)
    (outgoing_arcs[no_of_children])++;
  else
    (outgoing_arcs[0])++;
  if (hit_count < 10)
    (incoming_arcs[hit_count])++;
  else if (hit_count < 100)
    (incoming_arcs[10])++;
  else if (hit_count < 1000)
    (incoming_arcs[11])++;
  else
    (incoming_arcs[0])++;
  if (no_of_children == 1 && hit_count == 1) {
    in_single_line++;
    if (current_line == this) {
      if (line_length > 0 && line_length < 10)
	--(line_of[line_length]);
      line_length++;
      if (line_length < 10)
	(line_of[line_length])++;
      else if (line_length == 10)
	(line_of[0])++;
    }
    else {
      line_length = 1;
      (line_of[line_length])++;
    }
  }
  else
    line_length = 0;
  current_line = children->child;
#endif
// no conditionals

#ifdef SPARSE
  if (in_annotations) {
#endif
// Try to rearrange arcs for better compression
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(NEXTBIT) && defined(MORE_COMPR)
  if (gtl == 0 && no_of_children > 1) {
    static node  *descendants[MAX_ARCS_PER_NODE];
    // try to rearrange arcs so that one that leads to nodes with more
    // children will be the last one
    int found = 0;
    if (free_end > 0 && free_end < MAX_ARCS_PER_NODE) {
      for (int kk = no_of_children - free_end; kk < no_of_children; kk++)
        descendants[kk] = children[kk].child;
      int maxi = -1;
      int maxv = 1;
      while (!found) {
        int inactive = 0;
        for (int ll = no_of_children - 1; ll >= no_of_children - free_end;
             --ll) {
          if (descendants[ll] == NULL || descendants[ll]->arc_no != -1)
            inactive++;
          else if (descendants[ll]->no_of_children > maxv) {
            maxv = descendants[ll]->no_of_children;
            maxi = ll;
            found = TRUE;
          }
          else
            descendants[ll] = descendants[ll]->get_children()->child;
        }//for
        if (inactive >= free_end)
          break;
      }//while
      if (found) {
        if (maxi != no_of_children - 1) {
          // exchange two arcs
          arc_node tmp_arc = children[no_of_children - 1];
          children[no_of_children - 1] = children[maxi];
          children[maxi] = tmp_arc;
        }
      }
    }
  }
#endif //FLEXIBLE & STOPBIT & NEXTBIT & MORE_COMPR
#ifdef SPARSE
  }
#endif //SPARSE

  // Number arcs - or actually, the first arc in a node
  if (no_of_children) {
    p = this;
    limit = no_of_children;

#ifdef SPARSE
    if (in_annotations) {
#endif
    if (big_brother) {
      if (big_brother->arc_no != -1) {
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(TAILS)
	if (free_beg) {
	  // Situation t1b: tail shared with big brother
	  // The node will occupy only free_beg arcs + gtl bytes
	  // for a pointer to the tail in another node
	  limit = free_beg;
	}
	else {
#endif //FLEXIBLE,STOPBIT,TAILS
	  // big brother has been numbered
	  // Situations 2b, 4c, 4d.
	  arc_no =
#if defined(FLEXIBLE) && (defined(NEXTBIT) || (defined(STOPBIT) && defined(TAILS)))
	    (goto_offset + gtl) *
#endif //FLEXIBLE,(NEXTBIT|(STOPBIT,TAILS))
	    brother_offset + big_brother->arc_no;
	  return arc_no;
	  // Note: no need to number children (already numbered)
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(TAILS)
	}
#endif //FLEXIBLE,STOPBIT,TAILS
      }
      else {
	// Big brother not numbered yet

#ifndef STOPBIT
#ifdef JOIN_PAIRS
	limit = big_brother->no_of_children;	// change when chains of arcs
	// Situations 2a, 3a, 3b, 4a, 4b
	if (big_brother->big_brother && big_brother->big_brother != this) {
	  // Situations: 4a, 4b.
	  if (big_brother->brother_offset < 0)
	    // Situation 4b.
	    p = big_brother;
	  else
	    // Situation 4a.
	    p = big_brother->big_brother;
	  p->big_brother->arc_no = no_of_arcs + p->big_brother->brother_offset;
	  p->arc_no = no_of_arcs;	// we use it in the following line...
	  arc_no = big_brother->arc_no + brother_offset;
	  limit -= p->brother_offset;
	  // Handled: 4a: Bn, An; 4b: Cn, An.
	  // Wait for: 4a: Cn, Cc, B[1]c; 4b: Bn, Bc, C[1]c.
	}
	else if (big_brother->big_brother && big_brother->big_brother == this){
	  // Situations 3a, 3b.
	  if (big_brother->brother_offset < 0)
	    // Situation 3b.
	    p = big_brother;
	  else
	    // Situation 3a.
	    p = this;
	  p->big_brother->arc_no = no_of_arcs + p->big_brother->brother_offset;
	  limit -= p->brother_offset;
	  // Handled: 3a: An; 3b: Bn.
	  // Wait for: 3a: Bn, Bc, A[1]c; 3b: An, Ac, B[1]c.
	}
	else {
#endif //JOIN_PAIRS
#endif //!STOPBIT
#if defined (FLEXIBLE) && defined(STOPBIT) && defined(TAILS)
	  if (free_beg) {
	    limit = free_beg;
	  }
	  else {
#endif //FLEXIBLE,STOPBIT,TAILS
	  // Situation 2a
	  p = big_brother;
	  arc_no =
#if defined(FLEXIBLE) && (defined(NEXTBIT) || (defined(STOPBIT) && defined(TAILS)))
	    (gtl ? (goto_offset + gtl) : 1) *
#endif //FLEXIBLE,(NEXTBIT|(STOPBIT,TAILS))
	    brother_offset + no_of_arcs;
	  limit = p->no_of_children;
	  // handled: An.
	  // Wait for: Bn, Bc.
#if defined (FLEXIBLE) && defined(STOPBIT) && defined(TAILS)
	  }
#endif
#ifndef STOPBIT
#ifdef JOIN_PAIRS
	}
#endif //!STOPBIT
#endif //JOIN_PAIRS
      }//if the big brother is not numbered yet
// no conditionals
    }//if node has big brother

    p->arc_no = no_of_arcs;
    // All Xn handled.
#ifndef	LARGE_DICTIONARIES
#ifndef FLEXIBLE
    // if two bytes is not enough to hold the number of arcs
    if ((unsigned)no_of_arcs > (unsigned)(((1 << 16) - 1) - limit) &&
	no_of_arcs > limit &&        // I cannot remember what it is for
	not_yet_warned) {
      cerr << "Too many arcs in automaton.\n"
	   << "Compile programs with flag LARGE_DICTIONARIES\n"
	     << "or use optimization (if you do not use it already)\n";
      not_yet_warned = FALSE;
    }
#endif //!LARGE_DICTIONARIES
#endif //!FLEXIBLE
// no conditionals
#ifdef FLEXIBLE
#if defined(NEXTBIT) && defined(MORE_COMPR)
    if (gtl == 0 && (no_of_children &&
		     !nis_next_node(
#if defined(STOPBIT) && defined(TAILS)
				    free_beg ?
				    children[free_beg - 1].child :
#endif // STOPBIT&&TAILS
				    p->children[p->no_of_children -1].child))){
      // Try to rearrange arcs in p so that the last arc would point
      // to the next node
      int a, b;
      b = 
#if defined(STOPBIT) && defined(TAILS)
	free_beg ? free_beg - 1 :
#endif // STOPBIT&&TAILS
	p->no_of_children - 1;
      for (a = b - 1;
	   a >=
#if defined(STOPBIT) && defined(TAILS)
	     (free_beg ? 0 :
#endif // STOPBIT&&TAILS
	     p->no_of_children - p->free_end
#if defined(STOPBIT) && defined(TAILS)
	      )
#endif // STOPBIT&&TAILS
	      ; --a) {
	if (nis_next_node(p->children[a].child)) {
	  // Exchange arcs
	  arc_node temp_arc = p->children[a];
	  p->children[a] = p->children[b];
	  p->children[b] = temp_arc;
	  break;
	}
      }
    }
#endif // MORE_COMPR&NEXTBIT

    // update the arc counter, so that new arcs receive proper numbers
#if defined(TAILS) && defined (STOPBIT) || defined(NEXTBIT)
    if (gtl) {
      // We recalculate the numbers taking variable arc length into account
      no_of_arcs += limit * (goto_offset + gtl);
#if defined(STOPBIT) && defined(TAILS)
      if (free_beg)
	no_of_arcs += gtl;
#endif
#ifdef NUMBERS
      if (entryl)
	no_of_arcs += entryl;
#endif // NUMBERS
#ifdef NEXTBIT
      if (gtl > 1 && no_of_children &&
	  nis_next_node(p->children[
#if defined (STOPBIT) && defined(TAILS)
				    free_beg ? free_beg - 1 :
#endif //STOPBIT&&TAILS
				    p->no_of_children - 1].child)) {
	no_of_arcs -= (gtl - 1);
      }
#endif //NEXTBIT
    }
    else {
      no_of_arcs += limit;	// just counting arcs
#if defined(STOPBIT) && defined(TAILS)
      if (free_beg)
	tails++;
#endif
    }
#else //!NEXTBIT || !(STOPBIT&&TAILS)
#ifdef NUMBERS
    if (entryl)
      no_of_arcs += limit * a_size + entryl;
    else
      no_of_arcs += limit;
#else //!NUMBERS
    no_of_arcs += limit;
#endif //NUMBERS
#endif //NEXTBIT
#else //!FLEXIBLE
    no_of_arcs += limit;
#endif //!FLEXIBLE
#ifdef SPARSE
    }
    else {
      // We are in the part in front of annotations
      // and this part is to be represented as a sparse matrix
      // The sparse vector is written to the file elsewhere
      arc_no = sparse_vector.fits_in(this);
    }//if(!in_annotations)
#endif //SPARSE

// no conditionals

    // Now start numbering the children
#if defined(FLEXIBLE) && defined(NEXTBIT)
#ifdef SPARSE
    bool current_in_annots = in_annotations;
#endif //SPARSE
    // We start from the last arc to have more compression
    for (int j = limit - 1; j >= 0; --j) {
      pp = p->children[j].child;
#ifdef SPARSE
      in_annotations = current_in_annots ||
	(p->children[j].letter == ANNOT_SEPARATOR);
#endif //SPARSE
      if (pp && pp->arc_no == -1) {
#ifdef WEIGHTED
	pp->number_arcs(gtl, weighted);
#else //!WEIGHTED
	pp->number_arcs(gtl);
#endif //!WEIGHTED
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
	if (current_in_annots) {
#endif
#if defined(FLEXIBLE) && defined(NEXTBIT)
	  if (no_of_children && j == limit - 1 &&
	      nis_next_node(p->children[
#if defined (STOPBIT) && defined(TAILS)
					free_beg ? free_beg - 1 :
#endif //STOPBIT&&TAILS
					p->no_of_children - 1].child)) {
	    next_nodes++;
	  }
#endif
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
	}
#endif
      }
#ifdef STATISTICS
      if (pp == NULL) {
	sink_count++;
      }
#endif // STATISTICS
    }
#ifdef SPARSE
    in_annotations = current_in_annots;
#endif //SPARSE
#else // !(NEXTBIT&&FLEXIBLE)
    for (int i = 0; i < limit; i++) {
#ifndef STOPBIT
#ifdef JOIN_PAIRS
      if (i >= p->no_of_children)
	pp = p->big_brother->children[1].child;
      else
#endif //JOIN_PAIRS
#endif //!STOPBIT
      pp = p->children[i].child;
      if (pp && pp->arc_no == -1) {
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(TAILS)
	pp->number_arcs(gtl);
#else
	pp->number_arcs();
#endif
      }
#ifdef STATISTICS
      if (pp == NULL) {
	sink_count++;
      }
#endif //STATISTICS
    }
#endif // !NEXTBIT
  }
  else
    arc_no = 0;
  return arc_no;
}//node::number_arcs


#ifdef	SORT_ON_FREQ
/* Name:	count_arcs
 * Class:	node
 * Purpose:	Counts arcs having the same label.
 * Parameters:	freq_table	- number of occurences of arcs with
 *				  different labels.
 * Returns:	Nothing.
 * Remarks:	Nodes counted receive arc_no = -2.
 *		This will be changed by sort_arcs.
 *
 *		As a result, freq_table contains the number of occurences
 *		of arcs with a specific label in the cell indexed by that
 *		label code. The code is treated as unsigned.
 */
void
node::count_arcs(int *freq_table)
{
  node		*p;
  unsigned char	c;

  if (no_of_children) {
    arc_no = -2;
    for (int i = 0; i < no_of_children; i++) {
      c = unsigned(children[i].letter);
      freq_table[c]++;
      p = children[i].child;
      if (p && p->arc_no != -2)
	p->count_arcs(freq_table);
    }
  }
}//node::count_arcs


/* Name:	sort_arcs
 * Class:	node
 * Purpose:	Sorts arcs by frequency.
 * Parameters:	None.
 * Result:	Nothing.
 * Remarks:	Frequency is the frequency of occurence of an label
 *		in the automaton.
 *		Nodes already sorted receive arc_no = -1.
 *		before sorting, they should have arc_no = -2.
 */
void
node::sort_arcs(void)
{
  node	*p;

  if (no_of_children) {
    arc_no = -1;
    qsort(children, no_of_children, sizeof(arc_node), freq_cmp);
    for (int i = 0; i < no_of_children; i++) {
      p = children[i].child;
      if (p && p->arc_no != -1)
	p->sort_arcs();
    }
  }
}//node::sort_arcs


/* Name:	freq_cmp
 * Class:	None.
 * Purpose:	Compares arcs on frequency.
 * Parameters:	el1		- (i) the first arc;
 *		el2		- (i) the second arc;
 * Result:	1	- if the label on the first arc appears less frequently
 *				on arcs in the automaton than the label on the
 *				second arc;
 *		0	- if the label on the first arc appears the same number
 *				of times in the automaton as the label on the
 *				second arc;
 *		-1	- if the label on the first arc appears more frequently
 *				on arcs in the automaton than the label on the
 *				second arc.
 * Remarks:	The sorting based on this function is in reverse order.
 *		This should give better speed for word retrieval.
 */
int
freq_cmp(const void *el1, const void *el2)
{
  unsigned char	c1, c2;

  c1 = unsigned(((arc_node *)(el1))->letter);
  c2 = unsigned(((arc_node *)(el2))->letter);
#ifdef DESCENDING
  return frequency_table[c1] - frequency_table[c2];
#else
  return frequency_table[c2] - frequency_table[c1];
#endif
}//freq_cmp
#endif


/* Name:	write_arcs
 * Class:	node
 * Purpose:	Writes arcs of a node and arcs of its descendants to a file.
 * Parameters:	outfile		- (o) output file.
 * Returns:	TRUE if arcs written, FALSE otherwise.
 * Remarks:	It is assumed that arcs were numbered with node::number_arcs
 *		with root being #1. #0 is a dummy arc.
 */
int
#ifdef WEIGHTED
node::write_arcs(ostream &outfile, const int weighted)
#else //!WEIGHTED
node::write_arcs(ostream &outfile)
#endif //!WEIGHTED
{
#ifdef FLEXIBLE
  char		output_arc[10];
  char		*oa = &output_arc[0];
  fsa_arc_ptr	*dummy;
#ifdef STOPBIT
#else //!STOPBIT
#ifdef WEIGHTED
  int		goto_offset = 2 + weighted;
#else //!WEIGHTED
  const int	goto_offset = 2;
#endif //!WEIGHTED
#endif //!STOPBIT
#else //!FLEXIBLE
  fsa_arc	output_arc[1];
  fsa_arc	*oa = &output_arc[0];
#endif //!FLEXIBLE
// no conditionals
  int		i;
  node		*p;
  arc_node	pp;
  int		limit = no_of_children;

#ifdef DEBUG
  cerr << "write_arcs with hit_count = " << hit_count
    << ", arc_no = " << arc_no << ", children = " << no_of_children <<"\n";
  cerr << "node address is " << hex << long(this) << endl;
#endif

  // Establish the shape of the node and its arcs
#ifdef FLEXIBLE
  int gtl = dummy->gtl;
  int size = gtl + goto_offset;
#endif

// no conditionals
  if (hit_count > 0) {
    hit_count = 0;		// mark the node as visited (written)
    if (no_of_children > 0) {
#ifdef SPARSE
      if (in_annotations) {
#endif //SPARSE
#ifdef WEIGHTED
      if (weighted)
	flatten_weights(this);
#endif //WEIGHTED
#if defined(FLEXIBLE) && defined(NUMBERS)
      if (entryl) {
	// Write the number of strings in the subautomaton
	// starting in this node
	int r = entries;
	for (int ix = 0; ix < dummy->entryl; ix++) {
	  oa[ix] = r & 0xff;
	  r >>= 8;
	}
#ifdef SPARSE
	if (in_annotations) {
	  memcpy(annot_buffer + ab_ptr, oa, dummy->entryl);
	  ab_ptr += dummy->entryl;
	}
#else //!SPARSE
	if (!(outfile.write((char *)oa, dummy->entryl)))
	  return FALSE;
#endif //!SPARSE
      }
#endif // FLEXIBLE,NUMBERS
#ifdef JOIN_PAIRS
      if (big_brother &&
	  brother_offset >= 0) {
	// This means that the order in which the arcs are visited is wrong.
	// While considering children, we first check whether they have
	// big brothers, and if it is the case, we write
	// the bog brithers instead
	cerr << "While writing arcs, a node with bb encountered\n";
	cerr << "The node is:\n";
	print_node(this);
	cerr << "And its big brother is:\n";
	print_node(big_brother);
	exit(22);
      }
#endif //JOINPAIRS
#ifndef STOPBIT
#ifdef JOIN_PAIRS
      // This is for the case when we put a 2-arc node into two adjacent
      // 2-arc nodes so that one arc is in one node,
      // the other arc in the other node
      if (big_brother && no_of_children == 2 &&
	  brother_offset < 0)
	limit = no_of_children - brother_offset;
#endif
#endif
// no conditionals

#if defined(STOPBIT) && defined(TAILS)
      // If beg_free is positive, only some first arcs of this node
      // are written here.
      // The rest (i.e. the tail) is written as a tail of some other node.
      if (free_beg)
	limit = free_beg;
#endif

      // For all arcs of this node or its big brother (for JOIN_PAIRS)
      // write the arcs
      for (i = 0; i < limit; i++) {
#ifndef STOPBIT
#ifdef JOIN_PAIRS
	if (i >= no_of_children)
	  pp = big_brother->children[1];
	else
#endif
#endif
// no conditionals
	pp = children[i]; 
	p = pp.child;

	// set the label
#ifdef FLEXIBLE
#ifdef STOPBIT
	*oa = pp.letter;
#else
	oa[1] = pp.letter;
#endif
#else
	oa->letter = pp.letter;
#endif //FLEXIBLE
// no conditionals

	// Set goto field
#ifdef FLEXIBLE
	if (p) {
	  int r = p->arc_no;
#ifdef STOPBIT
#ifdef NEXTBIT
#ifdef TAILS
	  r <<= 4;
#else //!TAILS
	  r <<= 3;
#endif //!TAILS
#else // !NEXTBIT
#ifdef TAILS
	  r <<= 3;
#else //!TAILS
	  r <<= 2;
#endif //!TAILS
#endif // !NEXTBIT
#else // !STOPBIT
#ifdef NEXTBIT
	  r <<= 1;
#endif //NEXTBIT
#endif // !STOPBIT
	  for (int i2 = 0; i2 < gtl;  i2++) {
	    oa[goto_offset + i2] = r & 0xff;
	    r >>= 8;
	  }
	}//if p
	else {
	  // The address is NULL, so put all zeros
	  for (int i3 = 0; i3 < gtl; i3++) {
	    oa[goto_offset + i3] = 0;
	  }
	}//if !p
#ifdef NEXTBIT
	// 
	fsa_set_next(oa, i == limit - 1 && wis_next_node(p) ? 1 : 0);
	if (wis_next_node(p) && i == limit - 1)
	  oa[goto_offset] |= 0x80; // make sure goto != 0
#endif // NEXTBIT
#if defined(STOPBIT) && defined(TAILS)
	fsa_set_tail(oa, free_beg && i == limit - 1);
#endif //STOPBIT,TAILS
#endif // FLEXIBLE

// no conditionals

	if (p) {
#ifndef FLEXIBLE
	  oa->go_to = p->arc_no;
#endif //FLEXIBLE

	  // Set the number of arcs or the last arc marker
#if defined(FLEXIBLE) && defined(STOPBIT)
	  fsa_set_last(oa, i == limit - 1);
#else //!(FLEXIBLE,STOPBIT)
	  fsa_set_children(oa, p->no_of_children);
#endif //!(FLEXIBLE,STOPBIT)
// no conditionals
// This is exactly the opposite!!!!!
#if MAX_ARCS_PER_NODE==255
	  if (p->no_of_children > MAX_ARCS_PER_NODE) {
	    cerr << "Too many arcs leading from one node\nYour language "
	         << "contains more than " << MAX_ARCS_PER_NODE
		 << " characters!\n";
	    exit(99);
	  }
#endif
	  // And the final bit
	  fsa_set_final(oa, pp.is_final);
	}
	else {//if !p

	  // Set goto, number of arcs, and final fields
#ifndef FLEXIBLE
	  oa->go_to = 0;
#endif //FLEXIBLE
#ifdef STOPBIT
	  fsa_set_last(oa, i == limit - 1);
#else //!STOPBIT
	  fsa_set_children(oa, 0);
#endif //!STOPBIT
	  fsa_set_final(oa, TRUE);
	}//if !p
#ifdef WEIGHTED
	if (weighted) {
	  oa[goto_offset - 1] = (pp.weight & 0xff);
	}
#endif

// no conditionals

	// Write the result (the arc)
#ifdef FLEXIBLE
#ifdef NEXTBIT
	int bytes_to_write = (p && i == limit - 1 && wis_next_node(p)) ?
	  goto_offset + 1 : size;
#else //!NEXTBIT
	int bytes_to_write = size;
#endif // !NEXTBIT
#ifdef SPARSE
	memcpy(annot_buffer + ab_ptr, oa, bytes_to_write);
	ab_ptr += bytes_to_write;
#else //!SPARSE
	if (!(outfile.write(oa, bytes_to_write)))
	  return FALSE;
#endif
#else //!FLEXIBLE
	if (!(outfile.write((char *)&output_arc[0], sizeof output_arc[0])))
	  return FALSE;
#endif // !FLEXIBLE
      }//for i

// no conditionals

#if defined(FLEXIBLE) && defined(STOPBIT) && defined(TAILS)
      // If the tails of this node is written somewhere else,
      // then we need to write a pointer to it here
      if (free_beg) {
	int r1 = big_brother->arc_no + (brother_offset * (goto_offset + gtl));
#ifdef NEXTBIT
	r1 <<= 4;
#else //!NEXTBIT
	r1 <<= 3;
#endif // !NEXTBIT
	for (int i5 = 0; i5 < gtl; i5++) {
	  oa[i5] = r1 & 0xff;
	  r1 >>= 8;
	}
	// So we have the address part in oa, shifted appropriately.
	// The various bits are not set, and this is OK.
	// Now we write the arc
	if (!(outfile.write((char *)&output_arc[0], gtl)))
	  return FALSE;
      }//if free_beg
#endif //FLEXIBLE & STOPBIT & TAILS
#ifdef SPARSE
      }
#endif

// no conditionals

      // For all arcs of this node or its brother
      // write arcs of descendants
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
      bool current_in_annots = in_annotations;
#ifdef NUMBERS
      if (no_of_children && !in_annotations && entryl) {
	for (int j = 0; j < limit; j++) {
	  unsigned long hv = children[j].is_final +
	    (children[j].child ? children[j].child->entries : 0);
	  for (int k = 0; k < limit; k++) {
	    if (sparse_vector.get_character_number(children[j].letter) <
		sparse_vector.get_character_number(children[k].letter)) {
	      unsigned long chv =
		sparse_vector(arc_no,children[k].letter).get_hash_v();
	      sparse_vector(arc_no,children[k].letter).set_hash_v(chv + hv);
	    }
	  }
	}
// 	unsigned long curr_h = 0;
// 	for (int j = 0; j < limit; j++) {
// 	  sparse_vector(arc_no,children[j].letter).set_hash_v(curr_h);
// 	  curr_h += (children[j].child ? children[j].child->entries : 0);
// 	  curr_h += children[j].is_final;
// 	}//for j
      }
#endif //NUMBERS
#endif //FLEXIBLE&STOPBIT&SPARSE
#if defined(FLEXIBLE) && defined(NEXTBIT)
      for (i = limit - 1; i >= 0; --i) {
#else
      for (i = 0; i < limit; i++) {
#endif
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
	if (!current_in_annots) {
	  in_annotations = current_in_annots ||
	    (children[i].letter == ANNOT_SEPARATOR);
	  sparse_vector(arc_no,
		       children[i].letter).set_target(children[i].child ?
						      children[i].child->arc_no
						      : 0L);
	}
#endif //FLEXIBLE&STOPBIT&SPARSE
#ifndef STOPBIT
#ifdef JOIN_PAIRS
	if (i >= no_of_children)
	  p = big_brother->children[1].child;
	else
#endif
#endif
	p = children[i].child;
	if (p) {
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(TAILS)
	  if (p->free_beg == 0) {
#endif
	  if (p->big_brother
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
	      && in_annotations
#endif
#ifdef JOIN_PAIRS
	      && p->brother_offset >= 0
#endif
	      )
	    p = p->big_brother;
	  // This must be done twice, as a 1-arc node may refer to 2-arc node
	  // merged with another 2-arc node
	  if (p->big_brother
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
	      && in_annotations
#endif
#ifdef JOIN_PAIRS
	      && p->brother_offset >= 0
#endif
	      )
	    p = p->big_brother;
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(TAILS)
	  }//if p->free_beg == 0
#endif
#ifdef DEBUG
	  cerr << "Writing " << i << "th child of <" << arc_no << ">\n";
#endif
#ifdef WEIGHTED
	  if (!(p->write_arcs(outfile, weighted)))
#else //!WEIGHTED
	  if (!(p->write_arcs(outfile)))
#endif
	    return FALSE;
	}//if p
      }//for i
    }//if no_of_children > 0
  }//if hit_count > 0
  return TRUE;
}//node::write_arcs

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
delete_branch(node *branch)
{
  arc_node	*an;
  if (branch->hit_node(0) == -1)
    // branch is to be deleted higher up
    return;

  if (branch->hit_node(0)) {
    // the branch is still in use
    branch->hit_node(-1);
  }
  else {
    an = branch->get_children();
    branch->hit_node(-1 - branch->hit_node(0));	// to avoid loops
    for (int i = 0; i < branch->get_no_of_kids(); i++, an++) {
      if (an->child)
	delete_branch(an->child);
    }
    delete branch;
  }
}//delete_branch


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
 *
 *		Changed: Now the only difference to delete_branch is
 *		that the deleted node is also unregistered.
 */
int
delete_old_branch(node *branch)
{
  arc_node	*an;
  if (branch->hit_node(0) == -1)
    // branch is to be deleted higher up
    return FALSE;

  if (branch->hit_node(0) > 1) { // was: //was: > 1
    // the branch is still in use
    branch->hit_node(-1);
    return FALSE;
  }
  else {
    unregister_node(branch);
    an = branch->get_children();
    branch->hit_node(-1 - branch->hit_node(0));	// to avoid loops
    for (int i = 0; i < branch->get_no_of_kids(); i++, an++) {
      if (an->child)
	delete_old_branch(an->child);
    }
    delete branch;
    return TRUE;
  }
}//delete_old_branch

/* Name:	set_link
 * Class:	node
 * Purpose:	Establish link between this node, and a node having more links
 *		that include also all links from this node.
 * Parameters:	big_guy		- (i) bigger node;
 *		offset		- (i) offset to the set of arcs of this node
 *					found in big_guy.
 * Returns:	Pointer to this node.
 * Remarks:	None.
 */
node *
node::set_link(node *big_guy, const int offset)
{
#ifdef DEBUG
  cerr << "Node " << (long)this << " with ";
  for (int i = 0; i < no_of_children; i++)
    cerr << children[i].letter;
  cerr << "\n now points to " << (long)big_guy << "+" << offset << " with ";
  for (int j = 0; j < big_guy->no_of_children; j++)
    cerr << big_guy->children[j].letter;
  cerr << "\n";
#endif
  big_brother = big_guy;
  brother_offset = offset;
#ifdef MORE_COMPR
  // The commented out part should never be executed anyway,
  // as we cannot link a node to another node that already has a link
//  if (big_guy->free_beg > offset) // CHECK THIS PLEASE!!!!!
//    big_guy->free_beg = offset;
#ifdef STOPBIT
  if (big_guy->free_end > big_guy->no_of_children - offset)
    big_guy->free_end = big_guy->no_of_children - offset;
#else
  if (big_guy->free_end > big_guy->no_of_children - (offset + no_of_children))
    big_guy->free_end = big_guy->no_of_children - (offset + no_of_children);
#endif
#endif
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(TAILS)
  big_guy->brother_offset = (unsigned int)-1;
#endif
  return this;
}//node::set_link

#ifdef FLEXIBLE
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
number_entries(node *n)
{
  arc_node	*p;
  int		e;

  if (n->get_entries() == -1) {
    e = 0;
    p = n->get_children();
    for (int i = 0; i < n->get_no_of_kids(); i++, p++) {
      e += (p->is_final ? 1 : 0) + (p->child ? number_entries(p->child) : 0);
    }
    n->inc_arc_count(n->get_no_of_kids());
    n->inc_node_count();
    n->set_entries(e);
  }
  return n->get_entries();
}/*number_entries*/
#endif //NUMBERS
#endif //FLEXIBLE

/* Name:	print_node
 * Class:	None.
 * Purpose:	Prints node details for debugging purposes.
 * Parameters:	n		- (i) the node to be examined.
 * Returns:	Nothing.
 * Remarks:	Printing is done on cerr.
 */
void
print_node(const node *n)
{
  arc_node	*an;
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(TAILS)
  int		t;
#endif

  cerr << "Node: " << hex << (long)n << " hc=" << dec
       << ((node *)n)->hit_node(0)
       << " arc_no=" << n->get_arc_no() << " kids=" << n->get_no_of_kids();
  if (n->get_big_brother())
    cerr << " bb=" << hex << (long)(n->get_big_brother()) << dec;
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(TAILS)
#ifdef MORE_COMPR
  t = n->free_end;
  cerr << " free_e=" << dec << t;
#endif
  t = n->free_beg;
  cerr << " free_b=" << dec << t;
#endif
  cerr << endl;
  an = n->get_children();
  for (int i = 0; i < n->get_no_of_kids(); i++, an++) {
    cerr << " +- " << an->letter << (an->is_final ? "!" : " ")
	 << hex << (long)(an->child) << dec << endl;
  }
}


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
int
cmp_arcs(const void *a1, const void *a2)
{
  return (((arc_node *)a1)->letter - ((arc_node *)a2)->letter);
}//cmp_arcs



/* Name:	mark_inner
 * Class:	None.
 * Purpose:	Mark nodes with a specified value of arc_no.
 * Parameters:	n	- (i) node to be marked;
 *		no	- (i) arc_no for that node.
 * Returns:	n - the node to be marked.
 * Remarks:	It is assumed that node with arc_no = no are already marked.
 */
node *
mark_inner(node *n, const int no)
{
  if (n->get_arc_no() == no)
    return n;

  n->set_arc_no(no);
  arc_node *an = n->get_children();
  for (int i = 0; i < n->get_no_of_kids(); i++, an++)
    if (an->child)
      mark_inner(an->child, no);
  return n;
}

#ifdef SUBAUT
/* Name:	create_back_arcs
 * Class:	None.
 * Purpose:	Fill the vectors pointing to nodes having arcs coming
 *		to the present node. Do that for all descendant as well.
 * Parameters:	n	- (i/o) the node to visit;
 *		layers	- (o) vector of nodes in increasing height;
 *		nlayers	- (i/o) number of nodes in the vector.
 * Returns:	Nothing.
 * Remarks:	Filling is done from the other end, i.e. not from the target
 *		side.
 *		The vector `layers' is filled with nodes that have only
 *		arcs that point to the last node, i.e. a node with
 *		no descendants.
 */
void
create_back_arcs(node *n, node *layers[], int *nlayers)
{
  int all_null;
  if (n->back_arcs)
    return;			// node already visited
  n->back_arcs = new node *[n->hit_node(0)]; // create vector
  all_null = TRUE;
  // create vectors and fill descendants of children
  for (int i = 0; i <= n->get_no_of_kids(); i++) {
    int p;
    if ((p = n->get_children()[i].child) != NULL) {
      create_back_arcs(p);
      all_null = FALSE;
    }
  }
  if (all_null) {
    layers[(*nlayers)++] = n;
  }
  // fill the children's vectors on the way back
  for (int j = 0; j <= n->get_no_of_kids(); j++) {
    int q;
    if ((q = n->get_children()[j].child) != NULL)
      if (q->back_filled == 0 || q->back_arcs[q->back_filled] != n)
	q->back_arcs[q->back_filled++] = n;
  }
}/*create_back_arcs*/
#endif

#ifdef WEIGHTED
/* Name:	flatten_weights
 * Class:	None.
 * Purpose:	Compress weights on outgoing arcs of a node so that they fit
 *		into one byte.
 * Parameters:	n	- (i/o) the node to be treated.
 * Results:	Nothing.
 * Remarks:	Weights cannot be zero, as this may create a division by zero
 *		problem in computing probabilities. Weights are shifted right
 *		until the biggest one fits in a byte. If zero weights appear
 *		as a result of that process, they are replaced by 1.
 */
void
flatten_weights(node *n)
{
  int		kids = n->get_no_of_kids();
  int		max_weight = 0;
  arc_node 	*an = n->get_children();

  // find out what the maximum weight is
  for (int wi = 0; wi < kids; wi++, an++) {
    if (an->weight > max_weight)
      max_weight = an->weight;
  }

  // flatten
  while (max_weight > 255) {
    an = n->get_children();
    if (max_weight > 0xFFFF) {
      // we need at least one byte shift
      for (int wj = 0; wj < kids; wj++, an++) {
	an->weight >>= 8;
      }
      max_weight >>= 8;
    }
    else {
      for (int wk = 0; wk < kids; wk++, an++) {
	an->weight >>= 1;
	if (an->weight < 1)
	  an->weight = 1;
      }
      max_weight >>= 1;
    }
  }
}//flatten_weights
#endif //WEIGHTED

/***	EOF nnode.cc	***/
