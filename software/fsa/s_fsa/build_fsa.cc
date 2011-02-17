/***	build_fsa.cc	***/

/*	Copyright (C) Jan Daciuk, 1999-2004	*/

/*

This program builds a final state automaton that recognizes words from
the dictionary that is an argument to the program.

Words are read from the standard input. One word per line is assumed.
Words must be sorted.

The resulting automaton is written on standard output. It is in a binary
form: a table of structures corresponding to arcs, with each arc containing
a label, number of arcs that lead from the node the arc points to, and
the index of the first arc that leads from the node the given arc points to.


*/


#include	<iostream>
#include	<fstream>
#include	<stddef.h>
#include	<string.h>
#include	<stdlib.h>
#include	<new>
#include	<unistd.h>
#include	"fsa.h"
#include	"nnode.h"
#include	"nstr.h"
#include	"nindex.h"
#include	"build_fsa.h"

const	int	WORD_BUFFER_LENGTH = 128;

char		ANNOT_SEPARATOR = '+';	/* annotation separator */

#ifdef FLEXIBLE
int		fsa_arc_ptr::gtl = 2;	/* initialization (must be defined) */
int		fsa_arc_ptr::size = 4;	/* the same */
#ifdef NUMBERS
int		fsa_arc_ptr::entryl = 0;	/* the same */
int		fsa_arc_ptr::aunit = 1;	/* not used here (must be defined) */
int		node::node_count = 1;	/* number of nodes in the automaton */
int		node::arc_count = 1;	/* number of arcs in the automaton */
int		node::entryl=0;		/* must be defined somewhere */
int		node::a_size=0;		/* must be defined somewhere */
#endif
#endif
#ifdef WEIGHTED
int		goto_offset = 1;
#endif
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
char *annot_buffer;		// buffer for annotations
long int ab_ptr;		// index in that buffer
#endif //FLEXIBLE&STOPBIT&SPARSE


#ifdef A_TERGO
#include	"mkindex.cc"
#endif //A_TERGO

/* Name:	write_fsa
 * Class:	automaton
 * Purpose:	Writes the automaton in a binary form.
 * Parameters:	outfile		- (o) file to be written;
 *		make_numbers	- (i) whether to assign numbers to entries.
 * Returns:	TRUE if automaton written, FALSE otherwise.
 * Remarks:	Arcs must first be numbered in correct order.
 *		Arc number 0 represents the first arc of a node
 *		with no children :-). This makes it possible to use
 *		index 0 as a NULL pointer in a table of arcs.
 *
 *		The automaton file begins with the signature.
 *		The signature is followed by transitions of variable length.
#ifdef SPARSE
 *		The next part contains sparse vector encoding
 *		transitions in the part of the automaton before annotations.
#endif
*/
int
automaton::write_fsa(ostream &outfile, const int make_numbers)
{
#ifdef FLEXIBLE
  char		bytes[8];	/* output buffer for fsa arc */
  fsa_arc_ptr	*dummy;
#else //!FLEXIBLE
  fsa_arc	output_arc;
#endif //!FLEXIBLE
  node		*meta_root;
  signature	sig_arc;
  int		result;
#ifdef FLEXIBLE
  int		gtl_calculated = FALSE;
  int		gtl;
#endif //FLEXIBLE
#ifdef WEIGHTED
  int		weighted = (root->get_children()->weight ? 1 : 0);
#endif
  int		flag_bits =
#ifdef NEXTBIT
#ifdef STOPBIT
#ifdef TAILS
	     4			// 4 bits from go_to taken by flags
#else //!TAILS
	     3			// 3 bits from go_to taken by flags
#endif //!TAILS
#else //!STOPBIT
	     1			// 1 bit from go_to taken by a flag
#endif //!STOPBIT
#else //!NEXTBIT
#ifdef STOPBIT
#ifdef TAILS
	     3			// 3 bits from go_to taken by flags
#else //!TAILS
	     2			// 2 bits from go_to taken by flags
#endif //!TAILS
#else //!STOPBIT
	     0			// the final flag in the counter
#endif //!STOPBIT
#endif //!NEXTBIT
    ;

#ifdef WEIGHTED
  goto_offset = 1 + weighted;
#endif //WEIGHTED

  meta_root = new node;
  meta_root->add_child(START_CHAR, root);
  meta_root->hit_node();
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
  // Find minimal and maximal label
//  if ((unsigned char)('^') < min_label)
//    min_label = (unsigned char)'^';
//  if ((unsigned char)('^') > max_label)
//    max_label = (unsigned char)'^';
//  for (int rc = 0; rc < root->get_no_of_kids(); rc++) {
//    unsigned char rcl = (unsigned char)(root->get_children()[rc].letter);
//    if (rcl < min_label)
//      min_label = rcl;
//    if (rcl > max_label)
//      max_label = rcl;
//  }
//  sparse_vector.set_label_bounds(min_label, max_label);
  add_labels_to_alphabet(meta_root);
  add_labels_to_alphabet(root);
#endif //FLEXIBLE&STOPBIT&SPARSE

  root->hit_node();		// to "register" it
#ifdef FLEXIBLE
#ifdef NUMBERS
  if (make_numbers) {
#ifdef PROGRESS
    cerr << "Calculating numbers" << endl;
#endif //PROGRESS pending: NUMBERS & FLEXIBLE
    number_entries(meta_root);
  }
// pending: FLEXIBLE & NUMBERS
  dummy->entryl = 0;
  gtl = 0;
  int as;
  if (make_numbers) {
    // calculate the length (in bytes) of the number of words in the fsa
    for (int ni = root->get_entries(); ni; ni >>= 8)
      dummy->entryl++;
    // calculate the length (in bytes) of the goto field
    do {
      gtl++;
      as = root->get_node_count() * dummy->entryl
	+ root->get_arc_count() * 
	(gtl + goto_offset);
    } while (as >> (8 * gtl - flag_bits));
  }
  root->set_entryl(dummy->entryl);
  root->set_a_size(goto_offset + gtl);
  if (dummy->entryl)
    // because we have the option NUMBERS, and -N, so the addresses
    // are in bytes; therefore, we must reculaculate the first address
    // (the initial value of no_of_arcs, which used to be 1)
    root->no_of_arcs = gtl + dummy->entryl + goto_offset;
#else //!NUMBERS
#endif //!NUMBERS
#if defined(STOPBIT) && defined(SPARSE)
  meta_root->in_annotations = false;
#endif //SPARSE
#endif //FLEXIBLE
  // No conditionals here
#ifdef PROGRESS
  cerr << "Counting arcs" << endl;
#endif //PROGRESS
#ifdef FLEXIBLE
#if (defined(STOPBIT) && defined(TAILS)) || defined(NEXTBIT)
#ifdef WEIGHTED
  meta_root->number_arcs(0, weighted);	// only count arcs
#else //!WEIGHTED
  meta_root->number_arcs(0);	// only count arcs
#endif //!WEIGHTED
#if defined(SPARSE)
  meta_root->in_annotations = false;
#endif //SPARSE
  // we need to calcuate gtl first, as it is used in numbering arcs
  gtl = 1;
//  cerr << "no_of_arcs = " << meta_root->no_of_arcs << endl;
//  cerr << "tails = " << meta_root->tails << endl;
  for (int r = meta_root->no_of_arcs;(
#ifdef NUMBERS
       (root->get_node_count() * dummy->entryl) +
#endif //NUMBERS
       (r * (goto_offset+gtl)
#ifdef NEXTBIT
	- (meta_root->next_nodes * (gtl - 1))
#endif //NEXTBIT
#if defined(STOPBIT) && defined(TAILS)
	+ (meta_root->tails * gtl)
#endif //STOPBIT&TAILS
	))
	 >> ((8 * gtl) - flag_bits); ) {
//    cerr << "Increasing gtl from " << gtl << " to " << gtl + 1 << endl;
//    cerr << "Expression is " << ((r * (goto_offset+gtl)+ meta_root->tails * gtl) >> ((8 * gtl) -3)) << endl;
    gtl++;
  }
  dummy->gtl = sig_arc.gtl = gtl;
  dummy->size = gtl + goto_offset;
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
  sparse_vector.reset();
#endif
  mark_inner(root, -1);		// pretend arcs were not numbered
				// otherwise number_arcs() would not work again
  root->no_of_arcs = goto_offset + gtl
#ifdef NUMBERS
    + dummy->entryl
#endif //NUMBERS
    ;
  gtl_calculated = TRUE;

#ifdef NUMBERS
  sig_arc.gtl |= (dummy->entryl << 4);
#endif //NUMBERS
#endif //STOPBIT&TAILS || NEXTBIT
#endif //FLEXIBLE

// no conditionals here

#if defined(FLEXIBLE) && ((defined(STOPBIT) && defined(TAILS)) || defined(NEXTBIT))
#ifdef STATISTICS
  meta_root->print_statistics(meta_root);
#endif
#endif
  meta_root->number_arcs(
#if defined(FLEXIBLE) && ((defined(STOPBIT) && defined(TAILS)) || defined(NEXTBIT))
			 gtl
#ifdef WEIGHTED
			 ,
#endif //WEIGHTED
#endif //(FLEXIBLE,((STOPBIT,TAILS)|NEXTBIT))
#ifdef WEIGHTED
			 weighted
#endif //WEIGHTED
			 );	// assign addresses to nodes
#ifdef STATISTICS
#if !(defined(FLEXIBLE) && ((defined(STOPBIT) && defined(TAILS)) || defined(NEXTBIT)))
  meta_root->print_statistics(meta_root);
#endif
#endif



  // write signature (magic number)
#ifdef PROGRESS
  cerr << "Writing the automaton" << endl;
#endif
  sig_arc.sig[0] = '\\';
  sig_arc.sig[1] = 'f';
  sig_arc.sig[2] = 's';
  sig_arc.sig[3] = 'a';
  // No conditionals here
#ifdef FLEXIBLE
#ifdef STOPBIT
#ifdef NEXTBIT
#ifdef TAILS
  sig_arc.ver = 7;
#else //!TAILS
#ifdef WEIGHTED
  if (weighted)
    sig_arc.ver = 8;
  else
    sig_arc.ver = 5;
#else //!WEIGHTED
  sig_arc.ver = 5;
#endif //!WEIGHTED
#endif //!TAILS
#else // !NEXTBIT
#ifdef TAILS
  sig_arc.ver = 6;
#else
  sig_arc.ver = 4;
#endif
#endif // !NEXTBIT
#else // !STOPBIT
#ifdef SPARSE
  sig_arc.ver += 5;
#endif
#ifdef NEXTBIT
  sig_arc.ver = 2;
#else
  sig_arc.ver = 1;
#endif
#endif // STOPBIT pending: FLEXIBLE
#else
  sig_arc.ver = 0;
#ifdef LARGE_DICTIONARIES
  sig_arc.ver += 0x80;
#endif
#endif // FLEXIBLE
  // No conditionals here
  sig_arc.filler = FILLER;
  sig_arc.annot_sep = ANNOT_SEPARATOR;
#ifdef FLEXIBLE
  long int arc_bytes = 0L;
  if (!gtl_calculated) {
    // calculate gtl (length of the go_to field)
    // initially, r is the highest value in the go_to field with flags cleared,
    // but counted as part of the go_to field
    sig_arc.gtl = 0;
    arc_bytes = meta_root->no_of_arcs
#ifdef STOPBIT
#ifdef NEXTBIT
#ifdef TAILS
      * 16			// 4 bits from go_to taken by flags
#else //!TAILS
      * 8			// 3 bits from go_to taken by flags
#endif //!TAILS
#else // !NEXTBIT
#ifdef TAILS
      * 8			// 3 bits from go_to taken by flags
#else //!TAILS
      * 4			// 2 bits from go_to taken by flags
#endif //!TAILS
#endif //!NEXTBIT
#else //!STOPBIT
#ifdef NEXTBIT
      * 2			// 1 bit from go_to taken by a flag
#endif //NEXTBIT
#endif //!STOPBIT
      ;
    for (int r = arc_bytes; r; r >>= 8)
      sig_arc.gtl++;
    gtl = dummy->gtl = sig_arc.gtl;
    dummy->size = dummy->gtl + goto_offset;
#ifdef NUMBERS
    sig_arc.gtl |= (dummy->entryl << 4);
#endif // NUMBERS
  }
#if defined(STOPBIT) && defined(SPARSE)
  sig_arc.ver += 5;
#endif //STOPBIT&SPARSE
#endif // FLEXIBLE
  if (!(outfile.write((char *)&sig_arc, sizeof sig_arc)))
    return FALSE;
#if defined (FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
  // compute sparse gtl (the length of the pointer in the sparse table)
  int sp_gtl = 0;
  for (unsigned int sp_size = sparse_vector.no_of_states() * 2; sp_size > 0;
       sp_gtl++) {
    sp_size >>= 8;
  }
//  sp_gtl = min(dummy->gtl, sp_gtl);
  sp_gtl = max(dummy->gtl, sp_gtl); // the pointer also points to annotations

  // Write the size of the sparse vector (in entries)
  char sbuf[32];
  sbuf[0] = sp_gtl; 		// = max(gtl,sp_gtl);
//  sbuf[1] = min_label;
//  sbuf[2] = max_label;
//  if (!(outfile.write(sbuf, 3)))
//    return FALSE;
  if (!(outfile.write(sbuf, 1)))
    return FALSE;
  int sp_size1 = sparse_vector.size();
  for (int sbi = 0; sbi < sp_gtl + 1; sbi++) { // +1 for max_char-min_char
    sbuf[sbi] = (char)(sp_size1 & 0xff);
    sp_size1 >>= 8;
  }
  if (!(outfile.write(sbuf, sp_gtl + 1))) // +1 for max_char-min_char
    return FALSE;

  // Write the size of annotations (in bytes)
  arc_bytes = sp_size1 = meta_root->no_of_arcs + ((gtl + 1))
#ifdef NUMBERS
    + dummy->entryl
#endif
    ;
//  for (int sbj = 0; sbj < gtl; sbj++) {
//    sbuf[sbj] = (char)(sp_size1 & 0xff);
//    sp_size1 >>= 8;
//  }
//  if (!(outfile.write(sbuf, sp_gtl)))
//    return FALSE;
#endif //FLEXIBLE&STOPBIT&SPARSE

  // write the sink node  
#ifdef FLEXIBLE
  for (int i = 0; i < dummy->size; i++)
    bytes[i] = 0;
#if defined(STOPBIT) && defined(SPARSE)
  meta_root->in_annotations = false;
  annot_buffer = new char[arc_bytes];
  ab_ptr = 0L;
  memcpy(annot_buffer + ab_ptr, bytes, dummy->size);
  ab_ptr += dummy->size;
#ifdef NUMBERS
  if (make_numbers) {
    memcpy(annot_buffer + ab_ptr, bytes, dummy->entryl);
    ab_ptr += dummy->entryl;
  }
#endif //NUMBERS
#else //!(STOPBIT&&SPARSE)
  if (!(outfile.write(bytes, dummy->size)))
    return FALSE;
#ifdef NUMBERS
  if (make_numbers && !(outfile.write(bytes, dummy->entryl)))
    return FALSE;
#endif // NUMBERS
#endif //!(STOPBIT&&SPARSE)
#else // !FLEXIBLE
  output_arc.go_to = 0;
  output_arc.letter = 0;
  output_arc.counter = 0;
  if (!(outfile.write((char *)&output_arc, sizeof output_arc)))
    return FALSE;
#endif // FLEXIBLE

  // write the rest of the automaton
#ifdef WEIGHTED
  result = meta_root->write_arcs(outfile, weighted);
#else //!WEIGHTED
  result = meta_root->write_arcs(outfile);
#endif
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
  // Write the automaton
  // First, write additional information: the size of pointers in the sparse
  // table and the size of that table
  // Something missing?
  // Write the sparse table
#ifdef NUMBERS
  sparse_vector.write(outfile,dummy->entryl,sp_gtl);
#else //!NUMBERS
  sparse_vector.write(outfile,sp_gtl);
#endif //!NUMBERS
  // Write the annotations
  if (!(outfile.write(annot_buffer, ab_ptr)))
    return FALSE;
#endif //FLEXIBLE&STOPBIT&SPARSE
  delete meta_root;
  return result;
}//automaton::write_fsa

#ifdef SUBAUT
/* Name:	number_on_height
 * Class:	automaton
 * Purpose:	Number the nodes in the automaton, so that each node
 *		has a lower number than all nodes that have arcs going
 *		to that node.
 * Parameters:	None.
 * Returns:	Length of the longest path in the automaton.
 * Remarks:	First the nodes that have only arcs with empty children
 *		are put into the table. This creates the first layer.
 *		The second layer is created from those nodes
 *		that have all children already in the table. 
 *		Next layers are created in the same way. To chose the nodes
 *		for the next layer, backreferences from the last layer are
 *		used.
 */
int
automaton::number_on_height(node *layers[])
{
  int up_limit;
  int low_limit;
  int counted;
  int e;
  int height = 0;

  create_back_arcs(root, layers, &up_limit);
  low_limit = 0;
  counted = up_limit;
  for ( int l = 0; l < up_limit; l++)
    layers[l]->entries = l + 1;
  while (counted < root->get_node_count() - 3) {
    // process one more layer
    for (i = low_limit; i < up_limit; i++) {
      // look at parents of the node
      for (j = 0; j < layers[i]->hit_node(0); j++) {
	// look at children of the node
	suitable = TRUE;
	node *p = layers[i]->back_arcs[j];
	if (p->entries == 0) {
	  for (k = 0; k < p->getno_of_kids(); k++) {
	    if ((e = p->get_children()[k].child->entries) == 0 ||
		e  > up_limit) {
	      suitable = FALSE; break;
	    }
	  }
	  if (suitable) {
	    p->entries = counted + 1;
	    layers[counted++] = p; // all children already in the table
	  }
	}//if p->entries
      }//for j
    }//for i
    height++;
    low_limit = up_limit;
    up_limit = counted;
  }//while
  return height;
}//automaton::number_on_height

/* Name:	compute_subautomata
 * Class:	automaton
 * Purpose:	Compute clusters of nodes that share the same structures.
 * Parameters:	None.
 * Returns:	Nothing.
 * Remarks:	The subautomata have one final node (or rather one terminating
 *		node). The final markers on transitions leading to it
 *		are interpreted as a return statement in procedures or
 *		functions.
 */
void
automaton::compute_subautomata(void)
{
  node *layers[root->get_node_count()];
  node *myqueue[root->get_node_count() >> 4];
  node *IP0[root->get_node_count()];
  int  nIP0 = 0;
  node *IP[root->get_node_count() >> 4];
  int  nIP = 0;
  int  qlength = 0;

  height = number_on_height(layers);
  for (subroot = 0; subroot < root->get_node_count(); subroot++) {
    //
    if ((layers[subroot]->flags & ALREADY_TRIED) == 0) {
      node *sr = layers[subroot]
      sr->flags |= ALREADY_TRIED;
      // set IP0 to all nodes not yet tried
      for (int j = subroot + 1; j < root->get_node_count(); j++) {
	if ((layers[j]->flags & ALREADY_TRIED) == 0) {
	  IP0[nIP0++] = layers[j];
	}
      }
      sr->equiv_subn = IP0;
      sr->no_equiv_subn = nIP0;
      nIP0 = 0;
      // set IP to empty set
      nIP = 0;
      // enqueue(subroot)
      if (qlength < (root->get_node_count() >> 4))
	myqueue[qlength++] = sr;
      else {
	cerr << "Queue length in subautomata computations needs increasing"
	     << endl;
	exit(7);		// queue length needs increase
      }
      do {
	// dequeue(p)
	p = myqueue[0]; memmove(myqueue, myqueue + 1,
				((--qlength) * sizeof(node *)));
	for (int i = 0; i < p->back_filled; i++) {
	  // r1 is a node with an arc leading to p
	  node *r1 = p->back_arcs[i];
	  if ((r1->flags & ALREADY_TRIED) == 0) {
	    nIP = 0;
	    for (int k = 0; k < p->no_equiv_subn; k++) {
	      // q is a state equivalent to p
	      // (or not inequivalent in case of p0)
	      node *q = p->equiv_subn[k];
	      if ((q->flags & ALREADY_TRIED) == 0) {
		for (int l = 0; l < q->back_filled; l++) {
		  // r2 is a node with an arc leading to q
		  r2 = q->back_arcs[l];
		  if ((r2->flags & ALREADY_TRIED) &&
		      arcs_match(r1, p, r2, q)) {
		    // r1 and r2 are equivalent with regard to p and q
		    r1->flags |= ALREADY_TRIED;
		    r2->equiv_node = r1;
		    r2->flags |= ALREADY_TRIED;
		    IP[nIP++] = r2;	// add r2 to IP_r1
		    if (p == sr) {
		      if (nIP0 == 0 || IP0[nIP0] != q) {
			IP0[nIP0++] = q;
		      }
		    }//if p=sr
		  }//if arcs match
		}//for r2
	      }//if q not tried before
	    }//for k (q)
	  }
	  if (nIP > 0) {
	    // enqueue(r1)
	    if (qlength < (root->get_node_count() >> 4))
	      myqueue[qlength++] = r1;
	    else {
	      cerr << "Queue length in subautomata computations needs increasing"
		   << endl;
	      exit(7);		// queue length needs increase
	    }
	    r1->flags &= ALREADY_TRIED;
	    // set equivalent nodes
	    r1->equiv_subn = new node *[nIP];
	    memcpy(r1->equiv_subn, IP, nIP * sizeof(node));
	    r1->no_equiv_subn = nIP;
	  }
	}//for r1
      } while (qlength > 0);
      // update the set of equivalent nodes
      delete sr->equiv_subn;
      sr->equiv_subn = new node *[nIP0];
      memcpy(sr->equiv_subn, IP0, nIP0 * sizeof(node *));
      sr->no_equiv_subn = nIP0;
    }//if subroot not tried yet
  }//for all subroots
}//automaton::compute_subautomata
		
#endif

/* Name:	not_enough_memory
 * Class:	None.
 * Purpose:	Inform the user that there is not enough memory to continue
 *		and finish the program.
 * Parameters:	None.
 * Returns:	Nothing.
 * Remarks:	None.
 */
void
not_enough_memory(void)
{
  cerr << "Not enough memory for the automaton\n";
  exit(4);
}//not_enough_memory

/***	EOF build_fsa.cc	***/

