/***	mkindex.cc	***/

/*	Copyright (C) Jan Daciuk, 1996-2004.	*/

/* Note: this file is conditionally included into fsa_build.cc */

using namespace std;

// Status of nodes - this is kept in a separate variable
const int	UNREDUCIBLE = 4;
const int	WITH_ANNOT = 2;
const int	NO_ANNOT = 1;

// Categories of nodes kept in nodes as arc numbers
const int	NODE_TO_BE_REDUCED = -5;
const int	NODE_UNREDUCIBLE = -6;
const int	NODE_IN_TAGS = -7;
const int	NODE_MERGED = -8;
const int	NODE_TO_BE_MERGED = -9;

// Constants affecting the pruning process
const int	MIN_PRUNE = 2;	// min no of arcs per node to be pruned
const int	MAX_DESTS = 32;	// max number of different destinations
const int	MIN_DESTS_MEMBERS = 0; // min number of arcs
                                       // going to merged destinations
const int	MAX_ANNOTS = 20;       // max number of annotations
                                       // to be merged
const int	MAX_DIFF_ANNOTS = 20;  // max number of different annotations
				       // to be merged
const int	MIN_KIDS_TO_MERGE = 2; // minimal number of kids to be merged
const int	MIN_ANNOTS = 3;	       // maximal number of annots to be merged

// Those 2 constitute together a fraction of the total weight
// of all annotations of a node, a default annotation must be bigger than that
const int	AN_NOM = 1;	// nominator
const int	AN_DENOM = 2;	// denominator


struct arc_count_str { node *address; int counter; char letter;};


int count_diff_dests(const node *n, int &different_dests, arc_count_str *arcs,
		     const int max_dest);
#ifdef GENERALIZE
node *generalize(node *n);
int contained_in(const node *n1, const node *n2);
node *merge_annots(node *n1, node *n2);
int add_annots(ann_inf *annotations, const int bno, ann_inf *small_annots,
	       const int sno, const int prefix);
int collect_secondary(const node *n, node *v[], const int m);
#endif
#ifdef WEIGHTED
void transfer_weights(node *bigger, node *smaller);
#endif

/* Name:	reduce_inner
 * Class:	node
 * Purpose:	Delete nodes in the automaton that have all arcs leading
 *		to the same node.
 * Parameters:	n		- (i) node to be (possibly) reduced.
 * Returns:	Pointer to this node, or to the node pointed to by all
 *		arcs from this node.
 * Remarks:	In the center of the automaton, there are chains of nodes
 *		linked with single arcs. Because for every node in those
 *		chains, there is a single path leading to annotations,
 *		these nodes carry no useful information.
 *
 *		Nodes from annotations cannot be reduced.
 *
 *		Nodes that have arcs leading to nodes that cannot be reduced,
 *		cannot be reduced either.
 *
 *		Only nodes that have all arcs leading to the same (or
 *		isomorphic) node that has arcs labeled with ANNOT_SEPARATOR
 *		can be reduced.
 */
node *
reduce_inner(node *n)
{
  node		*token;
  node		*p;
  arc_node	*an;
  int		offspring_status;
  node		*new_child;
  int		node_modified;


  if (n->get_no_of_kids() == 0) {
    cerr << "No annotations or invalid annotation separator" << endl;
    exit(6);
  }

  if (n->get_arc_no() > NODE_TO_BE_REDUCED) {
    // node not visited yet
    n->set_arc_no(NODE_TO_BE_REDUCED);	// mark node as  that to be reduced

    // try to reduce children of this node
    offspring_status = 0;
    node_modified = FALSE;
    an = n->get_children();
    for (int i = 0; i < n->get_no_of_kids(); i++, an++) {

      if (an->letter != ANNOT_SEPARATOR) {

	new_child = reduce_inner(an->child);
	if (new_child != an->child) {
	  if (!node_modified) {
	    unregister_node(n);
	    node_modified = TRUE;
	  }
	  an->child = new_child;
	}

	if (an->child->get_arc_no() == NODE_UNREDUCIBLE)
	  offspring_status = UNREDUCIBLE;
	else
	  offspring_status |= NO_ANNOT;
      }

      else { // an->letter == ANNOT_SEPARATOR
	// the pruning process does not apply to tags
	mark_inner(an->child, NODE_IN_TAGS);
	n->set_arc_no(NODE_IN_TAGS);
	offspring_status |= WITH_ANNOT;
      }
    }//for i


    if ((offspring_status & UNREDUCIBLE) > 0) {
      // a parent of unreducible child cannot be reduced (R3)
      n->set_arc_no(NODE_UNREDUCIBLE);
    }

    // no arc is labeled with ANNOT_SEPARATOR
    else if (offspring_status == NO_ANNOT) {
      // Compare children on isomorphism
      an = n->get_children();
      token = an++->child;
      for (int j = 1; j < n->get_no_of_kids(); j++, an++) {
	// only addresses are compared, as isomorphic children were replaced
	// earlier
	if (token != an->child) {
	  n->set_arc_no(NODE_UNREDUCIBLE);
	  break;
	}
      }
      if (n->get_arc_no() != NODE_UNREDUCIBLE) {
	// we got here so all arcs from this node lead to one node
	// here the n node must be deleted and replaced by one of its children
	// the other children must be deleted (R5)
	an = n->get_children();
	token = an->child;
	delete_index_node(n);
	return token;
      }//if arc leads to node that can be reduced
    }

#ifdef PRUNE_ARCS
    n = prune_arcs(n);
#else
//#ifdef GENERALIZE
//    n = generalize(n);
//#endif
#endif
    if (node_modified && n->get_arc_no() != NODE_TO_BE_REDUCED &&
	n->get_arc_no() != NODE_MERGED)
      if ((new_child = find_or_register(n, PRIM_INDEX, TRUE)) != NULL) {
	if (new_child != n) {
	  // Node was modified so that now it is isomorphic to another,
	  //  registered node. So the node n must be deleted. However,
	  //  its arc_node, serving as a marker, must be copied to the other
	  //  node to prevent its reexamining
	  new_child->set_arc_no(n->get_arc_no());
	  new_child->hit_node();
	  delete_index_node(n);
	  n = new_child;
	}
	// else we got another node as n - a node that was deeper in
      }
  }//if node not visited yet

  // if this node is to be reduced
  if ((new_child = find_or_register(n, PRIM_INDEX, FIND)) != n &&
      new_child != NULL) {
    // The node should be replaced by a isomorphic node
    new_child->hit_node();
    delete_index_node(n);
    n = new_child;
  }
  if (n->get_arc_no() == NODE_TO_BE_REDUCED) {
    p = n->get_children()[0].child;	// remember (all the same) children
    delete_index_node(n);
    // the node is to be reduced - so return the child
    return p;
  }
  else
    return n;
}//node::reduce_inner

/* Name:	rebuild_index
 * Class:	none.
 * Purpose:	Rebuild index after it has been destroyed.
 * Parameters:	n		- (i) the first node of a subnode to be put
 *					into index.
 * Returns:	n or equivalent node already registered.
 * Remarks:	It is assumed that nodes have arc_no value < -1.
 *		This feature is used to distinguish between nodes that
 *		have already been visited in this function, and others.
 *		Nodes that have already been visited receive arc_no = -1.
 */
node *
rebuild_index(node *n)
{
  arc_node	*kid;
  node		*r;

  if (n->get_arc_no() == -1)
    return find_or_register(n, PRIM_INDEX, 0);

  n->set_arc_no(-1);
  kid = n->get_children();
  for (int i = 0; i < n->get_no_of_kids(); i++, kid++)
    if (kid->child)
      kid->child = rebuild_index(kid->child);
  if ((r = find_or_register(n, PRIM_INDEX, 1)) == NULL) {
    return n;
  }
  else {
    if (n->hit_node(-1) <= 0)
      delete n;
    return r;
  }
}//rebuild_index


/* Name:	delete_index_node
 * Class:	None.
 * Purpose:	Delete the node if possible; update hit_count where needed.
 * Parameters:	n		- (i) node to be deleted;
 *		hit_children	- (i) whether to increase hit_count
 *					for children of n.
 * Returns:	TRUE if deleted, FALSE otherwise.
 * Remarks:	If hit count for the node is greater than 1, it means that
 *		there is another node pointing to this one. So the node
 *		cannot be deleted. For the parent node, however, i.e. the
 *		node that invoked mark_inner with this node as argument,
 *		this node is replaced with its children. This means creating
 *		another link to the children, so their hit counts must be
 *		increased.
 */
int
delete_index_node(node *n, const int hit_children)
{
  arc_node *an = n->get_children();
  if (n->hit_node(-1) <= 0) {
    if (hit_children) {
      // This node is deleted (if possible),
      // but another link to child is created
      an++;
      for (int i = 1; i < n->get_no_of_kids(); i++, an++)
	an->child->hit_node(-1);
    }
    unregister_node(n);
    delete n;
    return TRUE;
  }
  else if (hit_children && an->child)
    an->child->hit_node();
  return FALSE;
}//delete_index_node

/* Name:	is_annotated
 * Class:	None.
 * Purpose:	Checks if the node has no arcs but one with annotation
 *		separator.
 * Parameters:	n		- (i) node to be checked.
 * Returns:	TRUE if there is only one arc, and it labeled with ANNOT_SEP.,
 *		FALSE otherwise.
 * Remarks:	Should be inline.
 */
int
is_annotated(const node *n)
{
  return (n->get_no_of_kids() == 1 &&
	  n->get_children()->letter == ANNOT_SEPARATOR);
}//is_annotated

#ifdef PRUNE_ARCS
/* Name:	prune_arcs
 * Class:	None.
 * Purpose:	Replace many arcs leading to one node with one arc.
 * Parameters:	n		- (i) the node to be considered.
 * Returns:	The node.
 * Remarks:	1. If the node has an arc labeled with annotation separator
 *		   it cannot be pruned.
 *		2. The number of arcs to be pruned must be at least
 *		   MIN_PRUNED for one node.
 *		3. There should be exactly one arc leading to a node
 *		   other than those pointed to by the nodes to be pruned.
 *		4. The arcs to be pruned must all lead to the same node
 *		   that must have all arcs leaving that node labeled with
 *		   annotation separator.
 *		4. All pruned arcs are replaced with children of the node
 *		   they lead to.
 */
node *
prune_arcs(node *n)
{
  const int	DIFFERENT_ARCS = 20;
  arc_node	*an;
  arc_count_str	arcs[DIFFERENT_ARCS];
  int		different_dests = 0;
  int		j;
  int		hit_children;
  node		*new_node;

  if (n->get_no_of_kids() < MIN_PRUNE + 1)
    // Nothing to be pruned - to few arcs
    return n;

  // Count arcs leading to different nodes
  if (count_diff_dests(n, different_dests, arcs, DIFFERENT_ARCS) == 0)
    return n;

  // Find the most popular destination
  int biggest = 0;
  node *to_be_slashed = NULL;
  for (int i1 = 0; i1 < different_dests; i1++) {
    if (arcs[i1].counter > biggest) {
      biggest = arcs[i1].counter;
      to_be_slashed = arcs[i1].address;
    }
  }

  // Check if there are enough arcs to be slashed
  if (biggest < (MIN_PRUNE * (n->get_no_of_kids() - biggest)))
    // Sorry, not enough arcs to be pruned
    return n;

  // Check if the biggest is eligible
  if (!is_annotated(to_be_slashed))
    // Sorry, arcs to be pruned must lead to nodes that have arcs with
    // ANNOT_SEPARATOR only
    return n;

  // Prepare new children of n
  // First arcs of n will now be the arcs of the node the pruned arcs lead to
  arc_node *narc = new arc_node[(n->get_no_of_kids() - biggest) + 1];
  memcpy(narc, to_be_slashed->get_children(),
	 sizeof(arc_node));
#ifdef WEIGHTED
  narc->weight *= biggest;
#endif
  // Now copy the other arcs
  arc_node *firstp = narc + 1;
  an = n->get_children();
  for (int i2 = 0; i2 < n->get_no_of_kids(); i2++, an++) {
    if (an->child != to_be_slashed)
      *firstp++ = *an;
  }

  // Prune!
  unregister_node(n);
  hit_children = TRUE;
  an = n->get_children();
  for (j = 0; j < n->get_no_of_kids(); j++, an++) {
    if (an->child == to_be_slashed) {
      delete_index_node(an->child, hit_children);
      hit_children = FALSE;
    }
  }

  // Install new children
  n->set_arc_no(NODE_UNREDUCIBLE);
  delete [] n->set_children(narc, (n->get_no_of_kids() - biggest) + 1);
  // Sort children so that optimization works
  qsort(n->get_children(), n->get_no_of_kids(), sizeof(arc_node), cmp_arcs);
  if ((new_node = find_or_register(n, PRIM_INDEX, TRUE)) != NULL) {
    delete_index_node(n, FALSE);
    n = new_node;
    n->hit_node();
  }
  return n;
}//prune_arcs

#endif

/* Name:	count_diff_dests
 * Class:	None.
 * Purpose:	Register and count different destinations of outgoing arcs.
 * Parameters:	n		- (i) the node to be examined;
 *		different_dests	- (o) number of different destinations;
 *		arcs		- (o) different destinations;
 *		max_dest	- (i) max number of different destinations.
 * Returns:	Number of different destinations.
 * Remarks:	If the node has an arc labeled with annotation separator,
 *		or if its arcs lead to more than max_dest destinations,
 *		0 is returned instead of the number of different destinations.
 */
int
count_diff_dests(const node *n, int &different_dests, arc_count_str *arcs,
		 const int max_dest)
{
  arc_node	*an;
  int		add;

  // Count arcs leading to different nodes
  an = n->get_children();
  for (int i = 0; i < n->get_no_of_kids(); i++, an++) {
    if (an->letter == ANNOT_SEPARATOR)
      // Nothing to be pruned here - ANNOT_SEPARATOR prohibits pruning
      return 0;

    // Increase counter for appropriate node if possible
    add = TRUE;
    for (int j = 0; j < different_dests; j++) {
      if (arcs[j].address == an->child) {
	arcs[j].counter++;
	add = FALSE;
	break;
      }
    }//for j
    if (add) {
      // New arc destination - add if possible
      if (different_dests < max_dest) {
	arcs[different_dests].address = an->child;
	arcs[different_dests].letter = an->letter;
	arcs[different_dests++].counter = 1;
      }
      else
	// Nothing to prune - more than max_dest different destinations
	return 0;
    }//if add
  }//for i
  return different_dests;
}//count_diff_dests


#ifdef GENERALIZE

/* Name:	generalize
 * Class:	None.
 * Purpose:	If the part of the automaton reachable from this node
 *		has arcs labeled with annotation separator that lead to
 *		only two destinations, merge them as one annotation of
 *		this node.
 * Parameters:	n		- (i) the node to be examined.
 * Returns:	The node.
 * Remarks:	Possible situations include:
 *		1)	The node has arcs leading to one destination.
 *		2)	The node has arcs leading to two destinations.
 *		3)	The node has arcs leading to two destinations,
 *			one simple, the other one being a merger of
 *			the simple destination, and another simple
 *			destination.
 *		4)	The node has arcs leading to two simple destinations,
 *			and to a destination being a merger of those
 *			two simple destinations.
 *		5)	None of the above.
 *
 *		The algorithm:
 *		First gather information on all immediate destinations.
 *		For all immediate destinations
 *			gather information on their (target) destinations
 *			and merge that data here
 */
node *
generalize(node *n)
{
  arc_node	*an;		// currently analyzed arc
  node		*new_node = NULL; // new node
  node		*merged_node;	// result of merger
  node		*dests[MAX_DESTS]; // targets of out-transitions from n
#ifdef WEIGHTED
  int		dests_count[MAX_DESTS];	// number of transitions per target
#endif
  int		no_of_dests = 0; // number of different targets
  int		i;
  node		*kid;
  int		first_dest_count;
  int		index_touched = 0;

  // Check if the node is eligible
  if (is_annotated(n))
    return n;
  an = n->get_children();
  for (int j = 0; j < n->get_no_of_kids(); j++, an++) {
    if (!is_annotated(an->child))
      return n;

    kid = an->child;
    for (i = 0; i < no_of_dests; i++)
      if (dests[i] == kid) {
#ifdef WEIGHTED
	dests_count[i]++;
#endif //WEIGHTED
	break;
      }
    if (i >= no_of_dests) {
      if (no_of_dests < MAX_DESTS) {
	// insert new destination
	dests[no_of_dests] = kid;
#ifdef WEIGHTED
	dests_count[no_of_dests] = 1;
#endif //WEIGHTED
	no_of_dests++;
      }
      else
	return n;	// too many destinations
    }
    // else kid is already in dests
  }

  // New code: check if any of the destinations is contained in another
  for (int ii = 0; ii < no_of_dests; ii++) {
    for (int jj = 0; jj < no_of_dests; jj++) {
      if ((ii != jj) && contained_in(dests[jj], dests[ii])) {
	// dests[jj] is contained in dests[ii]
	// replace every occurence of dests[jj] with dests[ii] in children of n
#ifdef WEIGHTED
	if (dests[ii]->hit_node(0) == dests_count[ii]) {
	  // no other links to that destination, safe to alter
	  transfer_weights(dests[ii], dests[jj]);
	}
	else {
	  // I'm not sure what to do here - create a copy of dests[ii] ?
	  // let's try
	  node *new_child_node = new node(dests[ii]);
	  // new node means new links
	  an = n->get_children();
	  for (int ll=0; ll < new_child_node->get_no_of_kids(); ll++, an++) {
	    an->child->hit_node();
	  }
	  an = n->get_children();
	  for (int x = 0; x < n->get_no_of_kids(); x++, an++) {
	    if (an->child == dests[ii]) {
	      // one less link to the old node
	      an->child->hit_node(-1);
	      // redirect link
	      an->child = new_child_node;
	      // increase hit count
	      // note that we cannot register the node,
	      // as it is isomorphic to the old an->child,
	      // only the wieghts are different
	      new_child_node->hit_node();
	    }
	  }
	  transfer_weights(new_child_node, dests[jj]);
	  dests[ii] = new_child_node;
	}
#endif //WEIGHTED
	an = n->get_children();
	for (int k = 0; k < n->get_no_of_kids(); k++, an++) {
	  if (an->child == dests[jj]) {
	    if (!index_touched) {
	      new_node = new node(n);
	      index_touched = 1;
	    }
	    //delete_old_branch(an->child);
	    new_node->get_children()[k].child = dests[ii];
	    //an->child->hit_node(); // new link to that node
	  }
	}
	for (int kk = jj; kk < no_of_dests - 1; kk++) {
	  dests[kk] = dests[kk + 1];
	}
	--jj;			// the loop above placed new item at i
	--no_of_dests;
	if (ii > jj) {
	  // the item that was at ii is now at ii-1
	  --ii;
	}
      }
    }
  }

#ifdef WEIGHTED
  int sum = 0;
  an = n->get_children();
  for (int sumi = 0; sumi < n->get_no_of_kids(); sumi++, an++) {
    sum += an->weight;
  }
#endif
  if (no_of_dests == 1) {
    // all children are the same now after merger, so this node is no longer
    // needed
    // dests[0] is the only destination of all arcs leaving n
    // first, make sure that dests[0] will not be deleted
    (dests[0])->hit_node();
    // then delete the node n and nodes below it, if they have hit_count=1
    delete_old_branch(n);
    // new_node is not registered yet, and its arcs are not counted
    //  in hit counts of their targets, so we can simply delete it
    if (index_touched)
      delete new_node;
#ifdef WEIGHTED
    // update weights of dests[0] - they should be the same as the weigth of n
    dests[0]->get_children()->weight = sum;
#endif
    // now we can use dests[0] as node n
    return dests[0];
  }

  if (index_touched) {
    // We created a new node that is to be used instead of n
    if ((merged_node = find_or_register(new_node, PRIM_INDEX, TRUE)) != NULL) {
      // that new node is isomorphic to a node already in the fsa
      // new node is not registered, nor included in hit count,
      // so we can simply delete it
#ifdef WEIGHTED
      // but before we do that, let's update weights
      // after this the weights on merged node should be increased by weights
      // on new_node
      transfer_weights(merged_node, new_node);
#endif
      delete new_node;
      // no need to hit the children of merged_node,
      // because there are already arcs from merged_node to them,
      // and those arcs are included in hit count
    }
    else {
      // else our new_node is unique, it got registered
      merged_node = new_node;
      an = merged_node->get_children();
      // our new node will take place of n, taking over n's children
      // however, delete_old_branch deletes children of n
      // solution: increase hit count of children that will be in new_node
      for (int iii=0; iii < merged_node->get_no_of_kids(); iii++)
	if (an->child)
	  an++->child->hit_node();
    }
    merged_node->hit_node();
    delete_old_branch(n);
    n = merged_node;
  }
      

  // Now we have 2 or 3 destinations. One of them may be a merger
  // of other 2 destinations
  if (no_of_dests > 3 || no_of_dests < 2)
    return n;
  if (no_of_dests == 2) {
    first_dest_count = 0;
    an = n->get_children();
    for (int iiii = 0; iiii < n->get_no_of_kids(); iiii++, an++)
      if (an->child == dests[0])
	first_dest_count++;
    if (dests[0]->get_arc_no() != NODE_MERGED &&
	dests[1]->get_arc_no() != NODE_MERGED &&
	n->get_no_of_kids() >= (2 * MIN_DESTS_MEMBERS) &&
	first_dest_count >= MIN_DESTS_MEMBERS) {
      // Nodes are different, merge them
      new_node = merged_node = merge_annots(dests[0], dests[1]);
    }
    else {
      // Merger not possible
      return n;
    }
    // As the node will take place on n node, increase hit count accordingly
    new_node->hit_node();
    // delete this node and all nodes beneath reachable only from
    // this part of the automaton
    n->set_arc_no(NODE_MERGED);// changed from NODE_TO_BE_MERGED
    delete_old_branch(n);
    return new_node;
  }
  // generalization not possible
  return n;
}//generalize



/* Name:	merge_annots
 * Class:	None.
 * Purpose:	Merges 2 (annotation) nodes into 1.
 * Parameters:	n1		- (i/o) first node;
 *		n2		- (i/o) second node.
 * Returns:	New (merged) node.
 * Remarks:	If nodes are the same, they constitute a merged node.
 *		A merged node has arcs labeled with all letters present
 *		in all arcs of n1 and n2. If both nodes have arcs labeled
 *		with the same letter, the corresponding arc in the merged
 *		node leads to a new merged node.
 *
 *		WARNING: input data is sorted with sort command that may
 *		sort differently than simple byte comparison used here!
 */
node *
merge_annots(node *n1, node *n2)
{
  arc_node	*an1, *an2;	// arcs of n1 and n2
  int		k1, k2;		// number of kids in n1 and n2
  int		i1, i2;		// current arc number in n1 and n2
#ifdef WEIGHTED
  int		i3;
#endif
  node		*merged_node;
  node		*new_node;

  // See if a new node really is needed
  if (n1 == n2)
    return n1;

  // Merge arcs of both nodes
  an1 = n1->get_children();
  k1 = n1->get_no_of_kids();
  an2 = n2->get_children();
  k2 = n2->get_no_of_kids();
  i1 = 0; i2 = 0;
#ifdef WEIGHTED
  i3 = 0;
#endif
  merged_node = new node;
  merged_node->set_arc_no(NODE_MERGED);	// indicate the node has been processed
  do {
    if (i1 >= k1) {
      // No more ars left in n1
      // Append arcs from n2 to merged_node
      merged_node->add_child(an2->letter, an2->child);
      i2++; an2++;
#ifdef WEIGHTED
      merged_node->get_children()[i3++].weight = an2->weight;
#endif
    }
    else if (i2 >= k2) {
      // No more arcs left in n2
      // Append arcs from n1 to merged_node
      merged_node->add_child(an1->letter, an1->child);
      i1++; an1++;
#ifdef WEIGHTED
      merged_node->get_children()[i3++].weight = an1->weight;
#endif
    }
    else {
      if (an1->letter == an2->letter) {
	// Both nodes have an arc with the same label
	if (an1->child == an2->child) {
	  // They both point to the same node - no need to merge them
	  merged_node->add_child(an1->letter, an1->child);
	  if (an1->is_final || an2->is_final)
	    merged_node->get_children()[merged_node->get_no_of_kids()
					- 1].is_final = TRUE;
#ifdef WEIGHTED
	  merged_node->get_children()[i3++].weight = an1->weight + an2->weight;
#endif
	}
	else {
	  if (an1->child == NULL || an2->child == NULL) {
	    merged_node->add_child(an1->letter,
				   an1->child ? an1->child : an2->child);
	    merged_node->get_children()[merged_node->get_no_of_kids()
					- 1].is_final = TRUE;
	  }
	  else {
	    // They point to different locations,
	    // so a new merged node is needed
	    merged_node->add_child(an1->letter,
				   merge_annots(an1->child, an2->child));
	    if (an1->is_final || an2->is_final)
	      merged_node->
		get_children()[merged_node->get_no_of_kids()-1].is_final = TRUE;
#ifdef WEIGHTED
	    merged_node->get_children()[i3++].weight = an1->weight +
	      an2->weight;
#endif
	  }
	}
	an1++; i1++;
	an2++; i2++;
      }
      else if (an1->letter < an2->letter) {
	merged_node->add_child(an1->letter, an1->child);
#ifdef WEIGHTED
	merged_node->get_children()[i3++].weight = an1->weight;
#endif
	an1++; i1++;
      }
      else {
	merged_node->add_child(an2->letter, an2->child);
#ifdef WEIGHTED
	merged_node->get_children()[i3++].weight = an2->weight;
#endif
	an2++; i2++;
      }
    }
  } while (i1 < k1 || i2 < k2);

  // See if merged_node is unique
  if ((new_node = find_or_register(merged_node, 0, TRUE))) {
    // Such a node already exists in the automaton
    // Delete merged_node and use the already existing copy
    delete merged_node;
    new_node->set_arc_no(NODE_MERGED);
    return new_node;
  }
  else {
    // Node is unique, registered
    an1 = merged_node->get_children();
    // We have a new node, and new links as well!
    for (int i = 0; i < merged_node->get_no_of_kids(); i++, an1++)
      if (an1->child)
	an1->child->hit_node();
    return merged_node;
  }
}//merge_annots

/* Name:	contained_in
 * Class:	None.
 * Purpose:	Check if the first node may be a starting point of
 *		an automaton that is contained in another automaton
 *		with the starting point in the second node.
 * Parameters:	n1	- (i) first (presumably smaller) node;
 *		n2	- (i) second (presumably larger) node.
 * Returns:	TRUE if n1 is contained in n2, FALSE otherwise.
 * Remarks:	This is to check if n2 is a merger of n1 and another node.
 */
int
contained_in(const node *n1, const node *n2)
{
  int		k1, k2;
  arc_node	*an1, *an2;
  int		found;

  if ((k1 = n1->get_no_of_kids()) > ((k2 = n2->get_no_of_kids())))
    return FALSE;

  an1 = n1->get_children();
  for (int i = 0; i < k1; i++, an1++) {
    found = FALSE;
    an2 = n2->get_children();
    for (int j = 0; j < k2; j++, an2++) {
      if (an1->letter == an2->letter) {
	// matching arc found
	if (an1->child == an2->child)
	  // and the destination is the same => arcs are identical
	  found = TRUE;
	else if ((an1->child == NULL) || (an2->child == NULL)) {
	  // and the destinations do not match
	  found = FALSE;
	}
	else {
	  // look deeper
	  found = contained_in(an1->child, an2->child);
	}
	break; // since matching arc for n1->children[i] found, stop searching
      }
    }
    if (!found)
      return FALSE;
  }
  return TRUE;
}//contained_in

#endif // GENERALIZE

#ifdef WEIGHTED
/* Name:	transfer_weights
 * Class:	None.
 * Purpose:	Add weights from branches of one node to another one
 *		that contains a superset of annotations for the first one.
 * Parameters:	bigger		- (i/o) the bigger node with superset
 *					of annotations;
 *		smaller		- (i) the smaller node with subset
 *					of annotations.
 * Returns:	Nothing.
 * Remarks:	It is assumed that contained_in(smaller, bigger) returned TRUE.
 *		Each path in the smaller node corresponds to a path
 *		in the bigger node. Add weights from the arcs in the samller
 *		node to the corresponding arcs in the bigger node.
 */
void
transfer_weights(node *bigger, node *smaller)
{
  arc_node	*anb, *ans;

  ans = smaller->get_children();
  for (int i = 0; i < smaller->get_no_of_kids(); i++) {
    anb = bigger->get_children();
    for (int j = 0; j < bigger->get_no_of_kids(); j++) {
      if (ans->letter == anb->letter) {
	// arcs match
	anb->weight += ans->weight;
	if (ans->child != anb->child) {
	  transfer_weights(anb->child, ans->child);
	}
      }
    }
  }
}//transfer_weights
#endif
#ifdef WEIGHTED
/* Name:	weight_arcs
 * Class:	None.
 * Purpose:	Associate anumber with arcs in the automaton. The number
 *		tells how many strings are associated with the arc.
 * Parameters:	n	- (i) node who's arcs are to be weighted.
 * Result:	Sum of weights for arcs of this node.
 * Remarks:	This applies to guessing automata.
 *		The weight of an arc is the sum of weights on arcs going out
 *		from its target node. That sum is increased by one
 *		if the arc is final. The weight of an arc with a NULL pointer
 *		is one (it must be final).
 *		Arcs already counter have non-zero weight.
 *		The weight of a node is the sum of the weights of all its
 *		outgoing arcs.
 */
int
weight_arcs(const node *n)
{
  arc_node	*an;

  int sum = 0;
  if (n == NULL)
    return 0;
  an = n->get_children();
  if (an->weight) {
    // The node has already been weighted.
    // calculate the sum, but don't go any deeper, nor change anything
    for (int k = 0; k < n->get_no_of_kids(); k++, an++) {
      sum += an->weight;
    }
  }
  else {
    for (int i = 0; i < n->get_no_of_kids(); i++, an++) {
      sum += (an->weight = weight_arcs(an->child) + (an->is_final? 1 : 0));
    }
  }
  return sum;    
}//weight_arcs
#endif //WEIGHTED

#ifdef GENERALIZE
/* Name:	collect_annotations
 * Class:	None.
 * Purpose:	Collects annotations on all annotations
 *		reachable from the node.
 * Parameters:	n	- (i) node to be analysed;
 *		prefix	- (i) TRUE if prefixes in data, FALSE otherwise.
 * Returns:	Number of annotated nodes below.
 * Remarks:	An annotated node is a node with an outgoing arc labelled
 *		with the annotation separator. The must be no annotation
 *		separator on arcs leading from the start node (root) to
 *		the annotated node. Note that the number of annotated nodes
 *		is not equal to the number of different annotations, because
 *		one annotated node may have more than one annotation.
 *		Annotations are stored in dynamically allocated vector
 *		annotations assigned to each node.
 *
 *		If there are too many annotations for a node, nannot is
 *		set to -1.
 *
 *		If a number of annotations reachable from the node is set
 *		to 0, the node has not been visited yet,
 *		because the construction of guessing automata requires
 *		that from each node not forming a part of annotations
 *		it should be possible to reach at least one annotation.
 */
int
collect_annotations(node *n, const int prefix)
{
  ann_inf annotations[MAX_ANNOTS];
  ann_inf child;
  int no_of_annots = 0;
  int d = 0;
  int too_many = FALSE;

  if (n->nannots == 0) {
    // node not visited yet
    arc_node *an = n->get_children();
    for (int i = 0; i < n->get_no_of_kids(); i++, an++) {
      if (an->letter == ANNOT_SEPARATOR) {
	// It's an annotation, and n is an annotated node
	child.annot = an->child;
#ifdef WEIGHTED
	child.weight = an->weight;
#endif //!WEIGHTED
	if (too_many ||
	    (no_of_annots =
	     add_annots(annotations, no_of_annots, &child, 1, prefix))) {
	  // Too many annotations
	  n->annots = NULL;
	  n->nannots = -1;
	  too_many = TRUE;
	}
      }
      else {
	collect_annotations(an->child, prefix);
	d = no_of_annots;
	if (too_many ||
	    (d = add_annots(annotations, no_of_annots, an->child->annots,
		       an->child->nannots, prefix)) == 0) {
	  // Too many annotations
	  n->annots = NULL;
	  n->nannots = -1;
	  too_many = TRUE;
	}
	no_of_annots = d;
      }
    }
    if (!too_many) {
      n->nannots = no_of_annots;
      n->annots = new ann_inf [no_of_annots];
      memcpy(n->annots, annotations, no_of_annots * sizeof(ann_inf));
    }
  }
  return (too_many ? -1 : n->nannots);
}//collect_annotations


/* Name:	new_gen
 * Class:	None.
 * Purpose:	Generalizes annotations for a given node.
 * Parameters:	n	- (i/o) node to be processed;
 *		pref	- (i) TRUE if prefixes present in data.
 * Returns:	TRUE if node generalized, FALSE otherwise.
 * Remarks:	The field annots contains annotated nodes that are reachable
 *		from this node. In short: they are annotations of this node.
 *		The field nannots says how many of them there are.
 *		If nannots is -1, it means that there too many of them.
 *		In that case, the node cannot be generalized.
 *
 *		There are two modes in which new_gen operates: with or
 *		without prefixes. The mode depends on the value
 *		of the variable pref: if it is TRUE, prefixes are used.
 *
 *		In prefix mode, annotated nodes with different prefixes
 *		can be merged. Annotated nodes with the same prefix
 *		are treated as different annotated nodes.
 *
 *		Annotated nodes whose annotations form a subset of another
 *		annotated node are treated as instances of that other node.
 *
 *		The node can be generalized if the same set of annotated
 *		nodes appears in every its child.
 *
 *		The node can be generalized if arcs going out of the node
 *		cannot be divided into groups leading to separate sets
 *		of annotations.
 */
int
new_gen(node *n, const int pref)
{
  arc_node *an;
  //
  if (n->nannots == -1) {
    // This node cannot be generalized, so move downwards
    an = n->get_children();
    for (int i = 0; i < n->get_no_of_kids(); i++, an++) {
      if (an->letter != ANNOT_SEPARATOR) {
	new_gen(an->child, pref);
      }
    }
    return 0;			// too many annotations
  }
  if (n->nannots == 1) {
    if (n->get_children()->letter == ANNOT_SEPARATOR) {
      // We got to the annotated nodes.
      // Nothing can be done here.
      return 0;
    }
    else if (n->get_no_of_kids() == 1) {
      // There is one child (which has identical annotations).
      // Replace destination of the single outgoing arc with
      // the single annotation. Note that the weight stays the same.
      unregister_node(n);	// we are changing its children
      n->annots[0].annot->hit_node();
      delete_old_branch(n->get_children()->child);
      n->get_children()->letter = ANNOT_SEPARATOR;
      n->get_children()->child = n->annots[0].annot;
      return 1;
    }
    else {
      // The node has one annotation but many children.
      // Delete all children, and create a new child with annotation
      // separator leading to annots[0]
#ifdef WEIGHTED
      int sum = 0;
#endif
      an = n->get_children();
      unregister_node(n);	// we modify its children
      for (int k = 0; k < n->get_no_of_kids(); k++, an++) {
#ifdef WEIGHTED
	sum += an->weight;
#endif //WEIGHTED
	delete_old_branch(an->child);
      }
      delete [] n->get_children();
      arc_node *new_kids = new arc_node[1];
      new_kids->child = n->annots[0].annot;
      new_kids->letter = ANNOT_SEPARATOR;
#ifdef WEIGHTED
      new_kids->weight = sum;
#endif
      n->set_children(new_kids, 1);
      new_kids->child->hit_node();
      return 1;
    }
  }
  int children_the_same = TRUE;
  if (n->get_no_of_kids() > 1) {
    an = n->get_children();
    for (int l = 0; l < n->get_no_of_kids(); l++, an++) {
      if (an->child->nannots != n->nannots) {
	children_the_same = FALSE;
	break;
      }
      for (int m = 0; m < n->nannots; m++) {
	if (n->annots[m].annot != an->child->annots[m].annot) {
	  children_the_same = FALSE;
	  break;
	}
      }
    }
    if (children_the_same) {
      // all children have the same annotations
      an = n->get_children();
      unregister_node(n);	// we modify its children
#ifdef WEIGHTED
      int sum = 0;
#endif
      for (int p = 0; p < n->get_no_of_kids(); p++, an++) {
#ifdef WEIGHTED
	sum += an->weight;
#endif
	delete_old_branch(an->child);
      }
      // merge annotations (there are at least two)
      node *merged_node = merge_annots(n->annots[0].annot, n->annots[1].annot);
      for (int s = 2; s < n->nannots; s++) {
	merged_node = merge_annots(merged_node, n->annots[s].annot);
      }
      unregister_node(n);	// we change its children (reregister...)
      delete [] n->get_children();
      arc_node *new_kids = new arc_node[1];
      new_kids->child = merged_node;
      new_kids->is_final = 0;
      new_kids->letter = ANNOT_SEPARATOR;
#ifdef WEIGHTED
      new_kids->weight = sum;
#endif
      n->set_children(new_kids, 1);
      new_kids->child->hit_node();
      return 1;
    }
  }
#ifdef WEIGHTED
  else {
    int sum1 = 0;
    an = n->get_children();
    for (int w = 0; w < n->get_no_of_kids(); an++, w++)
      sum1 += an->weight;
    for (int z = 0; z < n->nannots; z++) {
      if ((n->annots[z].weight * AN_NOM / AN_DENOM) > sum1) {
	// most weights are on one annotation
	// delete children and make the one annotation the child
	// but first make sure it won't be deleted
	n->annots[z].annot->hit_node();
	unregister_node(n);	// we change its children (reregister...)
	an = n->get_children();
	for (int z1 = 0; z1 < n->get_no_of_kids(); z1++, an++) {
	  delete_branch(an->child);
	}
	delete [] n->get_children();
	an = new arc_node[1];
	an->letter = ANNOT_SEPARATOR;
	an->child = n->annots[z].annot;
	an->weight = n->annots[z].weight;
	an->is_final = FALSE;
	n->set_children(an, 1);
	return 1;
      }
    }
    // many children and only a few annotations suggest they should be merged
    if (n->get_no_of_kids() > MIN_KIDS_TO_MERGE && n->nannots <= MIN_ANNOTS) {
      // merge all annots
      node *merged_node = merge_annots(n->annots[0].annot, n->annots[1].annot);
      for (int z2 = 2; z2 < n->nannots; z2++) {
	merged_node = merge_annots(merged_node, n->annots[z2].annot);
      }
      unregister_node(n);	// we change its children (reregister...)
      an = n->get_children();
      for (int z3 = 0; z3 < n->get_no_of_kids(); an++, z3++) {
	delete_branch(an->child);
      }
      delete [] n->get_children();
      an = new arc_node[1];
      an->letter = ANNOT_SEPARATOR;
      an->child = merged_node;
      an->weight = sum1;
      an->is_final = FALSE;
      n->set_children(an, 1);
      return 1;
    }
  }
#endif
  if (n->get_no_of_kids() < MIN_KIDS_TO_MERGE) {
    // This node cannot be generalized, so move downwards
    an = n->get_children();
    for (int j = 0; j < n->get_no_of_kids(); j++, an++) {
      if (an->letter != ANNOT_SEPARATOR) {
	new_gen(an->child, pref);
      }
    }
    return 0;			// too few kids to merge
  }
  // here we should also move downwards
  an = n->get_children();
  for (int r = 0; r < n->get_no_of_kids(); r++, an++) {
    if (an->letter != ANNOT_SEPARATOR) {
      new_gen(an->child, pref);
    }
  }
  return 0;
}//new_gen

/* Name:	add_annots
 * Class:	None
 * Purpose:	Adds another set of annotated nodes to the collection
 *		of such nodes reachable from the current node.
 * Parameters:	big_annots	- (i/o) vector of annotated nodes
 *					for the bigger node;
 *		bno		- (i) number of annotated nodes for the bigger
 *					node;
 *		small_annots	- (i) vector of annotated nodes
 *					for the smaller node;
 *		sno		- (i) number of annotated nodes
 *					for the smaller node;
 *		prefix		- (i) TRUE if prefixes used, FALSE otherwise.
 * Returns:	Number of annotated nodes for this node. Zero indicates that
 *		there are too many annotated nodes for this node, and that
 *		generalization is impossible.
 * Remarks:	There are two modes of operation: with or without prefixes.
 *		The mode without prefixes is in operation when prefix is FALSE.
 *		Adding an annotated node is implemented in two loops.
 *		The first one checks whether an annotated node from
 *		the smaller set is equal to any of the annotated nodes
 *		in the bigger set, or if all annotations of that annotated node
 *		are contained in annotations of any of the nodes in the bigger
 *		set. If so, for weighted automata, the weight of the
 *		corresponding node in the vector is increased, for unweighted
 *		automata, nothing more is done.
 *
 *		In the prefix mode, an annotated node can have many prefixes,
 *		and no prefix at all. Annotations after the prefix, i.e. after
 *		the second annotation separator, are called secondary
 *		annotations here. There are the following possible situations:
 *
 *		1.	The annotated nodes are the same. For weighted
 *			automata, the weight is increased.
 *		2.	The annotated nodes are different, but arcs labeled
 *			with the annotation separator lead to the same nodes
 *			(i.e. the nodes have the same annotations without
 *			prefixes)
 *		2a.	The secondary annotations are the same. The nodes
 *			are considered equal, but because they different in
 *			reality, they are inserted as separate entries.
 *		2b.	The secondary annotations of the samller node are part
 *			of the secondary annotations of the bigger node.
 *			For weighted automata, the weight of the bigger node
 *			is increased.
 *		2c.	The secondary annotations are different, the smaller
 *			node must be inserted.
 *		3.	The annotated nodes are different. The smaller node
 *			must be inserted.
 *
 *		When an annotated node is added, its hit count is increased.
 */
int
add_annots(ann_inf *annotations, const int bno, ann_inf *small_annots,
	   const int sno, const int prefix)
{
  int curr_bno = bno;
  int insertion_point = -1;
  int secondary_created = FALSE;
  node ***second_pointers = NULL;
  int *second_sizes = NULL;
  int place_found = FALSE;
  int sa_match = FALSE;
  int still_something_to_do = TRUE;
  ann_inf curr_ann;
  if (bno == -1 || sno == -1)
    return 0;
  if (prefix && bno != 0 && sno != 0) {
    // Strings in the automaton contain prefixes, i.e. after the first
    // annotation separator, there is a (possibly empty) prefix,
    // then another annotation separator, and then the rest of annotation,
    // here called secondary annotation

    // The first thing to do is the same as without prefixes - just compare
    // addresses
    for (int k = 0; k < sno; k++) {
      curr_ann = small_annots[k];
      place_found = FALSE;
      for (int l = 0; l < curr_bno; l++) {
	still_something_to_do = TRUE;
	if (curr_ann.annot == annotations[l].annot ||
	    contained_in(curr_ann.annot, annotations[l].annot)) {
	  // The node, or a node that has the same annotations and more,
	  // has been found in annotations - nothing to be added
#ifdef WEIGHTED
	  annotations[l].weight += curr_ann.weight;
#endif
	  insertion_point = -1;
	  place_found = TRUE;
	  still_something_to_do = FALSE;
	  break;
	}
	else if (contained_in(annotations[l].annot, curr_ann.annot)) {
	  // Replace annotated node in annotations with a more general one
	  // from small_annots
#ifdef WEIGHTED
	  curr_ann.weight += annotations[l].weight;
#endif
	  delete_old_branch(annotations[l].annot); // one incoming arc less
	  annotations[l] = curr_ann;
	  insertion_point = -1;
	  place_found = TRUE;
	  still_something_to_do = FALSE;
	  break;
	}
	else if (curr_ann.annot > annotations[l].annot && !place_found) {
	  insertion_point = l;
	  place_found = TRUE;
	}
      }//for l
      if (!place_found)
	insertion_point = curr_bno;
      if (insertion_point == -1 && still_something_to_do) {
	// Simple comparison of addresses did not work.
	// Here comes the hard stuff. We have to compare secondary annotations
	// of each pair of nodes, one from annotations, another one from
	// small_annots.
	// If the sets of secondary annotations are the same, or one is
	// a subset of another one, then two annotated nodes are merged,
	// and the merged node is put into annotations. Note that the fact
	// that one set of secondary annotations is a subset
	// of another set of secondary annotations does not mean
	// that contained_in() returns TRUE for their annotated nodes.

	// First we have to construct the sets of secondary annotations
	// There cannot be more secondary annotations than the weight
	// of the node.
#ifdef WEIGHTED
	node **second_annots1 = new node *[curr_ann.weight];
#else //!WEIGHTED
	node **second_annots1 = new node *[1024];
#endif //!WEIGHTED
	int second1 = collect_secondary(curr_ann.annot, second_annots1, 0);

	if (!secondary_created) {
	  // Now for each annotated state in annots create a vector
	  // of secondary annotations. This should speed the processing,
	  // because otherwise each such vector would have to be recreated
	  // for each annotated node in small_annots.
	  second_pointers = new node **[curr_bno];
	  second_sizes = new int[curr_bno];
	  for (int m = 0; m < curr_bno; m++) {
#ifdef WEIGHTED
	    second_pointers[m] = new node *[annotations[m].weight];
#else //!WEIGHTED
	    second_pointers[m] = new node *[1024];
#endif //!WEIGHTED
	    second_sizes[m] = collect_secondary(annotations[m].annot,
						second_pointers[m], 0);
	  }
	  secondary_created = TRUE;
	}

	// Now compare secondary annotations one by one
	// For every annotated node in 'annotations'
	for (int n = 0; n < curr_bno; n++) {
	  sa_match = FALSE;
	  // For every secondary annotation for current annotated node
	  // in small_annots (i.e. in curr_ann)
	  for (int p = 0; p < second1; p++) {
	    sa_match = FALSE;
	    // For every secondary annotation of the nth node in annotations
	    for (int r = 0; r < second_sizes[n]; r++) {
	      if (second_annots1[p] == second_pointers[n][r]) {
		sa_match = TRUE;
		break;
	      }
	    }//for r
	    if (!sa_match)
	      break;
	  }//for p
	  if (sa_match) {
	    insertion_point = n;
	    break;
	  }
	}//for n
	if (sa_match) {
	  // annotated nodes can be merged
#ifdef WEIGHTED
	  annotations[insertion_point].weight =
	    curr_ann.weight + annotations[insertion_point].weight;
#endif //!WEIGHTED
	  annotations[insertion_point].annot =
	    merge_annots(curr_ann.annot, annotations[insertion_point].annot);
	  insertion_point = -1;	// not to insert the smaller annotation
	}
	delete [] second_annots1;
      }//if insertion_point == -1
      if (insertion_point != -1) {
	// Annotated node from small_annots did not found in annots.
	// Insert it there if there is enough place
	if (++curr_bno < MAX_DIFF_ANNOTS) {
	  // There is still a place for the annotation
	  for (int m = curr_bno; m > insertion_point; --m)
	    memmove(annotations + m, annotations + m - 1, sizeof(ann_inf));
	  annotations[insertion_point] = curr_ann;
	  curr_ann.annot->hit_node();
	}
	else {
	  // Too many annotated nodes
	  delete [] second_pointers;
	  delete [] second_sizes;
	  return 0;
	}
      }//if insertion_point != -1
    }//for k
    if (secondary_created) {
      delete [] second_pointers;
      delete [] second_sizes;
    }
    return curr_bno;
  }
  else {
    // No prefixes, just compare addresses
    for (int i = 0; i < sno; i++) {
      curr_ann = small_annots[i];
      place_found = FALSE;
      if (curr_bno == 0) {
	place_found = TRUE;
	insertion_point = 0;
      }
      bool current_already_replaced = FALSE;
      for (int j = 0; j < curr_bno; j++) {
	if (curr_ann.annot == annotations[j].annot ||
	    contained_in(curr_ann.annot, annotations[j].annot)) {
	  // The node, or a node that has the same annotations and more,
	  // has been found in annotations - nothing to be added
#ifdef WEIGHTED
	  annotations[j].weight += curr_ann.weight;
#endif //WEIGHTED
	  insertion_point = -1;
	  place_found = TRUE;
	  break;
	}
	else if (contained_in(annotations[j].annot, curr_ann.annot)) {
	  // current arcs has more rich annotations than that in annotations[j]
#ifdef WEIGHTED
	  curr_ann.weight += annotations[j].weight;
#endif //!WEIGHTED
	  // Should we hit current annotation?!!! Yes!
	  curr_ann.annot->hit_node();
	  delete_old_branch(annotations[j].annot); // one incoming arc less
	  if (current_already_replaced) {
	    // the new annotation is already included, simply delete the old one
	    if (j != curr_bno - 1) {
	      memmove(annotations + j, annotations + j + 1,
		      (curr_bno - j - 1) * sizeof(ann_inf));
	    }
	  }
	  else {
	    // replace old annotation with the new one that contains it
	    annotations[j] = curr_ann;
	  }
	  current_already_replaced = TRUE;
	  insertion_point = -1;
	  place_found = TRUE;
	  // We replaced an existing annotation by curr_ann
	  // This means that it may be misplaced
	  // We don't care, as moving them to the correct place costs much
	}
	else if (curr_ann.annot > annotations[j].annot && !place_found) {
	  // new annotation so far, remember potential insertion point
	  insertion_point = j;
	  place_found = TRUE;
	  // no break, so go on with comparison
	}
      }//for j
      if (!place_found) {
	insertion_point = curr_bno;
      }
      // Let us sum up what we have done so far.
      // 1. The bigger collection is empty:
      //    place_found == TRUE,
      //    insertion_point == 0
      // 2. Current annotation is contained in annotations[j]
      //    place_found == TRUE
      //    insertion_point == -1
      // 3. Current annotation contains annotations[j]
      //    place_found == TRUE
      //    insertion_point == -1
      //    (note that the current annotaion has replaced annotations[j],
      //     and delete_old_branch called for the old contents)
      // 4. Current annotation neither is contained in annotations[j]
      //    nor contains it, and it is not smaller than all entries
      //    in the bigger collection
      //    place_found == TRUE
      //    insertion_point == where to insert the current annotation
      // 5. Current annotation neither is contained in annotations[j]
      //    nor contains it, and it is smaller than all entries
      //    in the bigger collection
      //    place_found == FALSE
      //    insertion_point == curr_bno (at the end of annotations!)
      if (insertion_point != -1) {
	// annotated node not found in annotations
	if (++curr_bno < MAX_DIFF_ANNOTS) {
	  // There is still a place for the annotation
	  // Move "bigger" annotations to make place for curr_ann
	  memmove(annotations + insertion_point + 1,
		  annotations + insertion_point,
		  (curr_bno - insertion_point - 1) * sizeof(ann_inf));
	  // Insert the current annotation at insertion point
	  annotations[insertion_point] = curr_ann;
	  curr_ann.annot->hit_node();
	}
	else {
	  // Too many annotated nodes
	  return 0;
	}
      }
    }//for i
    return curr_bno;
  }//if no prefixes
}//add_annots

/* Name:	collect_secondary
 * Class:	None.
 * Purpose:	Puts secondary annotations into a vector.
 * Parameters:	n	- (i) where to begin;
 *		v	- (i/o) the vector to be filled; 
 *		m	- (i) number of already filled positions in the vector.
 * Returns:	Number of positions filled.
 * Remarks:	The vector is allocated by the calling procedure. It is assumed
 *		that there is enough space for all secondary annotations.
 */
int
collect_secondary(const node *n, node *v[], const int m)
{
  int		vn = m;
  int		vp = 0;
  arc_node	*an = n->get_children();
  for (int i = 0; i < n->get_no_of_kids(); i++, an++) {
    if (an->letter == ANNOT_SEPARATOR) {
      for (vp = 0; vp < vn; vp++)
	if (v[vp] == an->child)
	  break;
      v[vp] = an->child;
      if (vp == vn)
	vn++;
    }
    else {
      vn = collect_secondary(an->child, v, vn);
    }
  }//for i
  return vn;
}//collect_secondary

/* Name:	remove_annot_pointers
 * Class:	None.
 * Purpose:	Removes annots fields from nodes, and updates hit counts
 *		of nodes they point to.
 * Parameters:	n	- (i) node to be examined.
 * Returns:	Nothing.
 * Remarks:	This is necessary for optimization that relies on hit count.
 */
void
remove_annot_pointers(node *n)
{
  if (n->annots == NULL) {
    return;
  }
  for (int i = 0; i < n->nannots; i++) {
    delete_old_branch(n->annots[i].annot);
  }
  delete [] n->annots;
  n->annots = NULL;
  arc_node *an = n->get_children();
  for (int j = 0; j < n->get_no_of_kids(); j++, an++) {
    if (an->letter != ANNOT_SEPARATOR) {
      remove_annot_pointers(an->child);
    }
  }
}//remove_annot_pointers
#endif //GENERALIZE

/***	EOF mkindex.cc	***/
