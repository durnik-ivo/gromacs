#include <string.h>
#include "smalloc.h"
#include "types/qhoprec.h"
#include "types/gmx_qhop_types.h"
#include "types/mdatom.h"
#include "types/topology.h"

extern t_qhoprec *mk_qhoprec(void)
{
  t_qhoprec *qr;

  snew(qr,1);
  memset((void *)qr, 0, sizeof(t_qhoprec)); /* Just in case */
  return (qr);
}  /* mk_qhoprec */

/* Sets the bqhopdonor[] and bqhopacceptor[] arrays in a t_mdatoms. */
static void qhop_atoms2md(t_mdatoms *md, const t_qhoprec *qr)
{
  int i, j;
  t_qhop_atom *a;
  qhop_db *db;

  db = qr->db;

  /* Should probably set the massT array too. */

  for (i=0; i < qr->nr_qhop_atoms; i++)
    {
      a = &(qr->qhop_atoms[i]);

      md->bqhopacceptor[a->atom_id] = (a->state & eQACC) != 0;
      md->bqhopdonor[a->atom_id]    = (a->state & eQDON) != 0;

      for (j=0; j < a->nr_protons; j++)
	{
	  if (db->H_map.H[db->H_map.atomid2H[a->protons[j]]] == 0)
	    {
	      md->massT[a->protons[j]] = 0;
	    }
	}
    }
}

/* Sets the interactions according to the hydrogen exstence map.
 * This requires a finalized t_mdatoms. */
extern void finalize_qhoprec(t_qhoprec *qhoprec, gmx_mtop_t *mtop, gmx_localtop_t *top, t_mdatoms *md, t_commrec *cr)
{
  int i, j, nb, ft, target;
  qhop_db *db;
  t_qhop_residue *qres;

  db = qhoprec->db;

  for (i=0; i < qhoprec->nr_qhop_residues; i++)
    {
      qres = &(qhoprec->qhop_residues[i]);

      /* What flavour of the residue shall we set it to? */
      target = which_subRes(mtop, qhoprec, db, i);
      qres->res = target;
      
      /* Alocate memory for the bonded index. */
      /* This should probably go to qhop_toputil.c */
      snew(qres->bindex.ilist_pos, F_NRE);
      snew(qres->bindex.indexed,   F_NRE);
      for (j=0; j < ebtsNR; j++)
	{
	  ft = db->rb.btype[j];
	  
	  if (ft >= 0)
	    {
	      nb = db->rtp[db->rb.irtp[qres->rtype]].rb[j].nb;
	      qres->bindex.nr[ft] = nb;

	      snew((qres->bindex.ilist_pos[ft]), nb);

	      snew((qres->bindex.indexed[ft]), nb);
	    }
	}
      set_interactions(qhoprec, db, top, md, qres, cr);
    }

  qhop_atoms2md(md, qhoprec);
}
