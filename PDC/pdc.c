#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include "pdc.h"

struct pdc_conf_t pdc = { 0 };
struct pdc_states_t st = { 0 };

// set pdc type using config
void pdc_init_type(void)
{
  // configure index speed
  st.pump.fast   = true;                         // pdc type 1 and 2
  st.signal.fast = !pdc.type1;                   // fast if type 2 used
  st.idler.fast  = false;
}

// init states using config, set conf_valid flag
void pdc_init(void)
{
  pdc.conf_valid = false;                        // set default
  st.pump.ind.ind2.x = 1;                        // n value used as no init flag for gui
  st.signal.ind.ind2.x = 1;
  st.idler.ind.ind2.x = 1;

  // check all wavelength are in crystal transparency range
  if (    (pdc.lambda_pump_nm < pdc.crystal_info.min_lambda_nm)
       || (pdc.lambda_pump_nm > pdc.crystal_info.max_lambda_nm))
    return;
  init_pho_lambda(&st.pump, pdc.lambda_pump_nm);

  // check lambda signal
  if (    (pdc.lambda_signal_nm < pdc.crystal_info.min_lambda_nm)
       || (pdc.lambda_signal_nm > pdc.crystal_info.max_lambda_nm)
       || (pdc.lambda_signal_nm < (pdc.lambda_pump_nm + 1)))
    return;
  init_pho_lambda(&st.signal, pdc.lambda_signal_nm);

  // define lambda idler for energy conservation
  pdc.lambda_idler_nm = (pdc.lambda_pump_nm * pdc.lambda_signal_nm)/(pdc.lambda_signal_nm - pdc.lambda_pump_nm);
  if (pdc.lambda_idler_nm > pdc.crystal_info.max_lambda_nm)
    return;
  init_pho_lambda(&st.idler, pdc.lambda_idler_nm);

  // wavelength ok, init photons
  pdc.conf_valid = true;
  pdc_init_type();

  // ----------------------------------
  // precompute w/c for speed
  // w = 2.pi.f, f = c/lambda, => w/c = 2.pi/lambda
#if 0
  #define NM_SCALE 1e9
#else
  // note: real scale value is 1e9 (nm => m), but this produce big numbers not readable.
  // Then 1000 is used instead to produce readable values in graph legend.
  // This do not affect graph.
  #define NM_SCALE 1000
#endif
  pdc.kw_pump   = (2*PI_RAD*NM_SCALE)/pdc.lambda_pump_nm;
  pdc.kw_signal = (2*PI_RAD*NM_SCALE)/pdc.lambda_signal_nm;
  pdc.kw_idler  = (2*PI_RAD*NM_SCALE)/pdc.lambda_idler_nm;

  // precompute w_pump/w_signal (used to define idler theta)
  pdc.k_wp_on_ws = pdc.lambda_signal_nm / pdc.lambda_pump_nm;  // w_pump/w_signal
}

// compute one time lambda dependant constants
void init_pho_lambda(pho_t *pho, double lambda)
{
  double nx2, ny2, nz2, nx2_nz2, ny2_nz2, nx2_ny2;
  lambda_ind_t *i = &pho->ind;

  // update index
  get_crystal_index(pdc.crystal_type, lambda, pdc.crystal_temp, &i->ind2, NULL);

  nx2 = i->ind2.x;
  ny2 = i->ind2.y;
  nz2 = i->ind2.z;

  nx2_nz2 = nx2*nz2;
  ny2_nz2 = ny2*nz2;
  nx2_ny2 = nx2*ny2;

  i->n0 = 2*nx2_ny2*nz2;
  i->n1 = ny2_nz2 + nx2_nz2;
  i->n2 = ny2_nz2 + nx2_ny2;
  i->n3 = nx2_nz2 + nx2_ny2;
}

// get index for sc (must be defined in crystal coordinates)
void def_dir_index(pho_t *pho, const vec3 *sc)
{
  const lambda_ind_t *i = &pho->ind;

  double sx2 = sc->x * sc->x;
  double sy2 = sc->y * sc->y;
  double sz2 = sc->z * sc->z;

  double a = i->n1*sz2 + i->n2*sy2 + i->n3*sx2;
  double b = i->ind2.z*sz2 + i->ind2.y*sy2 + i->ind2.x*sx2;
  double c = a*a - 2*b*i->n0;
  if (c > 0)                                     // small negative value may appear near 0
  {
    double e, d = sqrt(c);
    if (pho->fast)
      e = a + d;
    else
      e = a - d;
    pho->n2 = i->n0 / e;
  }
  else
    pho->n2 = i->n0 / a;
  pho->n = sqrt(pho->n2);
}

// define theta signal/idler.
void pdc_set_theta_signal(double theta)
{
  sc_rotate(&st.signal.rot.theta, theta);
}

// set theta idler depending of theta signal (must be < PI/2)
void pdc_set_theta_idler(void)
{
  pho_t *p = &st.pump;
  pho_t *s = &st.signal;
  pho_t *i = &st.idler;

  // theta_idler = asin( (n_signal*sin(theta_signal))/sqrt(n_signal^2 + n_pump^2*((w_pump^2)/(w_signal^2)) - 2*n_signal*n_pump*(w_pump/w_signal)*cos(theta_signal)));
  double a = p->n * pdc.k_wp_on_ws;                   // (n_pump*w_pump)/w_signal
  double b = s->n * (s->n - 2.0 * a * s->rot.theta.c);
  double c = (s->n * s->rot.theta.s) / sqrt(a*a + b);

#if 0
  // reference code
  double theta_idler = asin(c);
  sc_rotate(&i->rot.theta, theta_idler);
#else
  // speed optimized
  i->rot.theta.s = c;
  i->rot.theta.c = sqrt(1.0 - c*c);
#endif
}

// set phi for signal and idler
void pdc_set_phi(double phi)
{
  pho_t *s = &st.signal;
  pho_t *i = &st.idler;

  sc_rotate(&s->rot.phi, phi);

  // set phi idler to signal opposite phi
  i->rot.phi.s = -s->rot.phi.s;  // = sin(phi + PI)
  i->rot.phi.c = -s->rot.phi.c;  // = cos(phi + PI)
}

// update pump using current rp angles
void pump_update(void)
{
  pho_t *p  = &st.pump;
  rot_t *rp = &st.pump.rot;

  // define s in crystal
  p->s.x = rp->theta.s * rp->phi.c;
  p->s.y = rp->theta.s * rp->phi.s;
  p->s.z = rp->theta.c;

  // update index, fast
  def_dir_index(p, &p->s);
}

// set pump beam angle in crystal coordinates
void pump_rotate(double theta, double phi)
{
  rot_t *rp = &st.pump.rot;
  sc_rotate(&rp->theta, theta);
  sc_rotate(&rp->phi, phi);
  pump_update();
}

// set signal or idler angles in beam coordinates
void pho_update(pho_t *pho)
{
  const pho_t *p  = &st.pump;
  const rot_t *rp = &st.pump.rot;
  vec3 sc;
  
  // s in pump
  rot_t *r = &pho->rot;
  pho->s.x = r->theta.s * r->phi.c;
  pho->s.y = r->theta.s * r->phi.s;
  pho->s.z = r->theta.c;

  // s in crystal
	sc.x = rp->theta.c * rp->phi.c * pho->s.x - rp->phi.s * pho->s.y + p->s.x * pho->s.z;
	sc.y = rp->theta.c * rp->phi.s * pho->s.x + rp->phi.c * pho->s.y + p->s.y * pho->s.z;
	sc.z = rp->theta.c * pho->s.z - rp->theta.s * pho->s.x;

  // update index
  def_dir_index(pho, &sc);
}

// get phase match error (return squared)
double get_k_diff2(void)
{
  vec3 dk;
  double kp, ks, ki, diff2;

  kp = st.pump.n * pdc.kw_pump;
  ks = st.signal.n * pdc.kw_signal;
  ki = st.idler.n * pdc.kw_idler;

#if 0
  dk.z = st.signal.s.z * ks + st.idler.s.z * ki - kp;
  diff2 = dk.z*dk.z;
#else
  dk.x = st.signal.s.x * ks + st.idler.s.x * ki;
  dk.y = st.signal.s.y * ks + st.idler.s.y * ki;
  dk.z = st.signal.s.z * ks + st.idler.s.z * ki - kp;
  diff2 = dk.x*dk.x + dk.y*dk.y + dk.z*dk.z;
#endif
  return diff2;
}

// ----------------------------------------------
// search functions

// precomputed sin_cos
struct sin_cos_t
{
  float s;
  float c;
};

#if 0
#define I_PI 2048                                // 0.08 degree step, 16k bytes (fit cache l1)
#define I_COARSE 8                               // (8 * 180)/I_PI = 0.703.. deg
#else
#define I_PI 8192                                // 0.022 degree step, 64k bytes
#define I_COARSE 32                              // (32 * 180)/I_PI = 0.703.. deg
#endif

#define I_PI_MSK (I_PI-1)                        // mask for periodic index

static struct sin_cos_t sin_cos_i[I_PI];

void pdc_init_search(void)
{
  int i;
  float an;
  for (i=0, an=0.0f; i<I_PI; i++, an+=(float)(PI_RAD/I_PI))
  {
    sin_cos_i[i].s = sinf(an);
    sin_cos_i[i].c = cosf(an);
  }
}

// update states and return pm error
static double eval_pm_error_pump(void)
{
  pump_update();
  pho_update(&st.signal);
  pdc_set_theta_idler();
  pho_update(&st.idler);
  return get_k_diff2();
}

// coarse search min pm error using I_COARSE step.
// no phi signal adjust.
static double pdc_search_pm_coarse(int *best_theta, int *best_phi)
{
  double e, min_err = 1e6;
  int i, j;
  for (i=0; i<I_PI/2; i+=I_COARSE)
  {
    struct sin_cos_t *sc_theta = &sin_cos_i[i];
    st.pump.rot.theta.s = sc_theta->s;
    st.pump.rot.theta.c = sc_theta->c;
    for (j=0; j<I_PI; j+=I_COARSE)
    {
      struct sin_cos_t *sc_phi = &sin_cos_i[j];
      st.pump.rot.phi.s = sc_phi->s;
      st.pump.rot.phi.c = sc_phi->c;
      e = eval_pm_error_pump();
      if (e < min_err)
      {
        min_err = e;
        *best_theta = i;
        *best_phi = j;
      }
    }
  }
  return min_err;
}

// search min pm error using 180/I_PI deg step.
// no phi signal adjust.
static double pdc_search_pm_fine(int *best_theta, int *best_phi, double min_err)
{
  int i, j;
  int i0 = *best_theta - I_COARSE;
  int i1 = i0 + I_COARSE;
  int j0 = *best_phi - I_COARSE;
  int j1 = j0 + I_COARSE;
  
  for (i=i0; i<=i1; i++)
  {
    int theta = i & I_PI_MSK;
    struct sin_cos_t *sc_theta = &sin_cos_i[theta];
    st.pump.rot.theta.s = sc_theta->s;
    st.pump.rot.theta.c = sc_theta->c;

    for (j=j0; j<=j1; j++)
    {
      double e;
      int phi = j & I_PI_MSK;
      struct sin_cos_t *sc_phi = &sin_cos_i[phi];
      st.pump.rot.phi.s = sc_phi->s;
      st.pump.rot.phi.c = sc_phi->c;
      e = eval_pm_error_pump();
      if (e < min_err)
      {
        min_err = e;
        *best_theta = theta;
        *best_phi = phi;
      }
    }
  }
  return min_err;
}

// note: return result angles in degree
// todo ? : adjust_phi_signal ignored.
double pdc_search_pm(double theta_signal, double phi_signal, bool adjust_phi_signal, float *best_theta_pump, float *best_phi_pump)
{
  int best_theta = 0;
  int best_phi = 0;
  double min_err;
  
  pdc_set_theta_signal(theta_signal);            // fixed theta
  pdc_set_phi(phi_signal);                       // fixed phi
  
  min_err = pdc_search_pm_coarse(&best_theta, &best_phi);
  min_err = pdc_search_pm_fine(&best_theta, &best_phi, min_err);

  *best_theta_pump = (best_theta*180.0f)/I_PI;
  *best_phi_pump = (best_phi*180.0f)/I_PI;
  return min_err;
}

// adjust +/- 1 deg max
// todo ? : adjust_phi_signal ignored.
double pdc_adjust_pm_local(double theta_signal, double phi_signal, bool adjust_phi_signal, float *best_theta_pump, float *best_phi_pump)
{
  double min_err;

  // current best value
  int best_theta = (int)((*best_theta_pump*I_PI)/180.0f);
  int best_phi = (int)((*best_phi_pump*I_PI)/180.0f);

  pdc_set_theta_signal(theta_signal);            // fixed theta
  pdc_set_phi(phi_signal);                       // fixed phi
  
  min_err = pdc_search_pm_fine(&best_theta, &best_phi, 1e6);

  *best_theta_pump = (best_theta*180.0f)/I_PI;
  *best_phi_pump = (best_phi*180.0f)/I_PI;
  return min_err;
}

// ----------------------------------------------
// phi signal/idler adjust

static double eval_pm_error_phi(int i_phi)
{
  struct sin_cos_t *sc = &sin_cos_i[i_phi & I_PI_MSK];
  st.signal.rot.phi.s = sc->s;
  st.signal.rot.phi.c = sc->c;
  st.idler.rot.phi.s = -sc->s;                   // signal + PI
  st.idler.rot.phi.c = -sc->c;
  pho_update(&st.signal);
  pdc_set_theta_idler();
  pho_update(&st.idler);
  return get_k_diff2();
}

double get_min_pm_error_adjust_phi(int *i_phi, bool scan)
{
  double min_err;
  int i, i_ini;

  // find starting point
  if (scan)
  {
    *i_phi = 0;
    min_err = 1e6;
    for (i=0; i<I_PI; i+=I_PI/64)                // coarse search
    {
      double e = eval_pm_error_phi(i);
      if (e < min_err)
      {
        min_err = e;
        *i_phi = i;
      }
    }
  }
  else
    min_err = eval_pm_error_phi(*i_phi);         // initial error
  
  // adjust from current phi
  i_ini = *i_phi;
  
  // decrement search
  i = *i_phi;
  while (1)
  {
    double e;
    i = i - I_PI/64;
    e = eval_pm_error_phi(i);
    if (e >= min_err)
      break;
    min_err = e;
    *i_phi = i;
  }

  if (*i_phi != i_ini)                           // found at left, skip right search
    return min_err;

  // increment search
  i = *i_phi;
  while (1)
  {
    double e;
    i = i + I_PI/64;
    e = eval_pm_error_phi(i);
    if (e >= min_err)
      break;
    min_err = e;
    *i_phi = i;
  }
  return min_err;
}
