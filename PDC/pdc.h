// ----------------------------------------------
// some trigonometry utils

#define PI_RAD 3.14159265358979323846
#define DEG_TO_RAD(d) (((d)*PI_RAD)/180.0)
#define RAD_TO_DEG(r) (((r)*180.0)/PI_RAD)

#ifdef __GNUC__
#define _inline __inline
#endif

// 3D vector
typedef struct
{
  double x;
  double y;
  double z;
} vec3;

// sin/cos values
typedef struct
{
  double s;
  double c;
} sc_t;

static _inline void sc_rotate(sc_t *sc, double an)
{
#if 1
  sc->s = sin(an);
  sc->c = cos(an);
#else
  // seem ok with only > cosinus results
  double s = sin(an);
  sc->s = s;
  sc->c = sqrt(1 - s*s);
#endif
}

// sin/cos set for theta/phi rotation
typedef struct
{
  sc_t theta;
  sc_t phi;
} rot_t;

// ----------------------------------------------

// error, called if something wrong
void PDC_error(const char *err_msg);

// ----------------------------------------------
// return x^2
static _inline double pow2(double x)
{
  return x*x;
}

#include "crystal.h"

// ----------------------------------------------
// simulation states
// axes = crystal axes with n(x) <= n(y) <= n(z)
//   z
//   |
//   .--- y
//  /
// x
// theta relative to z
// phi relative to x

// crystal indices for given lambda
typedef struct
{
  vec3 ind2;
  double n0;
  double n1;
  double n2;
  double n3;
} lambda_ind_t;

// photon parameters
typedef struct
{
  // index
  lambda_ind_t ind;                              // s direction indices for lambda
  bool fast;                                     // speed: true => fast else slow
  double n2;                                     // index^2 for s direction
  double n;                                      // index for s direction

  rot_t rot;                                     // rotations
  vec3 s;                                        // direction (in crystal for pump, in pump for signal/idler)
} pho_t;

// PDC configuration
struct pdc_conf_t
{
  enum e_crystal_type crystal_type;              // type of crystal
  struct crystal_info_t crystal_info;
  float crystal_temp;                            // temperature (celcius) todo
  float lambda_pump_nm;
  float lambda_signal_nm;
  float lambda_idler_nm;
  bool type1;                                    // true: type 1, false: type 2
  bool conf_valid;                               // true if init success

  // precomputed values for speed
  double kw_pump;                                // kw = 2*pi/lamda
  double kw_signal;
  double kw_idler;
  double k_wp_on_ws;                             // w_pump/w_signal
};

// states
struct pdc_states_t
{
  pho_t pump;
  pho_t signal;
  pho_t idler;
};

extern struct pdc_conf_t pdc;
extern struct pdc_states_t st;

void pdc_init_type(void);
void pdc_init(void);

void init_pho_lambda(pho_t *pho, double lambda);
void def_dir_index(pho_t *pho, const vec3 *sc);

void pump_update(void);
void pump_rotate(double theta, double phi);
void pho_update(pho_t *p);
void pdc_set_theta_signal(double theta);         // set theta for signal and idler
void pdc_set_theta_idler(void);                  // depend of signal and pump
void pdc_set_phi(double phi);                    // set phi for signal and idler
double get_k_diff2(void);

// ----------------------------------------------
// search functions

void pdc_init_search(void);

double pdc_search_pm(double theta_signal, double phi_signal, bool adjust_phi_signal, float *best_theta_pump, float *best_phi_pump);
double pdc_adjust_pm_local(double theta_signal, double phi_signal, bool adjust_phi_signal, float *best_theta_pump, float *best_phi_pump);

double get_min_pm_error_adjust_phi(int *i_phi, bool scan);
