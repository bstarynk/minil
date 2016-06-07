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

// voir aussi https://www.cs.auckland.ac.nz/software/AlgAnim/red_black.html
enum Mi_CouleurRadical_en
{
  crad__rien,
  crad_noir,
  crad_rouge
};

#define MI_RAD_NMAGIQ ((uint16_t)0x5fe7)	/*24551 */
struct MiSt_ValeurRadical_st
{
  Mit_Symbole *vrad_symbprim;
  unsigned vrad_tailsec;
  unsigned vrad_nbsec;
  Mit_Symbole **vrad_tabsecsym;
};

struct MiSt_Radical_st
{
  uint16_t urad_nmagiq;		/* toujours MI_RAD_NMAGIQ */
  enum Mi_CouleurRadical_en urad_couleur;
  struct MiSt_Radical_st *urad_parent;
  struct MiSt_Radical_st *urad_gauche;
  struct MiSt_Radical_st *urad_droit;
  const Mit_Chaine *urad_nom;	// la clef
  struct MiSt_ValeurRadical_st urad_val;
};

static struct MiSt_Radical_st *mi_racine_radical;

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


const char *
mi_symbole_chaine (const Mit_Symbole *sy)
{
  if (!sy || sy->mi_type != MiTy_Symbole)
    return NULL;
  const struct MiSt_Radical_st *rad = sy->mi_radical;
  assert (rad && rad->urad_nmagiq == MI_RAD_NMAGIQ);
  const Mit_Chaine *nm = rad->urad_nom;
  assert (nm && nm->mi_type == MiTy_Chaine);
  return nm->mi_car;
}				/* fin mi_symbole_chaine */


static const char *
mi_radical_chaine (const struct MiSt_Radical_st *rad)
{
  if (!rad)
    return "?nil?";
  if (rad->urad_nmagiq != MI_RAD_NMAGIQ)
    return "?nmagic?";
  const Mit_Chaine *nm = rad->urad_nom;
  if (!nm || nm->mi_type != MiTy_Chaine)
    return "?nom?";
  return nm->mi_car;
}				// fin mi_radical_chaine */

/// arbre rouge-noir pour les radicaux
/// voir https://en.wikipedia.org/wiki/Red%E2%80%93black_tree
/// et https://fr.wikipedia.org/wiki/Arbre_bicolore

struct MiSt_Radical_st *
mi_trouver_radical (const Mit_Chaine *chn)
{
  if (!chn || chn->mi_type != MiTy_Chaine)
    return NULL;
  if (!mi_nom_licite (chn))
    return NULL;
  struct MiSt_Radical_st *rad = mi_racine_radical;
  while (rad)
    {
      assert (rad->urad_nmagiq == MI_RAD_NMAGIQ);
      assert (rad->urad_nom && rad->urad_nom->mi_type == MiTy_Chaine);
      int cmp = (rad->urad_nom == chn) ? 0
                : strcmp (chn->mi_car, rad->urad_nom->mi_car);
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

  struct MiSt_Radical_st *rad = mi_racine_radical;
  while (rad)
    {
      assert (rad->urad_nmagiq == MI_RAD_NMAGIQ);
      assert (rad->urad_nom && rad->urad_nom->mi_type == MiTy_Chaine);
      int cmp = strcmp (ch, rad->urad_nom->mi_car);
      if (!cmp)
        return rad;
      if (cmp < 0)
        rad = rad->urad_gauche;
      else
        rad = rad->urad_droit;
    }
  return NULL;
}				/* fin mi_trouver_radical_chaine */


struct MiSt_Radical_st *
mi_trouver_radical_apres_ou_egal(const char*ch)
{
  if (!ch) return NULL;
  MI_DEBOPRINTF("ch=%s", ch);
  struct MiSt_Radical_st *rad = mi_racine_radical;
  while (rad)
    {
      assert (rad->urad_nmagiq == MI_RAD_NMAGIQ);
      assert (rad->urad_nom && rad->urad_nom->mi_type == MiTy_Chaine);
      int cmp = strcmp (ch, rad->urad_nom->mi_car);
      MI_DEBOPRINTF("ch=%s rad@%p:%s cmp=%d", ch,
                    rad, rad->urad_nom->mi_car, cmp);
      if (cmp == 0) return rad;
      if (cmp < 0)
        {
          if (rad->urad_gauche != NULL)
            {
              rad = rad->urad_gauche;
              continue;
            }
          else
            {
              MI_DEBOPRINTF("ch=%s rad@%p:%s fini", ch, rad, rad->urad_nom->mi_car);
              return rad;
            }
        }
      else   // cmp > 0
        {
          if (rad->urad_droit != NULL)
            {
              rad = rad->urad_droit;
              continue;
            }
          else
            {
              MI_DEBOPRINTF("ch=%s rad@%p:%s feuille", ch, rad, rad->urad_nom->mi_car);
              while (rad->urad_parent != NULL)
                {
                  struct MiSt_Radical_st *parad = rad->urad_parent;
                  int pacmp = strcmp (ch, parad->urad_nom->mi_car);
                  MI_DEBOPRINTF("ch=%s rad@%p:%s parad@%p:%s pacmp=%d",
                                ch,
                                rad,  rad->urad_nom->mi_car,
                                parad,  parad->urad_nom->mi_car, pacmp);
                  if (pacmp<0)
                    {
                      MI_DEBOPRINTF("ch=%s donne parad@%p:%s", ch,
                                    parad,  parad->urad_nom->mi_car);
                      return parad;
                    }
                  rad = parad;
                }
              MI_DEBOPRINTF("ch=%s rad@%p:%s finfeuille", ch, rad, rad->urad_nom->mi_car);

              return rad;
            }
        }
    }
  MI_DEBOPRINTF("ch=%s échec", ch);
  return NULL;
} /* fin mi_trouver_radical_apres_ou_egal */

struct MiSt_Radical_st *
mi_trouver_radical_apres(const char*ch)
{
  if (!ch) return NULL;
  MI_DEBOPRINTF("ch=%s", ch);
  struct MiSt_Radical_st *rad = mi_racine_radical;
  while (rad)
    {
      assert (rad->urad_nmagiq == MI_RAD_NMAGIQ);
      assert (rad->urad_nom && rad->urad_nom->mi_type == MiTy_Chaine);
      int cmp = strcmp (ch, rad->urad_nom->mi_car);
      MI_DEBOPRINTF("ch=%s rad@%p:%s cmp=%d", ch,
                    rad, rad->urad_nom->mi_car, cmp);
      if (cmp < 0)
        {
          if (rad->urad_gauche != NULL)
            {
              rad = rad->urad_gauche;
              continue;
            }
          else
            {
              MI_DEBOPRINTF("ch=%s rad@%p:%s fini", ch, rad, rad->urad_nom->mi_car);
              return rad;
            }
        }
      else   // cmp >= 0
        {
          if (rad->urad_droit != NULL)
            {
              rad = rad->urad_droit;
              continue;
            }
          else
            {
              MI_DEBOPRINTF("ch=%s rad@%p:%s feuille", ch, rad, rad->urad_nom->mi_car);
              while (rad->urad_parent != NULL)
                {
                  struct MiSt_Radical_st *parad = rad->urad_parent;
                  int pacmp = strcmp (ch, parad->urad_nom->mi_car);
                  MI_DEBOPRINTF("ch=%s rad@%p:%s parad@%p:%s pacmp=%d",
                                ch,
                                rad,  rad->urad_nom->mi_car,
                                parad,  parad->urad_nom->mi_car, pacmp);
                  if (pacmp<0)
                    {
                      MI_DEBOPRINTF("ch=%s donne parad@%p:%s", ch,
                                    parad,  parad->urad_nom->mi_car);
                      return parad;
                    }
                  rad = parad;
                }
              MI_DEBOPRINTF("ch=%s rad@%p:%s finfeuille", ch, rad, rad->urad_nom->mi_car);

              return rad;
            }
        }
    }
  MI_DEBOPRINTF("ch=%s échec", ch);
  return NULL;
} /* fin mi_trouver_radical_apres_ou_egal */


struct MiSt_Radical_st *
mi_trouver_radical_avant_ou_egal(const char*ch)
{
  if (!ch) return NULL;
  MI_DEBOPRINTF("ch=%s", ch);
  struct MiSt_Radical_st *rad = mi_racine_radical;
  while (rad)
    {
      assert (rad->urad_nmagiq == MI_RAD_NMAGIQ);
      assert (rad->urad_nom && rad->urad_nom->mi_type == MiTy_Chaine);
      int cmp = strcmp (ch, rad->urad_nom->mi_car);
      MI_DEBOPRINTF("ch=%s rad@%p:%s cmp=%d", ch,
                    rad, rad->urad_nom->mi_car, cmp);
      if (cmp == 0) return rad;
      if (cmp > 0)
        {
          if (rad->urad_droit != NULL)
            {
              rad = rad->urad_droit;
              continue;
            }
          else
            {
              MI_DEBOPRINTF("ch=%s rad@%p:%s fini", ch, rad, rad->urad_nom->mi_car);
              return rad;
            }
        }
      else   // cmp < 0
        {
          if (rad->urad_gauche != NULL)
            {
              rad = rad->urad_gauche;
              continue;
            }
          else
            {
              MI_DEBOPRINTF("ch=%s rad@%p:%s feuille", ch, rad, rad->urad_nom->mi_car);
              while (rad->urad_parent != NULL)
                {
                  struct MiSt_Radical_st *parad = rad->urad_parent;
                  int pacmp = strcmp (ch, parad->urad_nom->mi_car);
                  MI_DEBOPRINTF("ch=%s rad@%p:%s parad@%p:%s pacmp=%d",
                                ch,
                                rad,  rad->urad_nom->mi_car,
                                parad,  parad->urad_nom->mi_car, pacmp);
                  if (pacmp<0)
                    {
                      MI_DEBOPRINTF("ch=%s donne parad@%p:%s", ch,
                                    parad,  parad->urad_nom->mi_car);
                      return parad;
                    }
                  rad = parad;
                }
              MI_DEBOPRINTF("ch=%s rad@%p:%s finfeuille", ch, rad, rad->urad_nom->mi_car);

              return rad;
            }
        }
    }
  MI_DEBOPRINTF("ch=%s échec", ch);
  return NULL;
} /* fin mi_trouver_radical_avant_ou_egal */


struct MiSt_Radical_st *
mi_trouver_radical_avant(const char*ch)
{
  if (!ch) return NULL;
  MI_DEBOPRINTF("ch=%s", ch);
  struct MiSt_Radical_st *rad = mi_racine_radical;
  while (rad)
    {
      assert (rad->urad_nmagiq == MI_RAD_NMAGIQ);
      assert (rad->urad_nom && rad->urad_nom->mi_type == MiTy_Chaine);
      int cmp = strcmp (ch, rad->urad_nom->mi_car);
      MI_DEBOPRINTF("ch=%s rad@%p:%s cmp=%d", ch,
                    rad, rad->urad_nom->mi_car, cmp);
      if (cmp > 0)
        {
          if (rad->urad_droit != NULL)
            {
              rad = rad->urad_droit;
              continue;
            }
          else
            {
              MI_DEBOPRINTF("ch=%s rad@%p:%s fini", ch, rad, rad->urad_nom->mi_car);
              return rad;
            }
        }
      else   // cmp <= 0
        {
          if (rad->urad_gauche != NULL)
            {
              rad = rad->urad_gauche;
              continue;
            }
          else
            {
              MI_DEBOPRINTF("ch=%s rad@%p:%s feuille", ch, rad, rad->urad_nom->mi_car);
              while (rad->urad_parent != NULL)
                {
                  struct MiSt_Radical_st *parad = rad->urad_parent;
                  int pacmp = strcmp (ch, parad->urad_nom->mi_car);
                  MI_DEBOPRINTF("ch=%s rad@%p:%s parad@%p:%s pacmp=%d",
                                ch,
                                rad,  rad->urad_nom->mi_car,
                                parad,  parad->urad_nom->mi_car, pacmp);
                  if (pacmp<0)
                    {
                      MI_DEBOPRINTF("ch=%s donne parad@%p:%s", ch,
                                    parad,  parad->urad_nom->mi_car);
                      return parad;
                    }
                  rad = parad;
                }
              MI_DEBOPRINTF("ch=%s rad@%p:%s finfeuille", ch, rad, rad->urad_nom->mi_car);

              return rad;
            }
        }
    }
  MI_DEBOPRINTF("ch=%s échec", ch);
  return NULL;
} /* fin mi_trouver_radical_avant_ou_egal */



static inline bool
mi_radical_rouge (const struct MiSt_Radical_st *rad)
{
  if (!rad)
    return false;
  assert (rad->urad_nmagiq == MI_RAD_NMAGIQ);
  assert (rad->urad_couleur != crad__rien);
  return rad->urad_couleur == crad_rouge;
}				/* fin mi_radical_rouge */

static inline bool
mi_radical_noir (const struct MiSt_Radical_st *rad)
{
  if (!rad)
    return true;
  assert (rad->urad_nmagiq == MI_RAD_NMAGIQ);
  assert (rad->urad_couleur != crad__rien);
  return rad->urad_couleur == crad_noir;
}				/* fin mi_radical_noir */

static inline enum Mi_CouleurRadical_en
mi_couleur_opposee (enum Mi_CouleurRadical_en c)
{
  assert (c != crad__rien);
  return (c == crad_rouge) ? crad_noir : crad_rouge;
}				/* fin mi_couleur_opposee */


// suivant le livre de Cormen, Leiserson, Rivest, Stein
// Algorithmique (3e edition)
static void
mi_radical_rotation_gauche (struct MiSt_Radical_st *radx)
{
  assert (radx && radx->urad_nmagiq == MI_RAD_NMAGIQ);
  struct MiSt_Radical_st *rady = radx->urad_droit;
  assert (rady && rady->urad_nmagiq == MI_RAD_NMAGIQ);
  radx->urad_droit = rady->urad_gauche;
  if (rady->urad_gauche != NULL)
    rady->urad_gauche->urad_parent = radx;
  rady->urad_parent = radx->urad_parent;
  if (radx->urad_parent == NULL)
    mi_racine_radical = rady;
  else if (radx == radx->urad_parent->urad_gauche)
    radx->urad_parent->urad_gauche = rady;
  else
    radx->urad_parent->urad_droit = rady;
  rady->urad_gauche = radx;
  radx->urad_parent = rady;
}				/* fin mi_radical_rotation_gauche */

static void
mi_radical_rotation_droite (struct MiSt_Radical_st *radx)
{
  assert (radx && radx->urad_nmagiq == MI_RAD_NMAGIQ);
  struct MiSt_Radical_st *rady = radx->urad_gauche;
  assert (rady && rady->urad_nmagiq == MI_RAD_NMAGIQ);
  radx->urad_gauche = rady->urad_droit;
  if (rady->urad_droit != NULL)
    rady->urad_droit->urad_parent = radx;
  rady->urad_parent = radx->urad_parent;
  if (radx->urad_parent == NULL)
    mi_racine_radical = rady;
  else if (radx == radx->urad_parent->urad_droit)
    radx->urad_parent->urad_droit = rady;
  else
    radx->urad_parent->urad_gauche = rady;
  rady->urad_droit = radx;
  radx->urad_parent = rady;
}				/* fin mi_radical_rotation_droite */



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
  rad->urad_parent = NULL;
  rad->urad_gauche = NULL;
  rad->urad_droit = NULL;
  MI_DEBOPRINTF ("rad@%p ch@%p'%s'", rad, ch, ch->mi_car);
  return rad;
}				/* fin mi_creer_radical */


static void mi_correction_apres_insertion (struct MiSt_Radical_st *);

static struct MiSt_Radical_st *
mi_radical_insere_nom (const Mit_Chaine *nomz)
{
  assert (mi_nom_licite (nomz));
  struct MiSt_Radical_st *rady = NULL;
  struct MiSt_Radical_st *radx = mi_racine_radical;
  int cmp = 0;
  MI_DEBOPRINTF ("début nomz@%p:'%s' mi_racine_radical@%p",
                 nomz, nomz->mi_car, mi_racine_radical);
  while (radx != NULL)
    {
      assert (radx->urad_nmagiq == MI_RAD_NMAGIQ);
      assert (radx->urad_nom && radx->urad_nom->mi_type == MiTy_Chaine);
      rady = radx;
      cmp = strcmp (nomz->mi_car, radx->urad_nom->mi_car);
      if (!cmp)
        return radx;
      else if (cmp < 0)
        radx = radx->urad_gauche;
      else
        radx = radx->urad_droit;
    }
  struct MiSt_Radical_st *radz = mi_creer_radical (nomz);
  MI_DEBOPRINTF ("rady@%p'%s' radz@%p'%s' nomz@%p:'%s'",
                 rady, mi_radical_chaine (rady), radz,
                 mi_radical_chaine (radz), nomz, nomz->mi_car);
  assert (radz && radz->urad_nmagiq == MI_RAD_NMAGIQ);
  radz->urad_parent = rady;
  if (rady == NULL)
    mi_racine_radical = radz;
  else if ((cmp = strcmp (nomz->mi_car, rady->urad_nom->mi_car)) < 0)
    rady->urad_gauche = radz;
  else if (cmp > 0)
    rady->urad_droit = radz;
  else				// cas impossible
    {
      assert ("cas impossible mauvais rady");
      MI_FATALPRINTF ("corruption de radical rady@%p", (void *) rady);
    }
  mi_correction_apres_insertion (radz);
  return radz;
}				/* fin mi_radical_insere_nom */


static struct MiSt_Radical_st *
mi_radical_insere_chaine (const char *ch)
{
  MI_DEBOPRINTF ("debut ch='%s' mi_racine_radical@%p", ch, mi_racine_radical);
  if (mi_deboguage)
    mi_afficher_radicaux ("insere_chaine");
  assert (mi_nom_licite_chaine (ch));
  struct MiSt_Radical_st *rady = NULL;
  struct MiSt_Radical_st *radx = mi_racine_radical;
  int cmp = 0;
  while (radx != NULL)
    {
      MI_DEBOPRINTF ("radx@%p '%s'", radx, mi_radical_chaine (radx));
      assert (radx->urad_nmagiq == MI_RAD_NMAGIQ);
      assert (radx->urad_nom && radx->urad_nom->mi_type == MiTy_Chaine);
      rady = radx;
      cmp = strcmp (ch, radx->urad_nom->mi_car);
      if (!cmp)
        {
          MI_DEBOPRINTF ("resultat radx@%p ch=%s", radx, ch);
          return radx;
        }
      else if (cmp < 0)
        radx = radx->urad_gauche;
      else
        radx = radx->urad_droit;
    }
  struct MiSt_Radical_st *radz = mi_creer_radical (mi_creer_chaine (ch));
  MI_DEBOPRINTF ("rady@%p'%s' radz@%p'%s' ch='%s'",
                 rady, mi_radical_chaine (rady),
                 radz, mi_radical_chaine (radz), ch);
  assert (radz && radz->urad_nmagiq == MI_RAD_NMAGIQ);
  radz->urad_parent = rady;
  if (rady == NULL)
    mi_racine_radical = radz;
  else if ((cmp = strcmp (ch, rady->urad_nom->mi_car)) < 0)
    rady->urad_gauche = radz;
  else if (cmp > 0)
    rady->urad_droit = radz;
  else				// cas impossible
    {
      assert ("cas impossible mauvais rady");
      MI_FATALPRINTF ("corruption de radical rady@%p'%s'",
                      (void *) rady, mi_radical_chaine (rady));
    }
  MI_DEBOPRINTF ("avant correction radz@%p'%s'",
                 radz, mi_radical_chaine (radz));
  if (mi_deboguage)
    mi_afficher_radicaux ("insere_ch. avant correction");
  mi_correction_apres_insertion (radz);
  MI_DEBOPRINTF ("apres correction radz@%p'%s'",
                 radz, mi_radical_chaine (radz));
  return radz;
}				/* fin mi_radical_insere_chaine */


static void
mi_correction_apres_insertion (struct MiSt_Radical_st *radz)
{
  MI_DEBOPRINTF ("début radz@%p'%s'", radz, mi_radical_chaine (radz));
  assert (radz && radz->urad_nmagiq == MI_RAD_NMAGIQ);
  struct MiSt_Radical_st *radparz = NULL;
  while ((radparz = radz->urad_parent), mi_radical_rouge (radparz))
    {
      MI_DEBOPRINTF ("radz@%p'%s' radparz@%p'%s'",
                     radz, mi_radical_chaine (radz),
                     radparz, mi_radical_chaine (radparz));
      assert (radparz->urad_nmagiq == MI_RAD_NMAGIQ);
      if (radparz->urad_parent != NULL
          && radparz == radparz->urad_parent->urad_gauche)
        {
          struct MiSt_Radical_st *rady = radparz->urad_parent->urad_droit;
          MI_DEBOPRINTF ("rady@%p'%s'", rady, mi_radical_chaine (rady));
          assert (!rady || rady->urad_nmagiq == MI_RAD_NMAGIQ);
          if (mi_radical_rouge (rady))
            {
              radparz->urad_couleur = crad_noir;
              rady->urad_couleur = crad_noir;
              MI_DEBOPRINTF ("radparz@%p'%s'",
                             radparz, mi_radical_chaine (radparz));
              radparz->urad_parent->urad_couleur = crad_rouge;
              radz = radparz->urad_parent;
            }
          else
            {
              if (radz == radparz->urad_droit)
                {
                  radz = radparz;
                  MI_DEBOPRINTF ("radz@%p'%s' avant rot.gauche",
                                 radz, mi_radical_chaine (radz));
                  mi_radical_rotation_gauche (radz);
                  radparz = radz->urad_parent;
                };
              radparz->urad_couleur = crad_noir;
              assert (radparz->urad_parent);
              radparz->urad_parent->urad_couleur = crad_rouge;
              MI_DEBOPRINTF ("radparz@%p'%s' avant rot.droite parent@%p'%s'",
                             radparz, mi_radical_chaine (radparz),
                             radparz->urad_parent,
                             mi_radical_chaine (radparz->urad_parent));
              mi_radical_rotation_droite (radparz->urad_parent);
            }
        }
      else if (radparz->urad_parent != NULL
               && radparz == radparz->urad_parent->urad_droit)
        {
          struct MiSt_Radical_st *rady = radparz->urad_parent->urad_gauche;
          MI_DEBOPRINTF ("rady@%p'%s'", rady, mi_radical_chaine (rady));
          assert (!rady || rady->urad_nmagiq == MI_RAD_NMAGIQ);
          if (mi_radical_rouge (rady))
            {
              radparz->urad_couleur = crad_noir;
              rady->urad_couleur = crad_noir;
              radparz->urad_parent->urad_couleur = crad_rouge;
              radz = radparz->urad_parent;
              MI_DEBOPRINTF ("radz@%p'%s'", radz, mi_radical_chaine (radz));
            }
          else
            {
              if (radz == radparz->urad_gauche)
                {
                  radz = radparz;
                  MI_DEBOPRINTF ("radz@%p'%s' avant rot.droite",
                                 radz, mi_radical_chaine (radz));
                  mi_radical_rotation_droite (radz);
                  radparz = radz->urad_parent;
                };
              radparz->urad_couleur = crad_noir;
              MI_DEBOPRINTF ("radparz@%p'%s'",
                             radparz, mi_radical_chaine (radparz));
              assert (radparz->urad_parent);
              radparz->urad_parent->urad_couleur = crad_rouge;
              MI_DEBOPRINTF ("avant rot.gauche radparz.parent@%p'%s'",
                             radparz->urad_parent,
                             mi_radical_chaine (radparz->urad_parent));
              mi_radical_rotation_gauche (radparz->urad_parent);
              MI_DEBOPRINTF ("apres rot.gauche radparz.parent@%p'%s'",
                             radparz->urad_parent,
                             mi_radical_chaine (radparz->urad_parent));
            }
        }
      else if (radparz->urad_parent != NULL)	// cas impossible
        {
          assert ("corruption dans insertion");
          MI_FATALPRINTF ("corruption radz@%p radparz@%p",
                          (void *) radz, (void *) radparz);
        }
      else
        {
          MI_DEBOPRINTF ("fin avec radz@%p'%s' radparz@%p'%s'",
                         radz, mi_radical_chaine (radz),
                         radparz, mi_radical_chaine (radparz));
          break;
        }
    }
  if (mi_racine_radical != NULL)
    mi_racine_radical->urad_couleur = crad_noir;
}				/* fin mi_correction_apres_insertion */

static int
mi_indice_radical_symbole_secondaire (struct MiSt_Radical_st *rad,
                                      unsigned ind);

Mit_Symbole *
mi_trouver_symbole_nom (const Mit_Chaine *nom, unsigned ind)
{
  struct MiSt_Radical_st *rad = mi_trouver_radical (nom);
  if (!rad)
    return NULL;
  if (ind == 0)
    return rad->urad_val.vrad_symbprim;
  if (rad->urad_val.vrad_nbsec == 0)
    return NULL;
  int pos = mi_indice_radical_symbole_secondaire (rad, ind);
  assert (pos < (int) rad->urad_val.vrad_tailsec);
  if (pos<0) return NULL;
  Mit_Symbole *sy = rad->urad_val.vrad_tabsecsym[pos];
  if (sy && sy != MI_TROU_SYMBOLE)
    {
      assert (sy->mi_type == MiTy_Symbole);
      return sy;
    }
  return NULL;
}				/* fin de mi_trouver_symbole_nom */


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
}				/* fin mi_hashage_symbole_indice */

unsigned
mi_hashage_nom_indice (const char *nom, unsigned ind)
{
  if (!mi_nom_licite_chaine (nom))
    return 0;
  unsigned h = mi_hashage_chaine (nom) ^ ind;
  if (!h)
    h = mi_hashage_chaine (nom) % 1200697 + (ind % 1500827) + 3;
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
  const struct MiSt_Radical_st *rad1 = sy1->mi_radical;
  const struct MiSt_Radical_st *rad2 = sy2->mi_radical;
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


Mit_Symbole*mi_radical_symbole_primaire(const struct MiSt_Radical_st*rad)
{
  if (!rad) return NULL;
  assert (rad->urad_nmagiq == MI_RAD_NMAGIQ);
  return rad->urad_val.vrad_symbprim;
} /* fin mi_radical_symbole_primaire */

const Mit_Chaine*mi_radical_nom(const struct MiSt_Radical_st*rad)
{
  if (!rad) return NULL;
  assert (rad->urad_nmagiq == MI_RAD_NMAGIQ);
  return rad->urad_nom;
} /* fin mi_radical_nom */

typedef bool mi_iterradical_sigt (struct MiSt_Radical_st *rad, void *client);
static struct MiSt_Radical_st *
mi_parcourir_radical (struct MiSt_Radical_st *rad, mi_iterradical_sigt * f,
                      void *client)
{
  if (!rad)
    return NULL;
  assert (rad->urad_nmagiq == MI_RAD_NMAGIQ);
  struct MiSt_Radical_st *r = NULL;
  r = rad->urad_gauche;
  if (r)
    {
      assert (r->urad_nmagiq == MI_RAD_NMAGIQ);
      r = mi_parcourir_radical (r, f, client);
      if (r)
        return r;
    }
  if ((*f) (rad, client))
    return rad;
  r = rad->urad_droit;
  if (r)
    {
      assert (r->urad_nmagiq == MI_RAD_NMAGIQ);
      r = mi_parcourir_radical (r, f, client);
      if (r)
        return r;
    }
  return NULL;
}

struct Mi_parcours_radical_primaire_st
{
  mi_itersymb_sigt *prp_f;
  void *prp_client;
};

static bool
mi_parcourir_radical_primaire (struct MiSt_Radical_st *rad, void *client)
{
  struct Mi_parcours_radical_primaire_st *prp = client;
  return (*prp->prp_f) (rad->urad_val.vrad_symbprim, prp->prp_client);
}

/// itérer sur chaque symbole primaire
void
mi_iterer_symbole_primaire (mi_itersymb_sigt * f, void *client)
{
  if (!f)
    return;
  struct Mi_parcours_radical_primaire_st prp = { f, client };
  mi_parcourir_radical (mi_racine_radical, mi_parcourir_radical_primaire,
                        &prp);
}				/* fin mi_iterer_symbole_primaire */


void mi_iterer_symbole_radical (const struct MiSt_Radical_st*rad, mi_itersymb_sigt * f,
                                void *client)
{
  if (!rad || !f) return;
  assert (rad->urad_nmagiq == MI_RAD_NMAGIQ);
  if ((*f)(rad->urad_val.vrad_symbprim, client))
    return;
  unsigned t = rad->urad_val.vrad_tailsec;
  if (t > 0)
    {
      assert (rad->urad_val.vrad_tabsecsym != NULL);
      for (unsigned ix = 0; ix < t; ix++)
        {
          Mit_Symbole *sy = rad->urad_val.vrad_tabsecsym[ix];
          if (!sy || sy == MI_TROU_SYMBOLE)
            continue;
          if ((*f) (sy, client))
            return;
        }
    }
} /* fin mi_iterer_symbole_radical */

static bool
mi_parcourir_radical_nomme (struct MiSt_Radical_st *rad, void *client)
{
  struct Mi_parcours_radical_primaire_st *prp = client;
  if ((*prp->prp_f) (rad->urad_val.vrad_symbprim, prp->prp_client))
    return true;
  unsigned t = rad->urad_val.vrad_tailsec;
  if (t > 0)
    {
      assert (rad->urad_val.vrad_tabsecsym != NULL);
      for (unsigned ix = 0; ix < t; ix++)
        {
          Mit_Symbole *sy = rad->urad_val.vrad_tabsecsym[ix];
          if (!sy || sy == MI_TROU_SYMBOLE)
            continue;
          if ((*prp->prp_f) (sy, prp->prp_client))
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
  struct Mi_parcours_radical_primaire_st prp = { f, client };
  mi_parcourir_radical (mi_racine_radical, mi_parcourir_radical_nomme, &prp);
}				// fin mi_iterer_symbole_nomme


static int
mi_indice_radical_symbole_secondaire (struct MiSt_Radical_st *rad,
                                      unsigned ind)
{
  assert (rad && rad->urad_nmagiq == MI_RAD_NMAGIQ);
  const Mit_Chaine *nomr = rad->urad_nom;
  assert (nomr && nomr->mi_type == MiTy_Chaine);
  unsigned h = mi_hashage_symbole_indice (nomr, ind);
  if (!rad->urad_val.vrad_tabsecsym)
    {
      assert (!rad->urad_val.vrad_tailsec);
      unsigned ta = 7;
      rad->urad_val.vrad_tabsecsym = calloc (ta, sizeof (Mit_Symbole *));
      if (!rad->urad_val.vrad_tabsecsym)
        MI_FATALPRINTF ("impossible d'allouer table de symboles"
                        "secondaires de %d nom %s (%s)",
                        ta, nomr->mi_car, strerror (errno));
      rad->urad_val.vrad_tailsec = ta;
      rad->urad_val.vrad_nbsec = 0;
    }
  unsigned ta = rad->urad_val.vrad_tailsec;
  assert (ta > 2);
  unsigned debix = h % rad->urad_val.vrad_tailsec;
  int pos = -1;
  for (unsigned ix = debix; ix < ta; ix++)
    {
      const Mit_Symbole *sy = rad->urad_val.vrad_tabsecsym[ix];
      if (!sy)
        {
          if (pos<0)
            pos = (int) ix;
          return pos;
        }
      else if (sy == MI_TROU_SYMBOLE)
        {
          if (pos<0)
            pos = (int) ix;
          continue;
        }
      assert (sy->mi_radical == rad);
      if (sy->mi_indice == ind)
        return (int) ix;
    }
  for (unsigned ix = 0; ix < debix; ix++)
    {
      const Mit_Symbole *sy = rad->urad_val.vrad_tabsecsym[ix];
      if (!sy)
        {
          if (pos<0)
            pos = (int) ix;
          return pos;
        }
      else if (sy == MI_TROU_SYMBOLE)
        {
          if (pos<0)
            pos = (int) ix;
          continue;
        }
      assert (sy->mi_radical == rad);
      if (sy->mi_indice == ind)
        return (int) ix;
    }
  return pos;
}				/* fin mi_indice_radical_symbole_secondaire */


void
mi_agrandir_radical_table_secondaire (struct MiSt_Radical_st *rad,
                                      unsigned xtra)
{
  assert (rad && rad->urad_nmagiq == MI_RAD_NMAGIQ);
  if (!rad->urad_val.vrad_tabsecsym)
    {
      unsigned ta = mi_nombre_premier_apres (9 * xtra / 8 + 5);
      rad->urad_val.vrad_tabsecsym = calloc (ta, sizeof (Mit_Symbole *));
      if (!rad->urad_val.vrad_tabsecsym)
        MI_FATALPRINTF
        ("impossible d'allouer table de symboles secondaire de %d nom %s (%s)",
         ta, rad->urad_nom->mi_car, strerror (errno));
      rad->urad_val.vrad_tailsec = ta;
      rad->urad_val.vrad_nbsec = 0;
    }
  else if (5 * rad->urad_val.vrad_nbsec + 4 * xtra >
           rad->urad_val.vrad_tailsec)
    {
      unsigned ancnb = rad->urad_val.vrad_nbsec;
      unsigned ta =
        mi_nombre_premier_apres (4 * (rad->urad_val.vrad_nbsec + xtra) / 3 +
                                 rad->urad_val.vrad_nbsec / 32 + 5);
      if (ta > rad->urad_val.vrad_tailsec)
        {
          Mit_Symbole **anctab = rad->urad_val.vrad_tabsecsym;
          unsigned anctail = rad->urad_val.vrad_tailsec;
          rad->urad_val.vrad_tabsecsym = calloc (ta, sizeof (Mit_Symbole *));
          if (!rad->urad_val.vrad_tabsecsym)
            MI_FATALPRINTF
            ("impossible d'allouer table de symboles secondaire de %d nom %s (%s)",
             ta, rad->urad_nom->mi_car, strerror (errno));
          rad->urad_val.vrad_tailsec = ta;
          rad->urad_val.vrad_nbsec = 0;
          for (unsigned ix = 0; ix < anctail; ix++)
            {
              Mit_Symbole *ancsy = anctab[ix];
              if (!ancsy || ancsy == MI_TROU_SYMBOLE)
                continue;
              assert (ancsy->mi_type == MiTy_Symbole
                      && ancsy->mi_radical == rad);
              assert (ancsy->mi_indice > 0);
              int pos =
                mi_indice_radical_symbole_secondaire (rad, ancsy->mi_indice);
              assert (pos >= 0 && pos < (int) ta
                      && rad->urad_val.vrad_tabsecsym[pos] == NULL);
              rad->urad_val.vrad_tabsecsym[pos] = ancsy;
              rad->urad_val.vrad_nbsec++;
            }
          assert (rad->urad_val.vrad_nbsec == ancnb);
        }
    }
}				/* fin mi_agrandir_radical_table_secondaire */

// Créer ou trouver un symbole de radical et indice donnés
Mit_Symbole*
mi_creer_symbole_radical(struct MiSt_Radical_st*rad, unsigned ind)
{
  if (!rad) return NULL;
  assert (rad && rad->urad_nmagiq == MI_RAD_NMAGIQ);
  assert (rad && rad->urad_nmagiq == MI_RAD_NMAGIQ);
  if (ind)
    {
      if (5 * rad->urad_val.vrad_nbsec + 2 >= 4 * rad->urad_val.vrad_tailsec)
        mi_agrandir_radical_table_secondaire (rad,
                                              2 +
                                              rad->urad_val.vrad_nbsec / 32);
      int pos = mi_indice_radical_symbole_secondaire (rad, ind);
      assert (pos >= 0 && pos < (int) rad->urad_val.vrad_tailsec);
      Mit_Symbole *ancsy = rad->urad_val.vrad_tabsecsym[pos];
      if (!ancsy || ancsy == MI_TROU_SYMBOLE)
        {
          Mit_Symbole *sy =
            mi_allouer_valeur (MiTy_Symbole, sizeof (Mit_Symbole));
          sy->mi_radical = rad;
          sy->mi_indice = ind;
          sy->mi_hash = mi_hashage_symbole_indice (rad->urad_nom, ind);
          rad->urad_val.vrad_tabsecsym[pos] = sy;
          rad->urad_val.vrad_nbsec++;
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
      Mit_Symbole *sy = rad->urad_val.vrad_symbprim;
      if (!sy)
        {
          sy = mi_allouer_valeur (MiTy_Symbole, sizeof (Mit_Symbole));
          rad->urad_val.vrad_symbprim = sy;
          sy->mi_radical = rad;
          sy->mi_indice = 0;
          sy->mi_hash = mi_hashage_symbole_indice (rad->urad_nom, 0);
        }
      return sy;
    }
} /* fin mi_creer_symbole_radical */


// Creer (ou trouver, s'il existe déjà) un symbole de nom et indice donnés
Mit_Symbole *
mi_creer_symbole_nom (const Mit_Chaine *nom, unsigned ind)
{
  if (!mi_nom_licite (nom))
    return NULL;
  struct MiSt_Radical_st *rad = mi_radical_insere_nom (nom);
  return mi_creer_symbole_radical(rad,ind);
}				/* fin mi_creer_symbole_nom */


Mit_Symbole *
mi_creer_symbole_chaine (const char *ch, unsigned ind)
{
  if (!mi_nom_licite_chaine (ch))
    return NULL;
  struct MiSt_Radical_st *rad = mi_radical_insere_chaine (ch);
  return mi_creer_symbole_radical(rad,ind);
}				/* fin mi_creer_symbole_chaine */



Mit_Symbole *
mi_trouver_symbole_chaine (const char *ch, unsigned ind)
{
  struct MiSt_Radical_st *rad = mi_trouver_radical_chaine (ch);
  if (!rad)
    return NULL;
  if (ind == 0)
    return rad->urad_val.vrad_symbprim;
  if (rad->urad_val.vrad_nbsec == 0)
    return NULL;
  int pos = mi_indice_radical_symbole_secondaire (rad, ind);
  assert (pos >= 0 && pos < (int) rad->urad_val.vrad_tailsec);
  Mit_Symbole *sy = rad->urad_val.vrad_tabsecsym[pos];
  if (sy && sy != MI_TROU_SYMBOLE)
    {
      assert (sy->mi_type == MiTy_Symbole);
      return sy;
    }
  return NULL;
}				/* fin mi_trouver_symbole_chaine */




Mit_Symbole *
mi_cloner_symbole (const Mit_Symbole *origsy)
{
  if (!origsy || origsy == MI_TROU_SYMBOLE || origsy->mi_type != MiTy_Symbole)
    return NULL;
  struct MiSt_Radical_st *rad = origsy->mi_radical;
  assert (rad && rad->urad_nmagiq == MI_RAD_NMAGIQ);
  if (rad->urad_val.vrad_tailsec == 0)
    mi_agrandir_radical_table_secondaire(rad, 7);
  else if (5 * rad->urad_val.vrad_nbsec + 2 > 4 * rad->urad_val.vrad_tailsec)
    mi_agrandir_radical_table_secondaire (rad,
                                          rad->urad_val.vrad_nbsec / 4 +
                                          10);
  unsigned ind = 0;
  int pos = -1;
  do
    {
      pos = -1;
      unsigned i = random ();
      if (i < 10)
        continue;
      pos = mi_indice_radical_symbole_secondaire (rad, i);
      assert (pos >= 0 && pos < (int) rad->urad_val.vrad_tailsec);
      Mit_Symbole *sy = rad->urad_val.vrad_tabsecsym[pos];
      if (!sy || sy == MI_TROU_SYMBOLE)
        ind = i;
      else if (sy->mi_indice == i)
        continue;
    }
  while (ind == 0);
  Mit_Symbole *sy =
    mi_allouer_valeur (MiTy_Symbole, sizeof (Mit_Symbole));
  sy->mi_radical = rad;
  sy->mi_indice = ind;
  sy->mi_hash = mi_hashage_symbole_indice (rad->urad_nom, ind);
  rad->urad_val.vrad_tabsecsym[pos] = sy;
  rad->urad_val.vrad_nbsec++;
  return sy;
}				/* fin mi_cloner_symbole */



// ... et avec le drapeau présent si trouvé
struct Mi_trouve_st
mi_symbole_attribut_present (Mit_Symbole *symb, Mit_Symbole *symbat)
{
  if (!symb || symb == MI_TROU_SYMBOLE || symb->mi_type != MiTy_Symbole
      || !symbat || symbat == MI_TROU_SYMBOLE
      || symbat->mi_type != MiTy_Symbole)
    return (struct Mi_trouve_st)
    {
      .t_val = MI_NILV,.t_pres = false
    };
  if (symb->mi_attrs == NULL)
    return (struct Mi_trouve_st)
    {
      .t_val = MI_NILV,.t_pres = false
    };
  return mi_assoc_chercher (symb->mi_attrs, symbat);
}				// fin mi_symbole_attribut_present

// mettre dans un symbole un attribut lié à une valeur
void
mi_symbole_mettre_attribut (Mit_Symbole *symb, Mit_Symbole *symbat,
                            Mit_Val val)
{
  if (!symb || symb == MI_TROU_SYMBOLE || symb->mi_type != MiTy_Symbole)
    return;
  if (!symbat || symbat == MI_TROU_SYMBOLE || symbat->mi_type != MiTy_Symbole)
    return;
  if (val.miva_ptr == NULL)
    return;
  symb->mi_attrs = mi_assoc_mettre (symb->mi_attrs, symbat, val);
}				// fin mi_symbole_mettre_attribut

void
mi_symbole_enlever_attribut (Mit_Symbole *symb, Mit_Symbole *symbat)
{
  if (!symb || symb == MI_TROU_SYMBOLE || symb->mi_type != MiTy_Symbole)
    return;
  if (!symbat || symbat == MI_TROU_SYMBOLE || symbat->mi_type != MiTy_Symbole)
    return;
#warning mi_symbole_enlever_attribut incomplet
}				// fin mi_symbole_enlever_attribut


static void
mi_impr_radical (const struct MiSt_Radical_st *rad, int prof)
{
  if (!rad)
    return;
  assert (rad->urad_nmagiq == MI_RAD_NMAGIQ);
  assert (rad->urad_nom != NULL && rad->urad_nom->mi_type == MiTy_Chaine);
  bool feuille = rad->urad_gauche == NULL && rad->urad_droit == NULL;
  for (int ix = 0; ix < prof; ix++)
    putchar (' ');
  printf ("%s%c%srad@%p '%s'", MI_TERMINAL_GRAS, feuille ? '*' : '+',
          MI_TERMINAL_NORMAL, rad, rad->urad_nom->mi_car);
  switch (rad->urad_couleur)
    {
    case crad_noir:
      printf (" %s*NOIR*%s", MI_TERMINAL_ITALIQUE, MI_TERMINAL_NORMAL);
      break;
    case crad_rouge:
      printf (" %s*ROUGE*%s", MI_TERMINAL_ITALIQUE, MI_TERMINAL_NORMAL);
      break;
    default:
      printf (" *?%d?*", (int) rad->urad_couleur);
      break;
    }
  if (!feuille)
    printf (" gch@%p drt@%p", (void *) rad->urad_gauche,
            (void *) rad->urad_droit);
  if (rad->urad_parent)
    printf (" par@%p", (void *) rad->urad_parent);
  putchar ('\n');
  if (feuille)
    return;
  mi_impr_radical (rad->urad_gauche, prof + 1);
  for (int ix = 0; ix < prof; ix++)
    putchar (' ');
  printf ("%s/%srad@%p '%s'", MI_TERMINAL_GRAS, MI_TERMINAL_NORMAL, rad,
          rad->urad_nom->mi_car);
  putchar ('\n');
  mi_impr_radical (rad->urad_droit, prof + 1);
  for (int ix = 0; ix < prof; ix++)
    putchar (' ');
  printf ("%s-%srad@%p '%s'", MI_TERMINAL_GRAS, MI_TERMINAL_NORMAL, rad,
          rad->urad_nom->mi_car);
  putchar ('\n');
}				// fin mi_impr_radical


void
mi_afficher_radicaux_en (const char *fich, int lin, const char *msg)
{
  static int count;
  count++;
  printf ("%saffichage des radicaux %s #%d%s; %s:%d mi_racine_radical@%p\n",
          MI_TERMINAL_GRAS, msg,
          count, MI_TERMINAL_NORMAL, fich, lin, (void *) mi_racine_radical);
  mi_impr_radical (mi_racine_radical, 0);
  fputc ('\n', stdout);
}				// fin mi_afficher_radicaux_en

// pour être appellé directement depuis le débogueur:
#undef mi_afficher_radicaux	// oublier que c'est une macro
// redéfinir comme une fonction
void
mi_afficher_radicaux (const char *msg)
{
  mi_afficher_radicaux_en ("?", 0, msg);
}				// fin mi_afficher_radicaux





#define MI_TABLATTR_NMAGIC 0x39f99e27	/* 972660263 */
struct MiSt_TableAttributs_st
{
  unsigned tat_nmagiq;		// toujours MI_TABLATTR_NMAGIC
  unsigned tat_taille;		// taille alloué
  unsigned tat_compteur;	// compteur de symboles utilisés
  const Mit_Symbole *tat_symboles[];	// de tat_taille
};

static bool
mi_ajouter_attribut_dans_table (const Mit_Symbole *sy, const Mit_Val va
                                __attribute__ ((unused)), void *client)
{
  struct MiSt_TableAttributs_st *tabat =
  (struct MiSt_TableAttributs_st *) client;
  assert (tabat && tabat->tat_nmagiq == MI_TABLATTR_NMAGIC);
  assert (tabat->tat_compteur < tabat->tat_taille);
  tabat->tat_symboles[tabat->tat_compteur++] = sy;
  return false;
}				// fin mi_ajouter_attribut_dans_table

void
mi_afficher_contenu_symbole (FILE * fi, const Mit_Symbole *sy)
{
  if (!fi)
    return;
  if (!sy || sy == MI_TROU_SYMBOLE)
    return;
  assert (sy->mi_type == MiTy_Symbole);
  fputc ('\n', fi);
  char tampind[16];
  fprintf (fi, "%s**%s", (fi == stdout) ? MI_TERMINAL_GRAS : "",
           (fi == stdout) ? MI_TERMINAL_NORMAL : "");
  fprintf (fi, " %s%s%s%s %s@%p%s %s",
           (fi == stdout) ? MI_TERMINAL_GRAS : "",
           mi_symbole_chaine (sy), mi_symbole_indice_ch (tampind, sy),
           (fi == stdout) ? MI_TERMINAL_NORMAL : "",
           (fi == stdout) ? MI_TERMINAL_ITALIQUE : "",
           (void *) sy,
           (fi == stdout) ? MI_TERMINAL_NORMAL : "",
           sy->mi_predef ? "prédefini" : "normal");
  fprintf (fi, " %s**%s\n",
           (fi == stdout) ? MI_TERMINAL_GRAS : "",
           (fi == stdout) ? MI_TERMINAL_NORMAL : "");
  unsigned nbat = mi_assoc_compte (sy->mi_attrs);
  if (nbat > 0)
    {
      struct MiSt_TableAttributs_st *tabat =	//
      calloc (1,
                sizeof (struct MiSt_TableAttributs_st) + (nbat +
                    1) *
                sizeof (Mit_Symbole *));
      if (!tabat)
        MI_FATALPRINTF ("impossible d'allouer table pour %d attributs (%s)",
                        nbat, strerror (errno));
      tabat->tat_nmagiq = MI_TABLATTR_NMAGIC;
      tabat->tat_taille = nbat + 1;
      tabat->tat_compteur = 0;
      mi_assoc_iterer (sy->mi_attrs, mi_ajouter_attribut_dans_table, tabat);
      assert (tabat->tat_compteur == nbat);
      qsort (tabat->tat_symboles, nbat, sizeof (Mit_Symbole *),
             mi_cmp_symboleptr);
      fprintf (fi, "-- %d attributs --\n", nbat);
      for (unsigned ix = 0; ix < nbat; ix++)
        {
          const Mit_Symbole *syat = tabat->tat_symboles[ix];
          assert (syat != NULL && syat != MI_TROU_SYMBOLE
                  && syat->mi_type == MiTy_Symbole);
          fprintf (fi, "* %s%s: ",
                   mi_symbole_chaine (syat), mi_symbole_indice_ch (tampind,
                       syat));
          mi_afficher_valeur (fi,
                              mi_assoc_chercher (sy->mi_attrs, syat).t_val);
        }
      free (tabat), tabat = NULL;
    }
  else
    fprintf (fi, "-- aucun attribut --\n");
  unsigned nbcomp = mi_vecteur_taille (sy->mi_comps);
  if (nbcomp > 0)
    {
      fprintf (fi, "-- %d composants --\n", nbcomp);
      for (unsigned ix = 0; ix < nbcomp; ix++)
        {
          fprintf (fi, "[%d]: ", ix);
          mi_afficher_valeur (fi, mi_vecteur_comp (sy->mi_comps, ix).t_val);
        }
    }
  else
    fprintf (fi, "-- aucun composant --\n");
}
