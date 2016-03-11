// fichier misymb.c - gestion des symboles
/* la notice de copyright est légalement en anglais */

// (C) 2016 Basile Starynkevitch
//   this file misymb.c is part of Minil
//   Minil is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Minil is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Minil.  If not, see <http://www.gnu.org/licenses/>.

#include "minil.h"

enum Mi_CouleurRadical_en
{
  crad__rien,
  crad_noir,
  crad_rouge
};

#define MI_RAD_NMAGIQ ((uint16_t)0x5fe7)	/*24551 */
struct MiSt_Radical_st
{
  uint16_t urad_nmagiq;		/* toujours MI_RAD_NMAGIQ */
  enum Mi_CouleurRadical_en urad_couleur;
  const Mit_Chaine *urad_nom;
  struct MiSt_Radical_st *urad_gauche;
  struct MiSt_Radical_st *urad_droit;
  Mit_Symbole *urad_symbprim;
  unsigned urad_tailsec;
  unsigned urad_nbsec; /* nombre de symboles secondaires, en hash table */
  Mit_Symbole**urad_tabsecsym;	/* de taille urad_tailsec */
};

static struct MiSt_Radical_st *mi_radical_racine;

// tester si une valeur chaine est licite pour un nom
bool
mi_nom_licite (const Mit_Chaine *nom)
{
  if (!nom || nom->mi_type != MiTy_Chaine)
    return false;
  return mi_nom_licite_chaine (nom->mi_car);
}				// fin mi_nom_licite

// tester si une chaine C est licite
bool
mi_nom_licite_chaine (const char *ch)
{
  if (!ch)
    return false;
  if (!isalpha (ch[0]))
    return false;
  for (const char *pc = ch + 1; *pc; pc++)
    if (!isalnum (*pc))
      return false;
  return true;
}				// fin mi_nom_licite_chaine

// calcul du hash d'une chaine, généralement non nul
unsigned
mi_hashage_chaine (const char *ch)
{
  unsigned h1 = 0, h2 = 0;
  unsigned rk = 0;
  if (!ch || !ch[0])
    return 0;
  /// lest constantes multiplicatives sont des nombres premiers
  for (const char *pc = ch; *pc; pc++, rk++)
    {
      if (rk % 2)
        h1 = (h1 * 463) ^ (((unsigned char *) pc)[0] * 2027 + 5 * rk);
      else
        h2 = (h2 * 1021 + 13 * rk) ^ (((unsigned char *) pc)[0] * 1049);
    }
  unsigned h = (5 * h1) ^ (17 * h2);
  if (!h)
    h = (h1 % 10039) + (h2 % 20047) + (rk % 30059) + 7;
  return h;
}				// fin mi_hashage_chaine

const char*mi_symbole_chaine(const Mit_Symbole*sy)
{
  if (!sy || sy->mi_type != MiTy_Symbole)return NULL;
  const struct MiSt_Radical_st *rad = sy->mi_radical;
  assert (rad && rad->urad_nmagiq == MI_RAD_NMAGIQ);
  const Mit_Chaine*nm = rad->urad_nom;
  assert (nm && nm->mi_type == MiTy_Chaine);
  return nm->mi_car;
} /* fin mi_symbole_chaine */

/// arbre rouge-noir pour les radicaux
/// voir https://en.wikipedia.org/wiki/Red%E2%80%93black_tree
/// et https://fr.wikipedia.org/wiki/Arbre_bicolore
/// code inspiré par https://github.com/sebastiencs/red-black-tree

struct MiSt_Radical_st *
mi_trouver_radical (const Mit_Chaine *chn)
{
  if (!chn || chn->mi_type != MiTy_Chaine)
    return NULL;
  if (!mi_nom_licite (chn))
    return NULL;
  struct MiSt_Radical_st *rad = mi_radical_racine;
  while (rad)
    {
      assert (rad->urad_nmagiq == MI_RAD_NMAGIQ);
      assert (rad->urad_nom && rad->urad_nom->mi_type == MiTy_Chaine);
      int cmp = (rad->urad_nom == chn) ? 0
                : strcmp (rad->urad_nom->mi_car, chn->mi_car);
      if (!cmp)
        return rad;
      if (cmp < 0)
        rad = rad->urad_gauche;
      else
        rad = rad->urad_droit;
    }
  return NULL;
}				/* fin mi_trouver_radical */

struct MiSt_Radical_st *
mi_trouver_radical_chaine (const char *ch)
{
  if (!ch || !mi_nom_licite_chaine (ch))
    return NULL;

  struct MiSt_Radical_st *rad = mi_radical_racine;
  while (rad)
    {
      assert (rad->urad_nmagiq == MI_RAD_NMAGIQ);
      assert (rad->urad_nom && rad->urad_nom->mi_type == MiTy_Chaine);
      int cmp = strcmp (rad->urad_nom->mi_car, ch);
      if (!cmp)
        return rad;
      if (cmp < 0)
        rad = rad->urad_gauche;
      else
        rad = rad->urad_droit;
    }
  return NULL;
}				/* fin mi_trouver_radical_chaine */

static inline bool
mi_radical_rouge (const struct MiSt_Radical_st *rad)
{
  if (!rad)
    return false;
  assert (rad->urad_nmagiq == MI_RAD_NMAGIQ);
  assert (rad->urad_couleur != crad__rien);
  return rad->urad_couleur == crad_rouge;
}				/* fin mi_radical_rouge */

static inline enum Mi_CouleurRadical_en
mi_couleur_opposee (enum Mi_CouleurRadical_en c)
{
  assert (c != crad__rien);
  return (c == crad_rouge) ? crad_noir : crad_rouge;
}				/* fin mi_couleur_opposee */

static void
mi_radical_changer_couleur (struct MiSt_Radical_st *rad)
{
  assert (rad && rad->urad_nmagiq == MI_RAD_NMAGIQ);
  assert (rad->urad_gauche && rad->urad_gauche->urad_nmagiq == MI_RAD_NMAGIQ);
  assert (rad->urad_droit && rad->urad_droit->urad_nmagiq == MI_RAD_NMAGIQ);
  rad->urad_gauche->urad_couleur =
    mi_couleur_opposee (rad->urad_gauche->urad_couleur);
  rad->urad_droit->urad_couleur =
    mi_couleur_opposee (rad->urad_droit->urad_couleur);
}				/* fin mi_radical_changer_couleur */


static struct MiSt_Radical_st *
mi_radical_tourner_gauche (struct MiSt_Radical_st *radg)
{
  struct MiSt_Radical_st *radroit;
  if (!radg)
    return NULL;
  assert (radg && radg->urad_nmagiq == MI_RAD_NMAGIQ);
  radroit = radg->urad_gauche;
  assert (radroit && radroit->urad_nmagiq == MI_RAD_NMAGIQ);
  radg->urad_droit = radroit->urad_gauche;
  radroit->urad_gauche = radg;
  radroit->urad_couleur = radg->urad_couleur;
  radg->urad_couleur = crad_rouge;
  return radroit;
}				/* fin mi_radical_tourner_gauche */


static struct MiSt_Radical_st *
mi_radical_tourner_droit (struct MiSt_Radical_st *radr)
{
  struct MiSt_Radical_st *radgau = NULL;
  if (!radr)
    return NULL;
  assert (radr && radr->urad_nmagiq == MI_RAD_NMAGIQ);
  radgau = radr->urad_gauche;
  assert (radgau && radgau->urad_nmagiq == MI_RAD_NMAGIQ);
  radr->urad_gauche = radgau->urad_droit;
  radgau->urad_droit = radr;
  radgau->urad_couleur = radr->urad_couleur;
  radr->urad_couleur = crad_rouge;
  return radgau;
}				/* fin mi_radical_tourner_droit */


static struct MiSt_Radical_st *
mi_creer_radical (const Mit_Chaine *ch)
{
  assert (ch != NULL && ch->mi_type == MiTy_Chaine);
  assert (mi_nom_licite (ch));
  struct MiSt_Radical_st *rad = calloc (1, sizeof (struct MiSt_Radical_st));
  if (!rad)
    {
      perror ("mi_creer_radical");
      exit (EXIT_FAILURE);
    };
  rad->urad_nmagiq = MI_RAD_NMAGIQ;
  rad->urad_couleur = crad_rouge;
  rad->urad_nom = ch;
  rad->urad_gauche = NULL;
  rad->urad_droit = NULL;
  return rad;
}				/* fin mi_creer_radical */

static struct MiSt_Radical_st *
mi_inserer_ce_radical_nom (struct MiSt_Radical_st *rad, const Mit_Chaine *ch)
{
  if (!rad)
    return mi_creer_radical (ch);
  assert (mi_nom_licite (ch));
  assert (rad && rad->urad_nmagiq == MI_RAD_NMAGIQ);
  int cmp = strcmp (ch->mi_car, rad->urad_nom->mi_car);
  if (cmp == 0)
    return rad;
  if (cmp < 0)
    rad->urad_gauche = mi_inserer_ce_radical_nom (rad->urad_gauche, ch);
  else
    rad->urad_droit = mi_inserer_ce_radical_nom (rad->urad_droit, ch);
  if (mi_radical_rouge (rad->urad_droit)
      && !mi_radical_rouge (rad->urad_gauche))
    rad = mi_radical_tourner_gauche (rad);
  if (mi_radical_rouge (rad->urad_gauche)
      && !mi_radical_rouge (rad->urad_gauche->urad_gauche))
    rad = mi_radical_tourner_droit (rad);
  if (mi_radical_rouge (rad->urad_droit)
      && mi_radical_rouge (rad->urad_gauche))
    mi_radical_changer_couleur (rad);
  return rad;
}				/* end of mi_inserer_ce_radical_nom */


static struct MiSt_Radical_st *
mi_inserer_ce_radical_chaine (struct MiSt_Radical_st *rad, const char*ch)
{
  assert (ch != NULL);
  if (!rad)
    {
      assert (mi_nom_licite_chaine (ch));
      return mi_creer_radical (mi_creer_chaine(ch));
    }
  assert (rad && rad->urad_nmagiq == MI_RAD_NMAGIQ);
  int cmp = strcmp (ch, rad->urad_nom->mi_car);
  if (cmp == 0)
    return rad;
  if (cmp < 0)
    rad->urad_gauche = mi_inserer_ce_radical_chaine (rad->urad_gauche, ch);
  else
    rad->urad_droit = mi_inserer_ce_radical_chaine (rad->urad_droit, ch);
  if (mi_radical_rouge (rad->urad_droit)
      && !mi_radical_rouge (rad->urad_gauche))
    rad = mi_radical_tourner_gauche (rad);
  if (mi_radical_rouge (rad->urad_gauche)
      && !mi_radical_rouge (rad->urad_gauche->urad_gauche))
    rad = mi_radical_tourner_droit (rad);
  if (mi_radical_rouge (rad->urad_droit)
      && mi_radical_rouge (rad->urad_gauche))
    mi_radical_changer_couleur (rad);
  return rad;
}				/* end of mi_inserer_ce_radical_chaine */


static int
mi_indice_radical_symbole_secondaire(struct MiSt_Radical_st*rad, unsigned ind);

Mit_Symbole *mi_trouver_symbole_nom (const Mit_Chaine *nom, unsigned ind)
{
  struct MiSt_Radical_st *rad =   mi_trouver_radical (nom);
  if (!rad) return NULL;
  if (ind == 0) return rad->urad_symbprim;
  if (rad->urad_nbsec == 0)
    return NULL;
  int pos = mi_indice_radical_symbole_secondaire(rad,ind);
  assert (pos >= 0 && pos< (int)rad->urad_tailsec);
  Mit_Symbole*sy = rad->urad_tabsecsym[pos];
  if (sy && sy != MI_TROU_SYMBOLE)
    {
      assert(sy->mi_type == MiTy_Symbole);
      return sy;
    }
  return NULL;
} /* fin de mi_trouver_symbole_nom */


Mit_Symbole *
mi_trouver_symbole (const char *ch, const char **pfin)
{
  if (pfin)
    *pfin = NULL;
  if (!ch || !isascii (ch[0]))
    return NULL;
  const char *pe = strchr (ch, '_');
  if (!pe)
    {
      Mit_Symbole *sy = mi_trouver_symbole_chaine (ch, 0);
      if (pfin)
        *pfin = ch + strlen (ch);
      return sy;
    }
  // si ça rentre utiliser un tampon sur la pile
  char *fin = NULL;
  unsigned ind = strtol (pe + 1, &fin, 10);
  if (fin == pe + 1)
    return NULL;
  char tamp[80];
  char *chn =
    (pe - ch < (int) sizeof (tamp) - 3) ? tamp : malloc (2 + pe - ch);
  if (!chn)
    MI_FATALPRINTF ("pas de place pour tampon de %d octets (%s)",
                    (int) (pe - ch), strerror (errno));
  memset (chn, 0, 2 + pe - ch);
  strncpy (chn, ch, pe - ch);
  Mit_Symbole *sy = mi_trouver_symbole_chaine (chn, ind);
  if (chn != tamp)
    free (chn);
  if (sy && pfin)
    *pfin = fin;
  return sy;
}				/* fin mi_trouver_symbole */



static unsigned
mi_hashage_symbole_indice (const Mit_Chaine *nom, unsigned ind)
{
  assert (nom && nom->mi_type == MiTy_Chaine && nom->mi_hash > 0);
  unsigned h = nom->mi_hash ^ ind;
  if (!h)
    h = (nom->mi_hash % 1200697) + (ind % 1500827) + 3;
  assert (h != 0);
  return h;
} /* fin mi_hashage_symbole_indice */

unsigned
mi_hashage_nom_indice(const char*nom, unsigned ind)
{
  if (!mi_nom_licite_chaine (nom))
    return 0;
  unsigned h = mi_hashage_chaine(nom) ^ ind;
  if (!h) h = mi_hashage_chaine(nom)  % 1200697 + (ind % 1500827) + 3;
  assert (h != 0);
  return h;
}

int
mi_cmp_symbole (const Mit_Symbole *sy1, const Mit_Symbole *sy2)
{
  if (sy1 == sy2)
    return 0;
  if (!sy1 || sy1->mi_type != MiTy_Symbole)
    return -1;
  if (!sy2 || sy2->mi_type != MiTy_Symbole)
    return 1;
  const struct MiSt_Radical_st*rad1 = sy1->mi_radical;
  const struct MiSt_Radical_st*rad2 = sy2->mi_radical;
  assert (rad1 && rad1->urad_nmagiq == MI_RAD_NMAGIQ);
  assert (rad2 && rad2->urad_nmagiq == MI_RAD_NMAGIQ);
  const Mit_Chaine *nom1 = rad1->urad_nom;
  const Mit_Chaine *nom2 = rad2->urad_nom;
  assert (nom1 && nom1->mi_type == MiTy_Chaine);
  assert (nom2 && nom2->mi_type == MiTy_Chaine);
  if (rad1 != rad2)
    {
      int cmp = strcmp (nom1->mi_car, nom2->mi_car);
      if (cmp < 0)
        return -1;
      else if (cmp > 0)
        return 1;
    }
  if (sy1->mi_indice < sy2->mi_indice)
    return -1;
  if (sy1->mi_indice > sy2->mi_indice)
    return +1;
  MI_FATALPRINTF
  ("symboles corrompus differents mais de même noms %s et indice %u",
   nom1->mi_car, sy1->mi_indice);
}

int
mi_cmp_symboleptr (const void *p1, const void *p2)
{
  assert (p1 != NULL);
  assert (p2 != NULL);
  return mi_cmp_symbole (*(const Mit_Symbole **) p1,
                         *(const Mit_Symbole **) p2);
}



typedef bool mi_iterradical_sigt(struct MiSt_Radical_st*rad, void*client);
static struct MiSt_Radical_st*
mi_parcourir_radical(struct MiSt_Radical_st*rad, mi_iterradical_sigt*f, void*client)
{
  if (!rad) return NULL;
  assert (rad->urad_nmagiq == MI_RAD_NMAGIQ);
  struct MiSt_Radical_st*r = NULL;
  r = rad->urad_gauche;
  if (r)
    {
      assert (r->urad_nmagiq == MI_RAD_NMAGIQ);
      r = mi_parcourir_radical(r,f,client);
      if (r) return r;
    }
  if ((*f)(rad,client)) return rad;
  r = rad->urad_droit;
  if (r)
    {
      assert (r->urad_nmagiq == MI_RAD_NMAGIQ);
      r = mi_parcourir_radical(r,f,client);
      if (r) return r;
    }
  return NULL;
}

struct Mi_parcours_radical_primaire_st
{
  mi_itersymb_sigt * prp_f;
  void* prp_client;
};

static bool
mi_parcourir_radical_primaire(struct MiSt_Radical_st*rad, void*client)
{
  struct Mi_parcours_radical_primaire_st*prp = client;
  return (*prp->prp_f)(rad->urad_symbprim,prp->prp_client);
}

/// itérer sur chaque symbole primaire
void
mi_iterer_symbole_primaire (mi_itersymb_sigt * f, void *client)
{
  if (!f)
    return;
  struct Mi_parcours_radical_primaire_st prp= {f,client};
  mi_parcourir_radical(mi_radical_racine, mi_parcourir_radical_primaire, &prp);
}				/* fin mi_iterer_symbole_primaire */


static bool
mi_parcourir_radical_nomme(struct MiSt_Radical_st*rad, void*client)
{
  struct Mi_parcours_radical_primaire_st*prp = client;
  if ((*prp->prp_f)(rad->urad_symbprim,prp->prp_client))
    return true;
  unsigned t = rad->urad_tailsec;
  if (t>0)
    {
      assert (rad->urad_tabsecsym != NULL);
      for (unsigned ix=0; ix<t; ix++)
        {
          Mit_Symbole*sy = rad->urad_tabsecsym[ix];
          if (!sy || sy==MI_TROU_SYMBOLE) continue;
          if ((*prp->prp_f)(sy,prp->prp_client))
            return true;
        }
    }
  return false;
}

/// itérer sur chaque symbole primaire ou secondaire de nom donné
void
mi_iterer_symbole_nomme (const char *ch, mi_itersymb_sigt * f, void *client)
{
  if (!f || !ch)
    return;
  if (!mi_nom_licite_chaine (ch))
    return;
  struct Mi_parcours_radical_primaire_st prp= {f,client};
  mi_parcourir_radical(mi_radical_racine, mi_parcourir_radical_nomme,
                       &prp);
}				// fin mi_iterer_symbole_nomme


static int
mi_indice_radical_symbole_secondaire(struct MiSt_Radical_st*rad, unsigned ind)
{
  assert (rad && rad->urad_nmagiq == MI_RAD_NMAGIQ);
  const Mit_Chaine*nomr = rad->urad_nom;
  assert (nomr && nomr->mi_type == MiTy_Chaine);
  unsigned h = mi_hashage_symbole_indice(nomr,ind);
  if (!rad->urad_tabsecsym)
    {
      assert (!rad->urad_tailsec);
      unsigned ta = 7;
      rad->urad_tabsecsym = calloc(ta, sizeof(Mit_Symbole*));
      if (!rad->urad_tabsecsym)
        MI_FATALPRINTF("impossible d'allouer table de symboles"
                       "secondaires de %d nom %s (%s)",
                       ta, nomr->mi_car, strerror(errno));
      rad->urad_tailsec = ta;
      rad->urad_nbsec = 0;
    }
  unsigned ta = rad->urad_tailsec;
  assert (ta > 2);
  unsigned debix = h % rad->urad_tailsec;
  int pos = -1;
  for (unsigned ix=debix; ix<ta; ix++)
    {
      const Mit_Symbole*sy = rad->urad_tabsecsym[ix];
      if (!sy)
        {
          if (!pos) pos=(int)ix;
          return pos;
        }
      else if (sy==MI_TROU_SYMBOLE)
        {
          if (!pos) pos=(int)ix;
          continue;
        }
      assert (sy->mi_radical == rad);
      if (sy->mi_indice == ind) return (int)ix;
    }
  for (unsigned ix=0; ix<debix; ix++)
    {
      const Mit_Symbole*sy = rad->urad_tabsecsym[ix];
      if (!sy)
        {
          if (!pos) pos=(int)ix;
          return pos;
        }
      else if (sy==MI_TROU_SYMBOLE)
        {
          if (!pos) pos=(int)ix;
          continue;
        }
      assert (sy->mi_radical == rad);
      if (sy->mi_indice == ind) return (int)ix;
    }
  return -1;
} /* fin mi_indice_radical_symbole_secondaire */


void
mi_agrandir_radical_table_secondaire(struct MiSt_Radical_st*rad, unsigned xtra)
{
  assert (rad && rad->urad_nmagiq == MI_RAD_NMAGIQ);
  if (!rad->urad_tabsecsym)
    {
      unsigned ta = mi_nombre_premier_apres(9*xtra/8+5);
      rad->urad_tabsecsym = calloc(ta, sizeof(Mit_Symbole*));
      if (!rad->urad_tabsecsym)
        MI_FATALPRINTF("impossible d'allouer table de symboles secondaire de %d nom %s (%s)",
                       ta, rad->urad_nom->mi_car, strerror(errno));
      rad->urad_tailsec = ta;
      rad->urad_nbsec = 0;
    }
  else if (5*rad->urad_nbsec + 4*xtra > rad->urad_tailsec)
    {
      unsigned ancnb = rad->urad_nbsec;
      unsigned ta = mi_nombre_premier_apres(4*(rad->urad_nbsec + xtra)/3 + rad->urad_nbsec/32 + 5);
      if (ta > rad->urad_tailsec)
        {
          Mit_Symbole**anctab = rad->urad_tabsecsym;
          unsigned anctail = rad->urad_tailsec;
          rad->urad_tabsecsym = calloc(ta, sizeof(Mit_Symbole*));
          if (!rad->urad_tabsecsym)
            MI_FATALPRINTF("impossible d'allouer table de symboles secondaire de %d nom %s (%s)",
                           ta, rad->urad_nom->mi_car, strerror(errno));
          rad->urad_tailsec = ta;
          rad->urad_nbsec = 0;
          for (unsigned ix=0; ix<anctail; ix++)
            {
              Mit_Symbole*ancsy = anctab[ix];
              if (!ancsy || ancsy == MI_TROU_SYMBOLE) continue;
              assert (ancsy->mi_type == MiTy_Symbole && ancsy->mi_radical == rad);
              assert (ancsy->mi_indice > 0);
              int pos = mi_indice_radical_symbole_secondaire(rad,ancsy->mi_indice);
              assert (pos >= 0 && pos<(int)ta && rad->urad_tabsecsym[pos]==NULL);
              rad->urad_tabsecsym[pos] = ancsy;
              rad->urad_nbsec++;
            }
          assert (rad->urad_nbsec == ancnb);
        }
    }
} /* fin mi_agrandir_radical_table_secondaire */


// Creer (ou trouver, s'il existe déjà) un symbole de nom et indice donnés
Mit_Symbole *mi_creer_symbole_nom (const Mit_Chaine *nom, unsigned ind)
{
  if (!mi_nom_licite(nom)) return NULL;
  struct MiSt_Radical_st*rad = mi_trouver_radical(nom);
  if (!rad)
    {
      if (!mi_radical_racine)
        mi_radical_racine = rad = mi_creer_radical(nom);
      else
        rad = mi_inserer_ce_radical_nom(mi_radical_racine, nom);
    }
  assert (rad && rad->urad_nmagiq == MI_RAD_NMAGIQ);
  if (ind)
    {
      if (5*rad->urad_nbsec + 2 >= 4*rad->urad_tailsec)
        mi_agrandir_radical_table_secondaire(rad, 2+rad->urad_nbsec/32);
      int pos = mi_indice_radical_symbole_secondaire(rad,ind);
      assert (pos >= 0 && pos< (int)rad->urad_tailsec);
      Mit_Symbole*ancsy = rad->urad_tabsecsym[pos];
      if (!ancsy || ancsy == MI_TROU_SYMBOLE)
        {
          Mit_Symbole*sy =  mi_allouer_valeur(MiTy_Symbole, sizeof(Mit_Symbole));
          sy->mi_radical = rad;
          sy->mi_indice = ind;
          sy->mi_hash = mi_hashage_symbole_indice(rad->urad_nom, ind);
          rad->urad_tabsecsym[pos] = sy;
          rad->urad_nbsec++;
          return sy;
        }
      else
        {
          assert (ancsy->mi_indice == ind);
          return ancsy;
        }
    }
  else
    {
      Mit_Symbole*sy = rad->urad_symbprim;
      if (!sy)
        {
          sy = mi_allouer_valeur(MiTy_Symbole, sizeof(Mit_Symbole));
          rad->urad_symbprim = sy;
          sy->mi_radical = rad;
          sy->mi_indice = 0;
          sy->mi_hash = mi_hashage_symbole_indice(rad->urad_nom, 0);
        }
      return sy;
    }
} /* fin mi_creer_symbole_nom */


Mit_Symbole *mi_creer_symbole_chaine (const char *ch, unsigned ind)
{
  if (!mi_nom_licite_chaine(ch)) return NULL;
  struct MiSt_Radical_st*rad = mi_trouver_radical_chaine(ch);
  if (!rad)
    {
      if (!mi_radical_racine)
        {
          const Mit_Chaine*nom = mi_creer_chaine(ch);
          mi_radical_racine = rad = mi_creer_radical(nom);
        }
      else
        rad = mi_inserer_ce_radical_chaine(mi_radical_racine, ch);
    }
  assert (rad && rad->urad_nmagiq == MI_RAD_NMAGIQ);
  if (ind)
    {
      if (5*rad->urad_nbsec + 2 >= 4*rad->urad_tailsec)
        mi_agrandir_radical_table_secondaire(rad, 2+rad->urad_nbsec/32);
      int pos = mi_indice_radical_symbole_secondaire(rad,ind);
      assert (pos >= 0 && pos< (int)rad->urad_tailsec);
      Mit_Symbole*ancsy = rad->urad_tabsecsym[pos];
      if (!ancsy || ancsy == MI_TROU_SYMBOLE)
        {
          Mit_Symbole*sy =  mi_allouer_valeur(MiTy_Symbole, sizeof(Mit_Symbole));
          sy->mi_radical = rad;
          sy->mi_indice = ind;
          sy->mi_hash = mi_hashage_symbole_indice(rad->urad_nom, ind);
          rad->urad_tabsecsym[pos] = sy;
          rad->urad_nbsec++;
          return sy;
        }
      else
        {
          assert (ancsy->mi_indice == ind);
          return ancsy;
        }
    }
  else
    {
      Mit_Symbole*sy = rad->urad_symbprim;
      if (!sy)
        {
          sy = mi_allouer_valeur(MiTy_Symbole, sizeof(Mit_Symbole));
          rad->urad_symbprim = sy;
          sy->mi_radical = rad;
          sy->mi_indice = 0;
          sy->mi_hash = mi_hashage_symbole_indice(rad->urad_nom, 0);
        }
      return sy;
    }
} /* fin mi_creer_symbole_chaine */

Mit_Symbole *mi_trouver_symbole_chaine (const char *ch, unsigned ind)
{
  struct MiSt_Radical_st*rad = mi_trouver_radical_chaine(ch);
  if (!rad) return NULL;
  if (ind == 0) return rad->urad_symbprim;
  if (rad->urad_nbsec == 0)
    return NULL;
  int pos = mi_indice_radical_symbole_secondaire(rad,ind);
  assert (pos >= 0 && pos< (int)rad->urad_tailsec);
  Mit_Symbole*sy = rad->urad_tabsecsym[pos];
  if (sy && sy != MI_TROU_SYMBOLE)
    {
      assert(sy->mi_type == MiTy_Symbole);
      return sy;
    }
  return NULL;
} /* fin mi_trouver_symbole_chaine */

Mit_Symbole*
mi_cloner_symbole(const Mit_Symbole*origsy)
{
  if (!origsy || origsy == MI_TROU_SYMBOLE || origsy->mi_type != MiTy_Symbole)
    return NULL;
  struct MiSt_Radical_st*rad = origsy->mi_radical;
  assert (rad && rad->urad_nmagiq == MI_RAD_NMAGIQ);
  if (rad->urad_nbsec == 0)
    {
      unsigned ind = 0;
      while (ind < 10) ind = random();
      return mi_creer_symbole_nom(rad->urad_nom, ind);
    }
  else
    {
      if (5*rad->urad_nbsec + 2 > 4*rad->urad_tailsec)
        mi_agrandir_radical_table_secondaire(rad, rad->urad_nbsec/8+10);
      unsigned ind = 0;
      int pos = -1;
      do
        {
          pos = -1;
          unsigned i = random();
          if (i<10) continue;
          pos = mi_indice_radical_symbole_secondaire(rad,i);
          assert (pos >= 0 && pos< (int)rad->urad_tailsec);
          Mit_Symbole*sy = rad->urad_tabsecsym[pos];
          if (!sy || sy == MI_TROU_SYMBOLE)
            ind = i;
          else if (sy->mi_indice == i)
            continue;
        }
      while(ind==0);
      Mit_Symbole*sy =  mi_allouer_valeur(MiTy_Symbole, sizeof(Mit_Symbole));
      sy->mi_radical = rad;
      sy->mi_indice = ind;
      sy->mi_hash = mi_hashage_symbole_indice(rad->urad_nom, ind);
      rad->urad_tabsecsym[pos] = sy;
      rad->urad_nbsec++;
      return sy;
    }
} /* fin mi_cloner_symbole */


// obtenir la valeur d'un attribut dans un symbole
Mit_Val mi_symbole_attribut(Mit_Symbole*symb, Mit_Symbole*symbat)
{
  if (!symb || symb == MI_TROU_SYMBOLE || symb->mi_type != MiTy_Symbole
      || !symbat || symbat == MI_TROU_SYMBOLE || symbat->mi_type != MiTy_Symbole) return MI_NILV;
  if (symb->mi_attrs == NULL) return MI_NILV;
  struct Mi_trouve_st t = mi_assoc_chercher(symb->mi_attrs, symbat);
  if (t.t_pres) return t.t_val;
  return MI_NILV;
} // fin mi_symbole_attribut

// ... et avec le drapeau présent si trouvé
struct Mi_trouve_st mi_symbole_attribut_present(Mit_Symbole*symb, Mit_Symbole*symbat)
{
  if (!symb || symb == MI_TROU_SYMBOLE || symb->mi_type != MiTy_Symbole
      || !symbat || symbat == MI_TROU_SYMBOLE || symbat->mi_type != MiTy_Symbole) return (struct Mi_trouve_st)
    {
      .t_val = MI_NILV, .t_pres=false
    };
  if (symb->mi_attrs == NULL) return (struct Mi_trouve_st)
    {
      .t_val = MI_NILV, .t_pres=false
    };
  return mi_assoc_chercher(symb->mi_attrs, symbat);
} // fin mi_symbole_attribut_present

// mettre dans un symbole un attribut lié à une valeur
void mi_symbole_mettre_attribut(Mit_Symbole*symb, Mit_Symbole*symbat, Mit_Val val)
{
  if (!symb || symb == MI_TROU_SYMBOLE || symb->mi_type != MiTy_Symbole) return;
  if (!symbat || symbat == MI_TROU_SYMBOLE || symbat->mi_type != MiTy_Symbole) return;
  if (val.miva_ptr == NULL) return;
  symb->mi_attrs = mi_assoc_mettre(symb->mi_attrs,symbat,val);
} // fin mi_symbole_mettre_attribut

void mi_symbole_enlever_attribut(Mit_Symbole*symb, Mit_Symbole*symbat)
{
#warning mi_symbole_enlever_attribut incomplet
} // fin mi_symbole_enlever_attribut

